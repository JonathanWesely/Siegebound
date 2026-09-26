// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Templates/Function.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

#include "SiegeControlsHelpWidget.generated.h"

class APlayerController;
class UBorder;
class UButton;
class UHorizontalBox;
class UInputAction;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class USiegeControlsDetailWidget;
class USiegeControlsHelpRowWidget;
class USiegeKeyboardLayoutSubsystem;
// TASK-1432 — forward-declared, ⛔ never included here: the overlay talks to the menu-input
// subsystem from exactly two call sites in the .cpp and this header stays free of it.
class USiegeMenuInputSubsystem;

/** The controls overlay's own log lane (the LogSiegeWarMap / LogSiegeAccount house shape). */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeControlsHelp, Log, All);

/**
 *  Fired by one row when the player activates it. NON-DYNAMIC and one-parameter —
 *  UDeckSlotEntryWidget::OnLeftClicked's exact contract, cloned: the row reports the
 *  gesture and its identity, and the MEANING lives in the receiver.
 */
DECLARE_DELEGATE_OneParam(FOnSiegeControlsRowActivated, FName /*ActionId*/);

/** Broadcast by the overlay whenever a row is activated — ⭐ THE TASK-707 SEAM (see §7 of the class comment). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeControlsRowSelected, FName, ActionId);

/**
 *  Fired by the full-screen detail view when the player asks to go back to the list (TASK-707).
 *  NON-DYNAMIC and parameterless — the exact FOnSiegeControlsRowActivated contract, mirrored:
 *  the view reports the GESTURE and the overlay decides what it means.
 *
 *  ⛔ THIS IS NOT A CLOSE. It returns to the list; the overlay's own Close button and the toggle
 *  key remain the ONLY two routes that close the overlay (`AS-§6 A-2`, `HELP-§5`), and ⛔ neither
 *  this delegate nor anything behind it ever touches `Escape`.
 */
DECLARE_DELEGATE(FOnSiegeControlsDetailBackRequested);

/** Broadcast on every REAL open-state change. The controller binds it so a Close-button close releases the cursor posture. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeControlsHelpOpenChanged, bool, bOpen);

/**
 *  ⛔⛔ THE LANE AUDIT, AS A TYPE (`HELP-§1`; handoffs/TASK-704-programmer.md §1.1).
 *
 *  Which input lane an action's DISPLAYED KEY comes from — and therefore whether
 *  USiegeKeyboardLayoutSubsystem::GetPositionalKey may be applied to it AT ALL.
 *
 *  ⛔ THE TRAP THIS ENUM EXISTS TO CLOSE (SiegePlayerController.h:1222-1226): the layout
 *  subsystem rewrites the APPLIED IMC duplicate's `.Key` fields WHOLESALE
 *  (SiegeKeyboardLayoutStatics.cpp:236), so an Enhanced-Input-mapped action's live key is
 *  ALREADY translated. A second GetPositionalKey call on it applies the map TWICE. On a
 *  QWERTY host the map is empty and the two are indistinguishable; ⭐ on US-Dvorak
 *  `F` -> `U` -> `G`, and the help screen teaches the wrong key.
 *
 *  ⛔ PLAIN `uint8` UENUM, and it is NEVER a BlueprintImplementableEvent parameter
 *  (`HELP-§3`'s FString/int32/bool/uint8-only BIE law).
 */
UENUM()
enum class ESiegeInputLane : uint8
{
	/**
	 *  The key lives in IMC_Hero and the subsystem retargets the APPLIED duplicate.
	 *  ⛔ ZERO GetPositionalKey calls on this lane's primary path: the key read back from
	 *  UEnhancedInputSubsystemInterface::QueryKeysMappedToAction IS the translated key
	 *  (that query reads the ACTIVE contexts —
	 *  EnhancedInputSubsystemInterface.h:381-384 — and the active context is the retargeted
	 *  duplicate AHeroCharacter passed to AddMappingContext, HeroCharacter.cpp:271-275).
	 */
	MappedAction,

	/**
	 *  A RAW-polled key that is NOT a letter — a mouse button, Escape, or the wheel — ⭐ or a
	 *  Slate-delivered mouse gesture: the lane selects the VERBATIM-LABEL algorithm, not the
	 *  delivery route.
	 *  ⛔ No call needed: the translation table is A..Z ONLY
	 *  (SiegeKeyboardLayoutStatics.cpp:57-63), so GetPositionalKey on these is a PROVABLE
	 *  identity. The reference key is labelled directly.
	 *
	 *  ⚠️ THE SECOND CLAUSE IS ADDITIVE AND IT DESCRIBES A SHIPPED MEMBER, ⛔ not a new rule
	 *  (TASK-870, from qa/TASK-816.md N-1): Interface.MapMarks sits on this lane and its four
	 *  gestures arrive as Slate events on a focused widget, ⛔ not as controller polls. The lane
	 *  ruling was measured and UPHELD; what was stale was this sentence, which described the
	 *  lane's DELIVERY ROUTE as though that were its membership test. ⛔ No enum value and ⛔ no
	 *  row's Lane is changed by that task.
	 */
	RawNonLetter,

	/**
	 *  A RAW-polled LETTER. Exactly one row: the assistant console's `Z` accept.
	 *  ⚠️ The shipped COMPARISON does translate (SiegeAssistantConsoleWidget.cpp:966); the
	 *  DISPLAYED LABEL deliberately does NOT — `KBD-§8` / `KBD-§0` ruling 1 pin every
	 *  human-facing accept-key string to a literal `Z` on every layout
	 *  (SiegeKeyboardLayoutSubsystem.h:256-260). See bLiteralKeyLabel.
	 */
	RawLetter,

	/** A mouse click on a UI element. No key at all ⇒ the row renders a pointer chip, ⛔ never a key. */
	PointerOnly
};

/**
 *  One row of the controls overlay — the data shape handed across the TASK-704 -> TASK-706
 *  seam (handoffs/TASK-704-programmer.md §3), with every field's authority cited there.
 *
 *  ⛔ THE RAW ACTION IDENTITY CROSSES THE SEAM AND EXACTLY ONE TRANSLATION HAPPENS
 *  DOWNSTREAM. QwertyReferenceKeys is a FALLBACK and a TEST FIXTURE for Lane A, ⛔ never the
 *  displayed truth while the action resolves in an active context.
 */
USTRUCT()
struct FSiegeControlsHelpAction
{
	GENERATED_BODY()

	/** Stable id, e.g. "Orders.Ambush". The tests assert on this; ⛔ never on a key letter. */
	UPROPERTY()
	FName ActionId;

	/** Hero | Cards | Orders | PickMode | Interface — the overlay's grouping headers, in registry order. */
	UPROPERTY()
	FName Category;

	/** The row's headline, e.g. "Ambush". */
	UPROPERTY()
	FText DisplayName;

	/** The one-line description shown on the row (TASK-704 §4, verbatim). */
	UPROPERTY()
	FText OneLine;

	/**
	 *  ⭐ THE FULL-SCREEN DETAIL PROSE. FILLED BY TASK-707 from
	 *  handoffs/TASK-704-programmer.md §4 — ⛔ transferred, ⛔ never re-authored. §4's
	 *  `file:line` citations ride in a C++ COMMENT above each string rather than in the
	 *  player-facing prose (`HELP-§2`'s own instruction for an unavoidable literal; the route
	 *  TASK-706 §4(e) assigned to this task).
	 *
	 *  ⭐⭐ MOVABLE KEYS INSIDE THIS PROSE ARE `{ActionId}` TOKENS, ⛔ NEVER TYPED LETTERS.
	 *  ResolveDetailTokens splices each one with that row's LIVE-DERIVED chip at compose time,
	 *  so a sentence such as "Pressing {Orders.Attack} or {Orders.Defend} destroys this group"
	 *  is right on every layout. ⛔ A typed `T` there would be the exact `HELP-§1` defect the
	 *  key chips exist to prevent, one line lower down the same screen.
	 *
	 *  ⚠️ An EMPTY value is still safe, not a blank page: ComposeDetailForDisplay() renders the
	 *  explicit "(undocumented — TODO)" string (`HELP-§2` mechanism 2: a visible gap gets
	 *  fixed, a silent omission does not).
	 *
	 *  ⛔ NO TUNABLE'S VALUE IS RE-TYPED HERE. TASK-704 §4 deliberately NAMES tunables
	 *  (GroupRadiusWheelStep, MeleeCooldown, EnemyRevealCost …) instead of restating numbers,
	 *  and this task keeps that (704 U-5; the M7.7 "in 400"/AoERadius-700 lesson). The ONE
	 *  stated number in the whole registry is the war map's 30 gold, because Jonathan's own
	 *  words are the source and they are quoted at the property (CommanderNpc.h:297-311).
	 */
	UPROPERTY()
	FText Detail;

	/**
	 *  ⭐ THE OTHER CONTROLS THAT BELONG TO THIS ONE — Jonathan's "all the controls with it".
	 *
	 *  Registry ids rendered UNDERNEATH this row's own detail prose, each with its LIVE-DERIVED
	 *  key chip and its OWN detail text. ⇒ the Ambush page answers "what the first, second and
	 *  third circles do, how to resize them, how to exit the command" by rendering
	 *  PickMode.Confirm / PickMode.Resize / PickMode.Cancel — ⛔ WITHOUT a second copy of that
	 *  prose existing anywhere (`HELP-§2`: one definition, two renderings, nothing to drift).
	 *
	 *  ⛔ ONE LEVEL DEEP, STRUCTURALLY. ComposeDetailContent reads the related rows' text and
	 *  ⛔ never their own RelatedActionIds, so a cycle in this data cannot recurse.
	 *  ⛔ An id that is not in the registry is DROPPED rather than rendered blank.
	 */
	UPROPERTY()
	TArray<FName> RelatedActionIds;

	/** Which lane the DISPLAYED key comes from. ⛔ Read this before touching any label code. */
	UPROPERTY()
	ESiegeInputLane Lane = ESiegeInputLane::MappedAction;

	/**
	 *  Lane A only — the /Game/Input/Actions/IA_* path, soft so an absent asset is INERT and
	 *  ⛔ never a crash (the IA_CmdAmbush / IA_CmdFollow / IA_AssistantConsole / IA_WarMap
	 *  precedent). ⛔ An unresolvable action does NOT hide the row: it renders with no key
	 *  chip (`HELP-§2` mechanism 2).
	 */
	UPROPERTY()
	TArray<TSoftObjectPtr<UInputAction>> Actions;

	/**
	 *  Lane A: the FALLBACK + the test fixture, ⛔ never the truth while the action resolves.
	 *  Lanes B/C: THE truth (identity by construction / pinned by `KBD-§8`).
	 *  Lane D: empty.
	 */
	UPROPERTY()
	TArray<FKey> QwertyReferenceKeys;

	/**
	 *  Lane C only, true for EXACTLY ONE row (`Interface.AssistantAccept`).
	 *  ⛔ `KBD-§8` / `KBD-§0` ruling 1: the human-facing accept-key string is `Z` on EVERY
	 *  layout, because the console's own live status line two inches away says `Z`
	 *  (SiegeAssistantConsoleWidget.cpp:71, raised at :1374-1377).
	 *  ⚖️ FLAGGED FOR JONATHAN as F-1 in handoffs/TASK-704-programmer.md §8 — the ONE place
	 *  `HELP-§1` (derive) and `KBD-§8` (literal) point opposite ways. Held at the literal so
	 *  the help cannot contradict the live prompt. ⭐ REVERSING IT IS THIS ONE FLAG, and it
	 *  also means amending the console's status line — a `KBD-§8` amendment, not a help edit.
	 */
	UPROPERTY()
	bool bLiteralKeyLabel = false;

	/** Lane D — render a pointer affordance, ⛔ never a key chip. */
	UPROPERTY()
	bool bPointerOnly = false;
};

/**
 *  ONE "related control" block on a detail page — a second row's chip, name and detail prose,
 *  rendered underneath the page's own (TASK-707).
 *
 *  ⛔ PLAIN C++, ⛔ NOT A USTRUCT, and that is deliberate: it holds only FText/FName, it is
 *  never a UPROPERTY, it is never a Blueprint parameter (`HELP-§3` bans struct BIE params
 *  outright), and keeping it out of reflection keeps the `EscapeIsNotClaimed` FKey-property
 *  sweep meaningful — there is no reflected surface here for a key to hide in.
 */
struct FSiegeControlsDetailEntry
{
	/** The related registry row's id. ⛔ Always a REAL row: ComposeDetailContent drops unknown ids. */
	FName ActionId;

	/** That row's LIVE-DERIVED chip, composed by ComposeKeyChipLabel — ⛔ never typed. */
	FText KeyChip;

	/** That row's headline, e.g. "Resize the circle". */
	FText DisplayName;

	/** That row's OWN detail prose, through ComposeDetailForDisplay — ⛔ never a second copy of it. */
	FText Body;
};

/**
 *  Everything the full-screen detail view renders for one action (TASK-707).
 *
 *  ⭐ COMPOSED BY THE PURE REGISTRY, RENDERED BY THE WIDGET — the same split that lets
 *  Tests/SiegeControlsHelpTest.cpp assert the whole page on both simulated layouts with ⛔ no
 *  viewport, ⛔ no world and ⛔ no Slate. ⛔ Plain C++ for the same reasons as
 *  FSiegeControlsDetailEntry.
 */
struct FSiegeControlsDetailContent
{
	/** The row this page is for, or NAME_None on an unstamped view. */
	FName ActionId;

	/** The page headline — the row's DisplayName. */
	FText Title;

	/** The page's own LIVE-DERIVED key chip. */
	FText KeyChip;

	/** The row's one-liner, repeated at the top as the page's summary. */
	FText Summary;

	/** ⭐ The detail prose, with every `{ActionId}` token already spliced with a derived chip. */
	FText Body;

	/** ⭐ Jonathan's "all the controls with it" — the RelatedActionIds rows, in registry-declared order. */
	TArray<FSiegeControlsDetailEntry> Related;
};

/**
 *  THE ACTION REGISTRY + THE PURE LABEL LANE — the whole of `HELP-§1`'s machinery, with ⛔ no
 *  world, ⛔ no viewport and ⛔ no OS anywhere in it, so Tests/SiegeControlsHelpTest.cpp can
 *  drive all four lanes and BOTH layout states on a QWERTY machine.
 *
 *  ⛔ Plain C++ statics, ⛔ NOT UFUNCTIONs (this is a data table and a set of pure functions over
 *  it, not a Blueprint API). ⛔ ZERO defaulted parameters anywhere below, so `SC-§33` cannot fire
 *  at all — structurally, not by care.
 *
 *  ⭐ THE LIVE PARTS ARE INJECTED, ⛔ NEVER REACHED FOR: the layout accessor arrives as a
 *  pointer that may be null, and Enhanced Input arrives as a callable. That is what lets the
 *  suite drive the WHOLE detail page — chips, prose, related blocks — on both simulated layouts
 *  with no world and no viewport, which is the only instrument that can catch a wrong key on a
 *  layout nobody here owns hardware for.
 */
struct GITCLAUDEUNREALTEST_API FSiegeControlsHelpRegistry
{
	/**
	 *  The registry rows, in category order (Hero -> Cards -> Orders -> PickMode -> Interface):
	 *  handoffs/TASK-704-programmer.md §4's original set plus every row a later feature has
	 *  appended (TASK-823 added the stack upgrade, the placement wheel and the map marks).
	 *  ⚠️ THE COUNT IS DELIBERATELY NOT WRITTEN HERE — it said "24" until the registry grew, and
	 *  a transcribed count rots silently; ask GetActions().Num(). Function-local static: the FKey
	 *  constants resolve long after InputCore's own statics exist (the
	 *  SiegeKeyboardLayoutStatics.cpp:22-30 precedent — no cross-module static-init order
	 *  question).
	 */
	static const TArray<FSiegeControlsHelpAction>& GetActions();

	/** The row with this id, or null. */
	static const FSiegeControlsHelpAction* FindAction(FName ActionId);

	/**
	 *  ⭐⭐ THE ONE-TRANSLATION SEAM (`HELP-§1`; TASK-704 §1.2's algorithm, character-for-character).
	 *
	 *      Lane A : AppliedKeys non-empty  ->  AppliedKeys VERBATIM. ⛔ ZERO GetPositionalKey calls.
	 *               AppliedKeys EMPTY      ->  GetPositionalKey(QwertyReferenceKeys[i]) — EXACTLY ONE,
	 *                                          and only on a key that is provably NOT already
	 *                                          translated (nothing mapped it).
	 *      Lane B : QwertyReferenceKeys VERBATIM — identity by construction, ⛔ no call.
	 *      Lane C : QwertyReferenceKeys VERBATIM — `KBD-§8`, the literal is for the human.
	 *      Lane D : empty.
	 *
	 *  ⛔ IN NO BRANCH IS A TRANSLATION APPLIED TWICE. That is the entire audit.
	 *
	 *  @param Row           the registry row.
	 *  @param AppliedKeys   Lane A ONLY: what QueryKeysMappedToAction returned for this row's
	 *                       actions — ⚠️ ALREADY RETARGETED by the layout system. Pass an
	 *                       EMPTY array for every other lane and for an unresolvable action.
	 *  @param LayoutSubsystem the layout subsystem, or NULL. ⛔ Null is a FAIL-SAFE, not an
	 *                       error (`KBD-§5`): the fallback degrades to the reference keys
	 *                       unchanged and the game behaves exactly as it did before the
	 *                       feature existed.
	 */
	static TArray<FKey> ResolveRowDisplayKeys(
		const FSiegeControlsHelpAction& Row,
		const TArray<FKey>& AppliedKeys,
		const USiegeKeyboardLayoutSubsystem* LayoutSubsystem);

	/**
	 *  The key chip's text. Pointer-only ⇒ the pointer affordance. No keys ⇒ the explicit
	 *  "not bound" chip (⛔ never a blank chip — `HELP-§2` mechanism 2). Otherwise the keys'
	 *  own SHORT display names, slash-separated.
	 *
	 *  ⛔ NO LETTER IS EVER TYPED HERE. Every character in a key chip comes from
	 *  FKey::GetDisplayName on an FKey this function was HANDED (`HELP-§1`).
	 */
	static FText ComposeKeyChipLabel(const FSiegeControlsHelpAction& Row, const TArray<FKey>& DisplayKeys);

	/** The row's one-liner, or the pinned "(undocumented — TODO)" when it is empty (`HELP-§2`). */
	static FText ComposeOneLineForDisplay(const FSiegeControlsHelpAction& Row);

	/** The detail prose, or the same pinned TODO string. The detail renderer reads THIS, ⛔ never Row.Detail directly. */
	static FText ComposeDetailForDisplay(const FSiegeControlsHelpAction& Row);

	/** The group header for a Category FName. An unknown category renders its own name rather than nothing. */
	static FText GetCategoryDisplayText(FName Category);

	/** The pinned `HELP-§2` gap string, exposed so the tests and the detail lane assert against ONE definition. */
	static const TCHAR* GetUndocumentedText();

	/**
	 *  The `{ActionId}` token for a row, e.g. "{Orders.Attack}" — ONE definition, so the prose
	 *  that writes a token and the code that resolves it cannot disagree about its shape.
	 */
	static FString MakeActionToken(FName ActionId);

	/**
	 *  ⭐⭐ THE DETAIL LANE'S HALF OF `HELP-§1`, AND IT IS THE SAME LAW AS THE ROW LANE.
	 *
	 *  Replaces every `{ActionId}` token in DetailText with that action's LIVE-DERIVED chip.
	 *  ⛔ NO LETTER IS EVER TYPED IN DETAIL PROSE: a sentence that must name a movable key names
	 *  the ACTION and this function names the KEY, exactly as ComposeKeyChipLabel does for a row.
	 *
	 *  ⚠️ Tokens are matched CASE-SENSITIVELY and only against real registry ids, so an
	 *  ordinary brace in prose is left alone and a MISSPELLED token survives VISIBLY (⛔ it is
	 *  never silently deleted — a visible gap gets fixed, `HELP-§2` mechanism 2; the suite
	 *  asserts that no shipped row leaves one behind).
	 *
	 *  ⛔ Two parameters, ⛔ neither defaulted (`SC-§33`).
	 *
	 *  @param DetailText   the prose, straight from ComposeDetailForDisplay.
	 *  @param ChipProvider answers "the chip for this ActionId" — the ONE place a key becomes
	 *                      characters, injected so the tests can drive it with no world.
	 */
	static FText ResolveDetailTokens(const FText& DetailText, TFunctionRef<FText(FName)> ChipProvider);

	/**
	 *  ⭐ THE WHOLE FULL-SCREEN PAGE, COMPOSED PURELY (TASK-707).
	 *
	 *  Chip + summary + token-resolved body + one block per RelatedActionIds row. Every key on
	 *  the finished page — the headline chip, the related chips, and every key named inside the
	 *  prose — has come through ResolveRowDisplayKeys with its own lane audit (`HELP-§1`).
	 *
	 *  ⛔ RELATED ROWS ARE RENDERED ONE LEVEL DEEP AND NEVER RECURSE; an unknown related id is
	 *  dropped rather than rendered blank.
	 *
	 *  ⛔ Three parameters, ⛔ none defaulted (`SC-§33`).
	 *
	 *  @param Row                the page's row.
	 *  @param LayoutSubsystem    the layout subsystem, or NULL (`KBD-§5`'s fail-safe: labels
	 *                            degrade to the reference keys, ⛔ never a crash).
	 *  @param AppliedKeyProvider answers QueryKeysMappedToAction for a row — ⚠️ ALREADY
	 *                            RETARGETED keys. Injected so the pure composer stays pure; the
	 *                            widget passes USiegeControlsHelpWidget::QueryAppliedKeysForRow
	 *                            and the tests pass a lambda.
	 */
	static FSiegeControlsDetailContent ComposeDetailContent(
		const FSiegeControlsHelpAction& Row,
		const USiegeKeyboardLayoutSubsystem* LayoutSubsystem,
		TFunctionRef<TArray<FKey>(const FSiegeControlsHelpAction&)> AppliedKeyProvider);
};

/**
 *  ═══ ONE ROW OF THE CONTROLS OVERLAY ═══  (TASK-706, `HELP-§3` pinned name)
 *
 *  Key chip · action name · one-line description, and the whole row is CLICKABLE (Jonathan's
 *  ask). ⛔ THIS CLASS DECIDES NOTHING: it reports its ActionId through OnRowActivated and the
 *  overlay decides what that means — the UDeckSlotEntryWidget contract, cloned.
 *
 *  ⛔ NO KEY HANDLING EXISTS IN THIS CLASS. In particular `Escape` stays permanently
 *  unabsorbed (`AS-§6 A-2`, `HELP-§5`).
 *
 *  CODE-AUTHORED under `HELP-§3` (`AS-§6` RULING A extended BY NAME to this feature, argued on
 *  its own facts — ⛔ it does not generalise). /Game/UI/WBP_ControlsHelpRow is RESERVED and
 *  UNAUTHORED; every child is BindWidgetOptional and constructed only while still null, so a
 *  later asset-authored tree using these exact names wins WHOLE with ZERO C++ change.
 *
 *  ⛔⛔ RebuildWidget() ORDER IS LOAD-BEARING: the tree is built and WidgetTree->RootWidget is
 *  set BEFORE `return Super::RebuildWidget();` — UserWidget.cpp:1214 returns an SSpacer on a
 *  null root, so anything built after Super is DISCARDED and the widget renders EMPTY while
 *  passing every property readback.
 *
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no new relevancy
 *  tier, no RPC. Client-local display only.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeControlsHelpRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Stamps this row's identity and its three strings. ORDER-INDEPENDENT [the BLOCKER
	 *  674-1 lesson, cloned]: everything is stored in members UNCONDITIONALLY and applied at
	 *  BOTH possible timings — immediately when the children already exist, and again from
	 *  RebuildWidget() when the tree comes alive after the stamp.
	 *
	 *  ⛔ Three parameters, ⛔ none defaulted (`SC-§33`).
	 */
	void SetRowContent(FName InActionId, const FText& InDisplayName, const FText& InOneLine, const FText& InKeyChip);

	/** This row's registry id, or NAME_None until stamped. */
	FName GetActionId() const { return ActionId; }

	/**
	 *  Fires OnRowActivated(ActionId). Public so the tests can drive the seam with no
	 *  viewport; also what the row's own button calls. A no-op while unstamped — an
	 *  un-identified row must never report a click as some other row's.
	 */
	void ActivateRow();

	/** ⭐ THE ROW HALF OF THE TASK-707 SEAM. Bound by the overlay at construction; ⛔ never bound anywhere else. */
	FOnSiegeControlsRowActivated OnRowActivated;

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** OnClicked thunk for RowButton (a dynamic delegate needs a UFUNCTION). Forwards to ActivateRow(). */
	UFUNCTION()
	void HandleRowButtonClicked();

	/** Builds the code-authored tree. Called from RebuildWidget() BEFORE Super — the order is load-bearing. */
	void ConstructRowTree();

	/** Writes the stored strings into whichever children exist. Idempotent; safe at every rebuild. */
	void ApplyStoredContent();

	// ------------------------------------------------------------------------
	// PINNED CHILDREN — all BindWidgetOptional, never BindWidget.
	// ------------------------------------------------------------------------

	/** The clickable surface and the tree root. The WHOLE row is the button (Jonathan's "clicking a row"). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RowButton;

	/** chip | text column. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> RowContentBox;

	/** The plate behind the key chip, so the key reads as a keycap and not as more prose. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> KeyChipBorder;

	/** ⭐ THE DERIVED KEY. Its text is composed by FSiegeControlsHelpRegistry::ComposeKeyChipLabel and ⛔ NEVER typed. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyChipText;

	/** name over description. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RowTextBox;

	/** "Ambush". */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionNameText;

	/** The one-liner, or the pinned TODO string — ⛔ never blank. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

private:

	/** NAME_None until SetRowContent stamps it. Fences ActivateRow so an unstamped row cannot mis-report. */
	FName ActionId;

	/**
	 *  The stamped strings, stored so a stamp landing BEFORE the tree exists survives until
	 *  ApplyStoredContent can write it (the BLOCKER 674-1 ordering hole).
	 *
	 *  ⚠️ These are per-OPEN values, ⛔ NOT a cache across opens: the overlay destroys and
	 *  rebuilds every row on every open (`HELP-§1` / `KBD-§0` ruling 2 — a mid-session
	 *  Win+Space must not leave a stale letter on screen).
	 */
	FText StoredDisplayName;
	FText StoredOneLine;
	FText StoredKeyChip;
};

/**
 *  ═══ THE FULL-SCREEN DETAIL VIEW ═══  (TASK-707, `HELP-§3` pinned name)
 *
 *  Jonathan's ask, verbatim, and it is the acceptance bar: clicking a row *"will show a much
 *  more detailed description on the entire screen of how it works and all the controls with it
 *  such as what the first, second, and third circles do, how to resize them, how to exit the
 *  command."*
 *
 *  HOW EACH CLAUSE OF THAT SENTENCE IS ANSWERED:
 *    • "on the entire screen" — this widget is the SECOND child of the overlay's ViewSwitcher
 *      and its switcher slot carries ZERO padding, while the list's carries the panel margin.
 *      ⇒ the list is a plate; this is edge to edge. A switcher (⛔ not two visibilities) makes
 *      "exactly one view is on screen" a STRUCTURAL property rather than a bug waiting to happen.
 *    • "a much more detailed description of how it works" — FSiegeControlsHelpAction::Detail,
 *      transferred from handoffs/TASK-704-programmer.md §4 where EVERY factual sentence is
 *      cited at a file:line somebody read. ⛔ Not re-authored here.
 *    • "and all the controls with it … the first, second, and third circles … how to resize
 *      them … how to exit the command" — FSiegeControlsHelpAction::RelatedActionIds. The three
 *      order pages render PickMode.Confirm (the three circles, in order), PickMode.Resize and
 *      PickMode.Cancel underneath their own prose, each with its LIVE-DERIVED chip.
 *
 *  ═══ 🚨🚨🚨 ⭐⭐ TASK-1484 [CONTROLS-HELP-DETAIL-SCROLL] — 🧑 HIS RULING, AND WHAT IT ADDED ═══
 *  🧑 He was offered *"count the screen, name the residual"* and chose the STRICTER reading of his
 *  own standing goal (*"the agent should do everything a human tester can"*): ⛔ **it is not
 *  navigable until an agent can SCROLL.** A human WHEEL-SCROLLS this page to read it — the author
 *  expects overflow (`AlwaysShowScrollbar` + `WhenScrollingPossible`) — and until this row there
 *  was ⛔ no keyboard/agent route to the prose below the fold ⛔ at all.
 *
 *  ⛔ THE MECHANISM THAT WAS BROKEN, MEASURED BY `qa/TASK-1479.md` AND ⛔ NOT RE-DERIVED HERE: the
 *  page presented ⛔ ONE stop (`BackButton`) ⇒ `MoveFocus` re-requested focus on the widget that
 *  ⛔ ALREADY HELD IT ⇒ `SetUserFocus` early-returned on *"focus has not changed"* ⇒
 *  `OnFocusChanging` ⛔ never fired ⇒ the `ScrollWidgetIntoView` hook had ⛔ no subject. ⛔ And
 *  `BackButton` is a ⛔ SIBLING of `DetailScrollBox`, ⛔ never a descendant, so focusing it could not
 *  drive the box either.
 *
 *  ⇒ ⛔ THE ANSWER IS A ⛔ SECOND STOP WHOSE ⛔ ACTIVATION PAGES THE BODY: `DetailScrollButton` →
 *  `AdvanceBodyScroll()`. ⛔ It is reachable on the ⛔ SHIPPED, PROVEN vocabulary — `IA_MenuDown`
 *  moves the ring onto it (a move that ⛔ now really changes focus, because there are ⛔ two stops),
 *  and `IA_MenuAccept` broadcasts its `OnClicked` — so ⛔ no new key, ⛔ no new binding and ⛔ no
 *  subsystem edit were needed. ⛔ The obvious "make the `UScrollBox` focusable" was ⛔ MEASURED both
 *  ⛔ FORBIDDEN (⭐ `TASK-1432` (2)) and ⛔ INEFFECTIVE (a `UScrollBox` is not one of
 *  `IsNavFocusStop`'s four admitted classes) ⇒ it would ⛔ break a rule to accomplish ⛔ zero.
 *
 *  🧑 ⛔ THE HUMAN PATH IS UNCHANGED BYTE-FOR-BYTE: the wheel still scrolls, the scrollbar still
 *  shows, `WhenScrollingPossible` / `AlwaysShowScrollbar` / `SetIsFocusable(false)` on the box are
 *  ⛔ untouched. ⛔ The ONLY visible delta is ⛔ one button below Back — ⛔ declared for 🧑 him rather
 *  than adjudicated here (`AS-§6 A(e)` / `HELP-§6`).
 *
 *  ⛔⛔ `Escape` IS UNTOUCHED HERE TOO (`AS-§6 A-2` — PERMANENT; `HELP-§5`). This class overrides
 *  NO key handler of any kind — ⛔ still true after TASK-1484, which added a ⛔ BUTTON and ⛔ not a
 *  key claim. It returns to the list on its own Back button; the OVERLAY closes
 *  on the toggle key or its Close button, and that is still the complete list.
 *
 *  ⛔ IT IS NOT A SECOND CURSOR OWNER. There is ⛔ no SetInputMode and ⛔ no bShowMouseCursor in
 *  this class — the overlay's ONE registration in ApplyCursorInputState() already covers both
 *  views (TASK-074's law). ⛔ And it does not pause: the battle keeps running behind it.
 *
 *  ⚠️ ITS PLATE IS `Visible` WHILE THE LIST'S BACKDROP IS `SelfHitTestInvisible`, and the
 *  difference is the geometry, not a change of mind: a plate the player is READING must absorb
 *  its own clicks or a click on a sentence places a card in the world underneath. The list
 *  panel already works exactly this way (TASK-706's PanelBorder); this plate is simply the size
 *  of the screen, because "the entire screen" is what was asked for.
 *
 *  CODE-AUTHORED under `HELP-§3`. /Game/UI/WBP_ControlsDetail is RESERVED and UNAUTHORED; every
 *  child is BindWidgetOptional and constructed only while still null, so a later asset-authored
 *  tree using these exact names wins WHOLE with ZERO C++ change.
 *
 *  ⛔⛔ RebuildWidget() ORDER IS LOAD-BEARING — see USiegeControlsHelpRowWidget's comment.
 *
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no new relevancy tier,
 *  no RPC. Client-local display only.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeControlsDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Stamps the whole page. ORDER-INDEPENDENT [the BLOCKER 674-1 lesson, cloned from the row
	 *  widget]: the content is stored UNCONDITIONALLY and applied at BOTH possible timings —
	 *  immediately when the children already exist, and again from RebuildWidget() when the tree
	 *  comes alive after the stamp.
	 *
	 *  ⛔ ONE parameter, ⛔ not defaulted (`SC-§33`).
	 */
	void SetDetailContent(const FSiegeControlsDetailContent& InContent);

	/** The row this page currently shows, or NAME_None until stamped. */
	FName GetDetailActionId() const { return Content.ActionId; }

	/**
	 *  Fires OnBackRequested. Public so the tests can drive the seam with no viewport; also what
	 *  the Back button calls.
	 *
	 *  ⭐⭐ ⛔ DELIBERATELY NOT FENCED ON A STAMP, unlike USiegeControlsHelpRowWidget::ActivateRow.
	 *  A row that does not know its identity must not report a click as some other row's — but a
	 *  detail page that does not know its identity STILL NEEDS A WAY OUT, and `Escape` may never
	 *  be that way out. A construction bug must never become a screen the player cannot leave.
	 */
	void RequestBack();

	/**
	 *  🚨🚨🚨 ⭐⭐ TASK-1484 [CONTROLS-HELP-DETAIL-SCROLL] — ⛔ THE PROSE BODY MOVES ⛔ ONE SCREENFUL,
	 *  AND THIS IS THE ⛔ WHOLE OF THE FEATURE 🧑 HE RULED BLOCKING: *"it isn't navigable until an
	 *  agent can scroll."*
	 *
	 *  Advances `DetailScrollBox` by `DetailScrollPageFraction` of its own VISIBLE height and
	 *  ⛔ WRAPS TO THE TOP once it is already at the end, so ⛔ every offset on a page of ⛔ any
	 *  length is reachable from ⛔ this one control. ⛔ Returns ⛔ TRUE IFF THE OFFSET ACTUALLY
	 *  CHANGED — ⛔ never "the handler ran".
	 *
	 *  ⭐⭐ ⛔ THE RETURN VALUE AND THE LOG LINE ARE ⛔ THE 5b INSTRUMENT, AND THEY ARE ⛔ BUILT TO
	 *  BE ABLE TO FAIL: the definition reads the offset ⛔ BEFORE and ⛔ AFTER the write and prints
	 *  ⛔ BOTH at `Log` (⛔ not `Verbose` — an absent line is a ZERO and a defect signature, and
	 *  `LogSiegeControlsHelp` is `(…, Log, All)`). ⛔ A page whose content FITS has
	 *  `GetScrollOffsetOfEnd() == 0`, so both numbers read `0.0` and this returns ⛔ false — ⛔ a
	 *  real negative, ⛔ not a silence. ⚖️ *An offset that moved is a claim about the SCREEN; a
	 *  handler that ran is only a claim about the code.*
	 *
	 *  ⛔ PUBLIC AND PARAMETERLESS (`SC-§33` is structurally unreachable): the ⛔ ONE caller today is
	 *  this page's own `DetailScrollButton` thunk, and it is public so a test or a later screen-side
	 *  caller can drive the seam with ⛔ no viewport — the `RequestBack()` precedent, cloned.
	 *
	 *  ⛔ IT TOUCHES ⛔ NOTHING ELSE: ⛔ no focus call, ⛔ no visibility write, ⛔ no registration,
	 *  ⛔ no input, ⛔ no `SetActiveWidgetIndex`, ⛔ no `Escape`, and ⛔ not one of the scroll box's
	 *  AUTHORED properties (`AlwaysShowScrollbar`, `WhenScrollingPossible` and `IsFocusable(false)`
	 *  are ⛔ read by nothing here and ⛔ written by nothing here) ⇒ 🧑 THE HUMAN WHEEL PATH IS
	 *  ⛔ BYTE-FOR-BYTE WHAT SHIPPED.
	 */
	bool AdvanceBodyScroll();

	/** ⭐ THE VIEW'S HALF OF THE BACK SEAM. Bound by the overlay; ⛔ nothing else may bind it. */
	FOnSiegeControlsDetailBackRequested OnBackRequested;

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** OnClicked thunk for BackButton (a dynamic delegate needs a UFUNCTION). Forwards to RequestBack(). */
	UFUNCTION()
	void HandleBackButtonClicked();

	/**
	 *  ⭐ TASK-1484 — OnClicked thunk for `DetailScrollButton`. Forwards to `AdvanceBodyScroll()`.
	 *  ⛔ A `UFUNCTION` for the same reason its neighbour is one: `UButton::OnClicked` is a DYNAMIC
	 *  multicast delegate and will not bind a plain member.
	 */
	UFUNCTION()
	void HandleScrollButtonClicked();

	/** Builds the code-authored tree. Called from RebuildWidget() BEFORE Super — the order is load-bearing. */
	void ConstructDetailTree();

	/** Writes the stored content into whichever children exist. Idempotent; safe at every rebuild. */
	void ApplyStoredContent();

	/** Rebuilds RelatedBox from Content.Related. ⛔ Destroys the previous blocks first — no page ever shows another page's controls. */
	void RebuildRelatedBlocks();

	// ------------------------------------------------------------------------
	// PINNED CHILDREN — all BindWidgetOptional, never BindWidget.
	// ------------------------------------------------------------------------

	/** The full-screen plate and the tree root. ⛔ Visible — it absorbs its own clicks (see the class comment). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DetailBackdrop;

	/** header / summary / scrolling body / back / scroll (⭐ TASK-1484 appended the last one, ⛔ last on purpose). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> DetailColumn;

	/** chip | title. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> DetailHeaderBox;

	/** The plate behind the page's key chip. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DetailKeyChipBorder;

	/** ⭐ THE PAGE'S DERIVED KEY. Composed by ComposeKeyChipLabel and ⛔ NEVER typed. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailKeyChipText;

	/** "Ambush". */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailTitleText;

	/** The row's one-liner. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailSummaryText;

	/** The scrolling region — the detail prose is long by design, and it must never be cut off. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> DetailScrollBox;

	/** The token-resolved detail prose. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailBodyText;

	/** "All the controls that go with it" — collapsed on a page with no related rows. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RelatedHeaderText;

	/** ⭐ Jonathan's "all the controls with it". RebuildRelatedBlocks() owns its children entirely. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RelatedBox;

	/** ⭐ The way back to the list — ⛔ and `Escape` is never it (`AS-§6 A-2`). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	/** The Back button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackLabelText;

	/**
	 *  🚨🚨 ⭐⭐ TASK-1484 — ⛔ THE PAGE'S ⛔ SECOND FOCUS STOP, AND THE ⛔ ONLY REASON IT EXISTS IS
	 *  THAT A ⛔ KEYBOARD/AGENT LANE ⛔ CANNOT REACH THE PROSE BELOW THE FOLD WITHOUT ONE.
	 *
	 *  ⛔ IT IS A `UButton` BECAUSE ⛔ THAT IS WHAT THE SHIPPED VOCABULARY CAN DRIVE, ⛔ not because a
	 *  button is the prettiest affordance: `USiegeMenuInputSubsystem::IsNavFocusStop` admits ⛔ four
	 *  classes (`UButton` / `UCheckBox` / `USlider` / `UEditableTextBox`) and `HandleMenuAccept`
	 *  gives ⛔ exactly one of them a press (`Focused->OnClicked.Broadcast()`). ⇒ ⛔ a `UScrollBox`
	 *  could never have been the answer (⭐ `TASK-1432` (2)'s standing ruling ⛔ AND the class list).
	 *
	 *  🚨 ⛔ IT IS THE ⛔ LAST CHILD OF `DetailColumn`, AND THE POSITION IS ⛔ LOAD-BEARING, ⛔ NOT
	 *  LAYOUT TASTE: `GetMenuFocusStops` returns depth-first ⛔ PRE-ORDER, so a stop added ⛔ after
	 *  `BackButton` leaves ⛔ STOP 0 = `BackButton` exactly where ⭐ `TASK-1478` declared it. ⛔ Put it
	 *  earlier and the ring would land somewhere else the moment a detail page opens, and that
	 *  sentence is ⛔ already written into three comments and one runtime expectation.
	 *
	 *  🚨🚨 ⛔ ITS NAME AND ITS LABEL ARE ⛔ BOTH SAFETY PROPERTIES — ⛔ DO NOT "TIDY" EITHER.
	 *  `DetailColumn` now holds ⛔ EXACTLY TWO `UButton`s, which ⛔ PASSES the count guard in
	 *  `USiegeMenuInputSubsystem::FindStepperPair`. ⛔ The only things that keep this pair from being
	 *  read as a STEPPER are its ⛔ two discriminators: (i) neither name ends in `PrevButton` or
	 *  `NextButton` with a shared base, and (ii) neither label is the bare glyph `<` or `>`.
	 *  ⛔ BREAK EITHER AND TWO THINGS HAPPEN AT ONCE, ⛔ both silent: `IsNavFocusStop` ⛔ DROPS the
	 *  `Next` member from the ring, and `StepFocusedStop` makes `IA_MenuLeft`/`IA_MenuRight` ⛔ PRESS
	 *  the partner — i.e. an arrow key would ⛔ LEAVE THE PAGE. (`USettingsMenuWidget`'s footer is the
	 *  shipped precedent for a two-button parent that is correctly refused.)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DetailScrollButton;

	/** ⭐ TASK-1484 — the scroll control's content text. ⛔ It names a DIRECTION, ⛔ never a key (`HELP-§1`/`HELP-§4`). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailScrollLabelText;

private:

	/**
	 *  The stamped page, stored so a stamp landing BEFORE the tree exists survives until
	 *  ApplyStoredContent can write it (the BLOCKER 674-1 ordering hole).
	 *
	 *  ⚠️ A PER-CLICK VALUE, ⛔ NOT A CACHE ACROSS OPENS: every chip in it was derived at the
	 *  moment of the click, and the overlay re-composes the page on every activation
	 *  (`HELP-§1` / `KBD-§0` ruling 2 — a mid-session Win+Space must not leave a stale letter).
	 *  ⛔ Plain C++ members: FText/FName only, so there is nothing here for the GC to keep and
	 *  ⛔ no FKey anywhere (the suite's reflection sweep asserts that half).
	 */
	FSiegeControlsDetailContent Content;
};

/**
 *  ═══ THE TAB CONTROLS OVERLAY ═══  (TASK-706, `HELP-§3` pinned name)
 *
 *  A scrolling list of every keyed and clickable action, each row carrying the key AS BOUND
 *  FOR THE PLAYER'S ACTUAL LAYOUT plus a one-line description. Rows are clickable and hand off
 *  to TASK-707's full-screen detail view.
 *
 *  ---------------------------------------------------------------------------
 *  §1 ⛔⛔ THE CENTRAL LAW — THE KEY LABELS ARE DERIVED LIVE (`HELP-§1`)
 *  ---------------------------------------------------------------------------
 *  ⛔ NO LETTER IN THIS FEATURE IS EVER TYPED AS A STRING LITERAL. Every chip's characters
 *  come from FKey::GetDisplayName on an FKey produced by
 *  FSiegeControlsHelpRegistry::ResolveRowDisplayKeys, whose four lanes are pinned on
 *  ESiegeInputLane. ⭐ On the Lane-A primary path there are ZERO GetPositionalKey calls,
 *  because QueryKeysMappedToAction already returns the RETARGETED key — that is the whole
 *  double-translate audit, and it is closed STRUCTURALLY rather than by care.
 *
 *  ⛔ LABELS RE-DERIVE ON EVERY OPEN AND NO KEY STRING IS CACHED ACROSS OPENS
 *  (`KBD-§0` ruling 2 puts mid-session Win+Space switches in scope; the poll is 1 Hz).
 *  OpenHelp() calls RefreshKeyboardLayout() ONCE — the caller refreshes, the accessor reads
 *  (`KBD-§8`; USiegeAssistantConsoleWidget::OpenConsole's shipped precedent) — and then
 *  DESTROYS AND REBUILDS every row.
 *  ⛔ This widget does NOT bind OnKeyboardLayoutChanged: `KBD-§8` forbids it for a widget
 *  (new lifetime state to unbind wrongly, for a value re-read at every open anyway).
 *
 *  ⛔ READ-ONLY ON THE LAYOUT SYSTEM: no IMC_Hero write, ⛔ no MapKey / UnmapKey / UnmapAll
 *  anywhere in this file. A help screen that edits bindings is an automatic FAIL (`KBD-§1`/`§2`).
 *
 *  ---------------------------------------------------------------------------
 *  §2 ⛔⛔ `Escape` IS NEVER TOUCHED (`AS-§6 A-2`, `HELP-§5` — a PERMANENT Jonathan ruling)
 *  ---------------------------------------------------------------------------
 *  ⛔⛔ CORRECTED 2026-09-25 BY TASK-1432 (`SC-§53` — the repair lands and the paragraph
 *  describing the pre-repair state must NOT be left behind). ⛔ THE PREMISE BELOW CHANGED;
 *  ⭐ THE CONCLUSION DID NOT, AND THE CHANGE IS WHAT KEEPS THE CONCLUSION TRUE.
 *
 *  ~~"ALL THREE classes … override NO key handler at all: no NativeOnKeyDown, no
 *  NativeOnPreviewKeyDown, …"~~ ⇒ ⛔ NO LONGER TRUE OF THIS CLASS. `USiegeControlsHelpWidget`
 *  now overrides ⛔ EXACTLY ONE handler — `NativeOnPreviewKeyDown` — and it answers to
 *  ⛔ EXACTLY ONE key: the DERIVED `IA_ControlsHelp` toggle this screen already owned.
 *  ✅ USiegeControlsHelpRowWidget and USiegeControlsDetailWidget still override NONE.
 *  ✅ There is still no NativeOnKeyDown anywhere, no Enhanced Input action of this widget's
 *  own, and no viewport intercept, in any of the three.
 *
 *  ⛔ WHY IT HAD TO EXIST, AND WHY IT IS NOT A WIDENING: TASK-1432 makes `CloseButton` a
 *  keyboard focus stop so the overlay can be closed without the mouse. MEASURED at engine
 *  source, the moment ANY widget here holds Slate focus, `Tab` is consumed as "focus next"
 *  (`SWidget::OnKeyDown` → `FReply::Handled().SetNavigation`, with `bTabNavigation` true by
 *  default) and ⛔ never reaches `SViewport` → Enhanced Input → IA_ControlsHelp — there is no
 *  game-viewport fallback for an unhandled key. ⇒ WITHOUT this handler the toggle would have
 *  ⛔ SILENTLY STOPPED CLOSING THE OVERLAY and the close list would have HALVED.
 *  ⛔ The handler refuses `Escape` FIRST and unconditionally, and it derives its key rather
 *  than naming one (`HELP-§1`/`HELP-§4`); it binds, maps and remaps NOTHING (`KBD-§1`/`§2`).
 *
 *  The token `Escape` appears in this file ONLY as documentation prose (the two cancel rows'
 *  data and their detail text) and as that ONE refusal guard — ⛔ never as a claim on the key.
 *  ⇒ THE OVERLAY CLOSES ON TAB (the toggle) AND ON ITS OWN CLOSE BUTTON. That is the
 *  COMPLETE list, and it did NOT grow when the detail view landed: that view returns to the
 *  LIST on its own Back button, which is a different thing from closing (TASK-707).
 *  ⛔ Nor did it grow with TASK-1432: BOTH shipped routes are preserved and the keyboard now
 *  reaches both — `Enter`/`Space` on the focused Close button (Slate's own Accept path), and
 *  the toggle key through the handler above.
 *  ⭐ AND IT IS WHY THAT CONTROL IS A BUTTON: the only key a "go back" affordance would
 *  naturally have claimed is `Escape`, and `Escape` is permanently unavailable.
 *  Every shipped cancel route — placement, spell targeting, the group-order pick — keeps
 *  firing byte-identically while the overlay is open.
 *
 *  ---------------------------------------------------------------------------
 *  §3 ⛔ CURSOR OWNERSHIP GOES THROUGH ApplyCursorInputState() AND NOWHERE ELSE
 *  ---------------------------------------------------------------------------
 *  This widget contains ⛔ NO SetInputMode and ⛔ NO bShowMouseCursor write. It shows and
 *  hides itself; ASiegePlayerController::SetControlsHelpOpen owns the posture and is the
 *  SIXTH term in that one composition (SiegePlayerController.cpp:6457 — ⛔ re-measured by
 *  TASK-1432 under `SC-§53`/`HELP-§2`'s citation-rot bullet; the cited line had drifted to
 *  :4357, which today lands in an unrelated formation comment. ⚠️ A LINE NUMBER IS A CITATION
 *  THAT ROTS: the anchor that does not is the expression itself, `const bool bWantCursor = …`).
 *  A direct posture call
 *  from a widget is the defect that booted the arena input-dead and cost a playtest (TASK-074).
 *  OnHelpOpenChanged exists so a Close-BUTTON close — a route the controller never sees —
 *  still releases the posture (the UWarMapWidget::OnMapOpenChanged precedent).
 *
 *  ---------------------------------------------------------------------------
 *  §4 ⚖️ IT DOES NOT PAUSE, AND IT IS READ-ONLY ON THE WORLD (`HELP-§5`)
 *  ---------------------------------------------------------------------------
 *  Single-player has no pause today and the bot keeps marching; a pause would be a new
 *  mechanic, not a side effect of a help screen. Opening issues no order, cancels no group,
 *  plays no card and moves no gold — grep this file for a single mutating controller call and
 *  there is none.
 *
 *  ⇒ THAT IS WHY BackdropBorder IS SelfHitTestInvisible AND PanelBorder IS Visible, AND IT IS
 *    CORRECTNESS, NOT STYLING. The battlefield stays live underneath, so a full-screen
 *    click-absorbing plate would silently eat the very cancel clicks §2 promises keep working.
 *    The PANEL absorbs its own clicks (so reading the list never places a card); everything
 *    outside it falls through to the game exactly as before.
 *    ⚠️ This is the OPPOSITE of UAccountMenuWidget's Visible backdrop, deliberately: that
 *    panel overlays a MENU whose buttons must not be click-through; this one overlays a LIVE
 *    MATCH that must not be frozen.
 *
 *  ---------------------------------------------------------------------------
 *  §5 CODE-AUTHORED (`HELP-§3`, `AS-§6` RULING A extended BY NAME)
 *  ---------------------------------------------------------------------------
 *  /Game/UI/WBP_ControlsHelp, /Game/UI/WBP_ControlsHelpRow and /Game/UI/WBP_ControlsDetail are
 *  all RESERVED and UNAUTHORED. Every child is BindWidgetOptional and constructed only while
 *  still null, and ALL THREE class slots are SOFT with a null-safe fallback to these C++
 *  classes — so a later asset-authored tree using these exact names wins WHOLE with ZERO C++
 *  change. ⛔ The ruling is scoped to this feature and does NOT generalise (`AS-§6` A(a)'s
 *  scope clause stands).
 *
 *  ⛔⛔ RebuildWidget() ORDER IS LOAD-BEARING — see the row class's comment.
 *  ⛔ VERIFICATION CLOSES ON PIXELS OR JONATHAN'S EYES, ⛔ NEVER ON A PROPERTY READBACK
 *  (`AS-§6` A(e); MCP readback has repeatedly passed on visually-broken UMG here).
 *
 *  ---------------------------------------------------------------------------
 *  §6 ⛔ AN UNRESOLVABLE ACTION DOES NOT HIDE ITS ROW (`HELP-§2` mechanism 2)
 *  ---------------------------------------------------------------------------
 *  A missing IA_* asset ⇒ the row still renders, with the explicit "not bound" chip. Missing
 *  text ⇒ the explicit "(undocumented — TODO)" string. ⚖️ A VISIBLE GAP GETS FIXED; A SILENT
 *  OMISSION DOES NOT.
 *
 *  ---------------------------------------------------------------------------
 *  §7 ⭐ THE ROW-CLICK -> DETAIL SEAM — TASK-706 BUILT IT, TASK-707 FILLED IT
 *  ---------------------------------------------------------------------------
 *    (a) USiegeControlsHelpRowWidget::OnRowActivated (non-dynamic, one FName) fires on a
 *        row click. The overlay binds it at row construction; ⛔ nothing else may bind it.
 *    (b) It lands in HandleRowActivated(FName), which validates the id against the registry,
 *        stores it in SelectedActionId, calls ShowDetailForAction(ActionId), then broadcasts
 *        OnRowSelected. ⛔ Nothing else routes a row click.
 *    (c) ✅ FILLED (TASK-707). ShowDetailForAction(FName) composes the page through
 *        FSiegeControlsHelpRegistry::ComposeDetailContent, stamps DetailView and switches
 *        ViewSwitcher to the detail index; ReturnToList() switches it back. Both are
 *        idempotent and null-safe, and CloseHelp() already calls ReturnToList() first — so
 *        every open starts on the list and ⛔ no new close path was invented.
 *    (d) ✅ The prose comes through ComposeDetailForDisplay — ⛔ never Row.Detail raw, which
 *        would skip the pinned TODO string and render a blank page.
 *    (e) ⛔ The detail view is NOT a second cursor owner: this overlay's single registration
 *        in ApplyCursorInputState() covers BOTH views, and TASK-707 added no posture code.
 *    (f) ⛔ `Escape` is untouched in the detail lane too — the page returns to the list on its
 *        own Back button, and the OVERLAY still closes only on the toggle key or Close.
 *    (g) ⭐ EVERY KEY THE DETAIL PAGE NAMES DERIVES THROUGH ResolveRowDisplayKeys WITH THE
 *        SAME LANE AUDIT — in TWO places: the related blocks (PickMode.Confirm / Resize /
 *        Cancel carry LMB, the wheel and the exit keys as real chips) AND the prose itself,
 *        where a movable key is written as a `{ActionId}` token and spliced by
 *        ResolveDetailTokens. ⛔ A hardcoded letter in the detail lane is the identical
 *        failure the row lane forbids.
 *
 *  ---------------------------------------------------------------------------
 *  M8 DECLARATION (verbatim): adds no replicated property, no new replicated class, no new
 *  relevancy tier, no RPC. The overlay is CLIENT-LOCAL DISPLAY ONLY — it reads the local
 *  player's input mappings and the local keyboard layout and writes nothing anywhere.
 *  ---------------------------------------------------------------------------
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeControlsHelpWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Creates the overlay for OwningController and adds it to the viewport, CLOSED — so this
	 *  call alone puts nothing on screen and makes nothing hit-testable.
	 *
	 *  ⛔ THREE PARAMETERS, ⛔ NONE DEFAULTED (`SC-§33`, structurally immune). A null
	 *  HelpClass falls back to this C++ class, which is the shipping state today:
	 *  /Game/UI/WBP_ControlsHelp is RESERVED and UNAUTHORED.
	 *
	 *  @param OwningController the local player controller. Null ⇒ no overlay, logged, ⛔ never fatal.
	 *  @param HelpClass        /Game/UI/WBP_ControlsHelp once it exists; null ⇒ this class.
	 *  @param ZOrder           viewport Z order.
	 */
	static USiegeControlsHelpWidget* CreateAndAddToViewport(
		APlayerController* OwningController,
		TSubclassOf<USiegeControlsHelpWidget> HelpClass,
		int32 ZOrder);

	/**
	 *  Shows the overlay and RE-DERIVES EVERY LABEL (`HELP-§1`): RefreshKeyboardLayout() once,
	 *  then every row is destroyed and rebuilt. ⛔ It sets no input mode and shows no cursor —
	 *  ASiegePlayerController::SetControlsHelpOpen owns the posture (§3).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|ControlsHelp")
	void OpenHelp();

	/** Hides the overlay and returns to the list view. ⛔ Never refused — a close that can fail can strand the cursor. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|ControlsHelp")
	void CloseHelp();

	UFUNCTION(BlueprintPure, Category = "Siegebound|ControlsHelp")
	bool IsHelpOpen() const { return bHelpOpen; }

	/** The id of the last row the player activated, or NAME_None. The detail view renders it. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|ControlsHelp")
	FName GetSelectedActionId() const { return SelectedActionId; }

	/**
	 *  True while the full-screen detail page is the visible view (TASK-707).
	 *  ⛔ Asked of the SWITCHER, ⛔ never of a second bool: one source of truth for which view
	 *  is up means a bool and a screen can never disagree.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|ControlsHelp")
	bool IsDetailViewActive() const;

	/** ⭐ THE OVERLAY HALF OF THE TASK-707 SEAM (§7b). Broadcast AFTER ShowDetailForAction returns. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|ControlsHelp")
	FOnSiegeControlsRowSelected OnRowSelected;

	/** Broadcast on every REAL open-state change. The controller binds it to release the cursor posture on a Close-button close (§3). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|ControlsHelp")
	FOnSiegeControlsHelpOpenChanged OnHelpOpenChanged;

	/**
	 *  Rebuilds the whole list from the registry with FRESHLY DERIVED key labels.
	 *  ⛔ Every existing row widget is destroyed first — that is what structurally guarantees
	 *  "no key string cached across opens" (`KBD-§0` ruling 2) rather than promising it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|ControlsHelp")
	void RefreshRows();

	/**
	 *  ⭐ THE LIVE LANE-A QUERY, ISOLATED SO THE PURE PART STAYS PURE.
	 *
	 *  Returns what UEnhancedInputSubsystemInterface::QueryKeysMappedToAction reports for this
	 *  row's actions in the ACTIVE contexts — i.e. ⚠️ THE ALREADY-RETARGETED KEYS
	 *  (EnhancedInputSubsystemInterface.h:381-384; HeroCharacter.cpp:271-275).
	 *  ⛔ IT APPLIES NO TRANSLATION OF ITS OWN AND CALLS GetPositionalKey ZERO TIMES.
	 *
	 *  Empty for every non-Lane-A row, for an unresolvable asset, and for a Lane-A action that
	 *  no active context maps — which is exactly when ResolveRowDisplayKeys takes its single
	 *  translation fallback.
	 *
	 *  ⛔ Two parameters, ⛔ neither defaulted (`SC-§33`).
	 */
	static TArray<FKey> QueryAppliedKeysForRow(const FSiegeControlsHelpAction& Row, const APlayerController* OwningController);

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 *  ⭐⭐ TASK-1432 — THE ONE KEY THIS SCREEN ANSWERS, AND IT IS THE KEY IT ⛔ ALREADY OWNED.
	 *
	 *  🚨 WHY IT HAD TO EXIST THE MOMENT `CloseButton` BECAME A FOCUS STOP — ⛔ MEASURED AT
	 *  ENGINE SOURCE, ⛔ NOT REASONED FROM THE DOCS:
	 *    • `FNavigationConfig::GetNavigationDirectionFromKey` maps ⛔ `Tab` → `EUINavigation::Next`
	 *      whenever `bTabNavigation` is set, which the default ctor sets ⛔ true
	 *      (`Slate/Private/Framework/Application/NavigationConfig.cpp:9`, `:68-77`). This project
	 *      installs ⛔ no custom navigation config (census: 0 `SetNavigationConfig` calls).
	 *    • `SButton::OnKeyDown` forwards every non-Accept key to `SBorder`→`SWidget::OnKeyDown`
	 *      (`Slate/Private/Widgets/Input/SButton.cpp:293-320`), and `SWidget::OnKeyDown` returns
	 *      ⛔ `FReply::Handled().SetNavigation(...)` for any navigation key when the widget
	 *      `SupportsKeyboardFocus()` (`SlateCore/Private/Widgets/SWidget.cpp:416-429`).
	 *    • `FSlateApplication::ProcessKeyDownEvent` has ⛔ NO game-viewport fallback: an unhandled
	 *      key goes to `UnhandledKeyDownEventHandler`, which is bound ⛔ only by the editor's main
	 *      frame (`SlateApplication.cpp:5065-5069`; sole binder `MainFrameActions.cpp:97`).
	 *  ⇒ ⛔ **ONCE ANYTHING IN THIS OVERLAY HOLDS SLATE FOCUS, `Tab` IS EATEN AS "FOCUS NEXT" AND
	 *  ⛔ NEVER REACHES `SViewport` → Enhanced Input → `IA_ControlsHelp`.** Without this override
	 *  the row would have shipped a screen that ⛔ silently lost one of its ⛔ TWO ways out.
	 *
	 *  ⛔ THAT IS A LAW-LEVEL BREAK, NOT A NICETY: `HELP-§5` states the close list is "the toggle
	 *  key and/or its own on-screen Close button — that is the complete list", and the shipped
	 *  R-24 detail prose tells the player so in as many words (`SiegeControlsHelpWidget.cpp`, the
	 *  `Interface.ControlsHelp` row: *"It closes on {Interface.ControlsHelp} or its own Close
	 *  button, and that is the complete list"*). ⛔ Halving the list would have made ⛔ shipped
	 *  player-facing prose FALSE with every gate green — ⛔ `HELP-§2` mechanism 4's exact failure
	 *  mode, on the one screen that can contradict the game to the player's face.
	 *
	 *  ⭐ PREVIEW (TUNNEL), ⛔ NOT `NativeOnKeyDown` (BUBBLE), AND THE CHOICE IS FORCED: the bubble
	 *  starts at the ⛔ focused leaf, so `CloseButton`'s `SButton` would consume `Tab` before any
	 *  ancestor saw it. The tunnel runs ⛔ root→leaf along the same focus path
	 *  (`SlateApplication.cpp:5024-5041`), and `SObjectWidget::OnPreviewKeyDown` forwards here
	 *  unconditionally (`UMG/Private/Slate/SObjectWidget.cpp:221-229`).
	 *
	 *  ⛔⛔ `Escape` IS REFUSED FIRST AND UNCONDITIONALLY. `HELP-§5` is a CLOSED 🧑 Jonathan
	 *  ruling that names ⛔ `NativeOnPreviewKeyDown` ⛔ by name as a forbidden route for it.
	 *
	 *  ⛔ NOTHING IS BOUND, MAPPED OR REMAPPED HERE (`KBD-§1`/`§2`, `HELP-§1`'s read-only clause):
	 *  this is a ⛔ READ of `QueryKeysMappedToAction` through the row registry — the ⛔ same
	 *  accessor and the ⛔ same row the key chip and the hint line already read. ⛔ No key is
	 *  typed, no context is mutated, no `MapKey`/`UnmapKey` exists on this path.
	 *
	 *  ⛔ NO DOUBLE-FIRE IS POSSIBLE, BY CONSTRUCTION: this runs only when the overlay is in the
	 *  focus path, and in exactly that state the viewport is ⛔ not, so Enhanced Input's
	 *  `IA_ControlsHelp` binding cannot also fire. The two lanes are ⛔ mutually exclusive.
	 *
	 *  🚨 ⭐ TASK-1478 — ⛔ THE FOCUSABLE SET GREW FROM ⛔ ONE BUTTON TO ⛔ `RowWidgets.Num()` + 2,
	 *  AND THIS HANDLER IS ⛔ UNCHANGED — ⛔ a re-measurement, ⛔ not an omission. The sentence in
	 *  bold above already says *"once ⛔ ANYTHING in this overlay holds Slate focus"*, and the cover
	 *  is a property of the ⛔ TREE'S SHAPE rather than of which button is focusable: the preview
	 *  phase runs along the focused widget's ⛔ ANCESTOR CHAIN, and ⛔ every `RowButton` and
	 *  `BackButton` is a ⛔ DESCENDANT of this overlay ⇒ this widget is on that path ⛔ for all of
	 *  them, ⛔ by construction. ⛔ The full re-derivation, including the new mouse-click-focuses-a-row
	 *  case, is at the function's definition in the `.cpp`.
	 */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End UUserWidget interface

	/**
	 *  ⭐ SHOWS THE FULL-SCREEN DETAIL PAGE FOR ONE ROW (§7c, TASK-707).
	 *
	 *  Composes the page purely (ComposeDetailContent), stamps DetailView, and switches the
	 *  view. ⛔ It sets NO input mode, shows NO cursor and is NOT a second cursor owner — the
	 *  overlay's ONE registration in ApplyCursorInputState() covers both views (TASK-074's law).
	 *  ⛔ It claims no key. An unknown id, a missing switcher or a missing view all leave the
	 *  LIST on screen and log — ⛔ never a blank page and ⛔ never a screen with no way out.
	 *
	 *  ⭐ TASK-1432 QA LOOP 1 — ⛔ IT ALSO ~~⛔ GIVES THE MENU RING BACK~~, and that is not a detail:
	 *  this screen's only focus stop (`CloseButton`) lives in the switcher's ⛔ LIST slot, and a
	 *  `SWidgetSwitcher` refuses focus to its inactive slot while ⛔ leaving every visibility flag
	 *  alone — so a registration held across this call would report ⛔ 1 stop that ⛔ CANNOT take
	 *  focus and would ⛔ SWALLOW the ring. Guarded on a real list→detail edge and on `bHelpOpen`.
	 *  ⛔ The measurement and the three rejected alternatives are at the call site.
	 *
	 *  🚨 ⭐ TASK-1478 — ⛔ IT NOW ⛔ MOVES THE RING INSTEAD OF ⛔ SURRENDERING IT (`SC-§120`: the
	 *  paragraph above is kept because its ⛔ measurement of the switcher is ⛔ still exactly right —
	 *  what expired is ⛔ which remedies were available). ⛔ `BackButton` is now focusable and the
	 *  walker now ⛔ descends into `DetailView`'s tree, so the detail page ⛔ HAS a ring of its own:
	 *  ~~⛔ `{ BackButton }`, ⛔ exactly one, ⛔ by the structure of `ConstructDetailTree`.~~
	 *  ⭐⭐ TASK-1484 — ⛔ **`{ BackButton, DetailScrollButton }`, ⛔ exactly TWO**, ⛔ still by the
	 *  STRUCTURE of `ConstructDetailTree` rather than by a tally (it builds ⛔ those two `UButton`s
	 *  and ⛔ no other admitted-class widget, ⛔ however many related blocks a page grows).
	 *  ⛔ `BackButton` is ⛔ STILL STOP 0: the new stop is the ⛔ LAST child of `DetailColumn` and the
	 *  walker is depth-first ⛔ PRE-ORDER ⇒ ⛔ every sentence in this file about WHERE the ring lands
	 *  on a detail page is ⛔ unchanged. ⛔ This
	 *  function therefore ⛔ RE-REGISTERS on the list→detail edge, and `RegisterMenuNavTarget`'s
	 *  closing `FocusFirstNavStop()` ⛔ re-places the outline onto that button. ⛔ The inactive list
	 *  branch is kept out of the ring by `ApplyActiveView`'s collapse, ⛔ not by an absent
	 *  registration. ⇒ ⛔ **registered ⟺ `bHelpOpen`**, and this is ⛔ no longer a registration edge
	 *  at all — it is a ⛔ REFRESH.
	 */
	virtual void ShowDetailForAction(FName InActionId);

	/**
	 *  ⭐ THE DETAIL VIEW'S "BACK TO THE LIST" (§7c, TASK-707). Idempotent and null-safe, which
	 *  is why CloseHelp() can call it unconditionally on EVERY close — including a close that
	 *  happened while the list was already up.
	 *
	 *  ⭐ TASK-1432 QA LOOP 1 — ⛔ IT ALSO ⛔ TAKES THE MENU RING BACK, but ⛔ ONLY on a ⛔ REAL
	 *  detail→list edge ⛔ AND only while `bHelpOpen`: this function is called unconditionally by
	 *  BOTH `OpenHelp()` (⛔ before the overlay is visible) and `CloseHelp()` (⛔ before it closes),
	 *  and neither of those may register anything. ⛔ A list→list call registers ⛔ nothing.
	 *  ⚠️ ⛔ ONE TRANSIENT `registered`→`unregistered` PAIR IS EXPECTED IN THE LOG on exactly one
	 *  route — closing ⛔ from the detail page — and it is ⛔ declared at the call site so 5b does
	 *  not read it as churn. ⛔ Both lines are true of states the program really passes through.
	 *
	 *  🚨 ⭐ TASK-1478 — ⛔ THE STOP IT LANDS ON MOVED, and it is declared here so 5b does not have
	 *  to discover it: the list's stop 0 is now the ⛔ FIRST `RowButton`, ⛔ not `CloseButton`,
	 *  because `RowScrollBox` precedes `CloseButton` in `RootPanel`'s slot order and the walker
	 *  returns depth-first ⛔ PRE-ORDER. ⇒ ⛔ leaving a detail page puts the ring at the ⛔ TOP OF THE
	 *  LIST rather than on the row the player came from. ⚖️ ⛔ Not fixed here, ⛔ on purpose: it
	 *  would need per-screen focus memory plus a direct focus call, and `RegisterAsMenuNavTarget`'s
	 *  own law forbids the second. ⛔ A usability note for 🧑 him, ⛔ not a severed ring.
	 */
	virtual void ReturnToList();

	/**
	 *  🚨🚨🚨 ⭐ TASK-1478 [CONTROLS-HELP-NAVIGABLE] — ⛔ THE ⛔ ONE WRITER of which view is up, and
	 *  ⛔ THE ANSWER TO THE `SWidgetSwitcher` HAZARD THIS ROW'S TWO FOCUS FLIPS OPEN.
	 *
	 *  ⛔ IT DOES ⛔ THREE THINGS, ⛔ ALWAYS TOGETHER, ⛔ FROM ⛔ ONE PARAMETER: sets `PanelBorder`'s
	 *  visibility, sets `DetailView`'s visibility, and sets `ViewSwitcher`'s active index. ⛔ The
	 *  ⛔ INACTIVE branch is ⛔ COLLAPSED, which is what makes `HasVisibleSlateAncestry` — the
	 *  ⛔ SHIPPED predicate, ⛔ unmodified — drop that branch's focusable `UButton`s from the ring.
	 *  ⛔ Without it, a focusable button in an inactive switcher slot is ⛔ ADMITTED by the walker and
	 *  ⛔ REFUSES focus (`SWidgetSwitcher::ValidatePathToChild`), and such a stop ⛔ SWALLOWS THE RING.
	 *
	 *  ⛔ IT IS A ⛔ PURE VIEW-STATE WRITER: ⛔ no registration, ⛔ no focus call, ⛔ no input, ⛔ no
	 *  broadcast — which is what makes it safe to call from `ConstructHelpTree`, where registering
	 *  anything would be the ⛔ never-arms defect `ApplyOpenState`'s block warns about.
	 *
	 *  ⛔ THE ACTIVE BRANCH IS RESTORED TO ITS ⛔ AUTHORED value (`PanelBorder` → `Visible`, which is
	 *  ⛔ hit-test correctness rather than styling; `DetailView` → `Visible`, its untouched `UWidget`
	 *  default), so ⛔ every frame the player actually sees is ⛔ byte-identical to what shipped.
	 *  ⛔ Collapsing the inactive branch is ⛔ visually inert — the switcher already declined to
	 *  arrange, render and hit-test it; ⛔ the only thing that was untrue of that branch was its
	 *  visibility ⛔ ATTRIBUTE, which is ⛔ exactly what the predicate reads.
	 *
	 *  ⛔ THE FULL ARGUMENT — ⛔ the four shapes measured, ⛔ why registering `DetailView` instead is
	 *  ⛔ MEASURED INERT (`UWidget::IsInViewport()`), and ⛔ how ⭐ `TASK-1432`'s REJECTED (b) two
	 *  objections are answered ⛔ mechanically — lives at the definition in the `.cpp`. ⛔ Read it
	 *  before adding a third branch, a fourth caller, or any other `SetActiveWidgetIndex` write.
	 */
	void ApplyActiveView(int32 InViewIndex);

	/** The single row-click entry point (§7b). ⛔ Nothing else routes a row click. */
	void HandleRowActivated(FName InActionId);

	/** OnClicked thunk for CloseButton. Forwards to CloseHelp(). */
	UFUNCTION()
	void HandleCloseButtonClicked();

	/** The detail view's Back seam lands here. ⛔ Returns to the list; it does NOT close the overlay. */
	void HandleDetailBackRequested();

	/** Builds the code-authored tree. Called from RebuildWidget() BEFORE Super — the order is load-bearing. */
	void ConstructHelpTree();

	/** Applies bHelpOpen to the tree's visibility and broadcasts OnHelpOpenChanged on a REAL change only. */
	void ApplyOpenState(bool bOpen);

	/** Destroys every row and category header currently in RowScrollBox. */
	void ClearRowWidgets();

	/** Null-safe resolve of the layout subsystem through this widget's game instance. Null ⇒ the `KBD-§5` fail-safe. */
	USiegeKeyboardLayoutSubsystem* ResolveKeyboardLayoutSubsystem() const;

	// ═══════════════════════════════════════════════════════════════════════════════════════
	//  ⭐ TASK-1432 [MENU-NAV-CONTROLS-HELP] — THE SCREEN ANSWERS THE KEYBOARD.
	//
	//  🚨🚨 THE ONE THING THAT WOULD MAKE THIS SCREEN SILENTLY INERT, WRITTEN WHERE THE NEXT
	//  EDITOR WILL SEE IT: ⛔ THESE ARE CALLED FROM `ApplyOpenState` (⛔ and, since TASK-1432 QA
	//  loop 1, from the two ⛔ VIEW-SWITCH edges as well — see `UnregisterAsMenuNavTarget`),
	//  ⛔ NEVER FROM `NativeConstruct`. This overlay is added to the viewport ⛔ CLOSED
	//  (`CreateAndAddToViewport` → `AddToViewport` → `NativeConstruct` collapses it), and the
	//  subsystem's in-match arming backstop is ⛔ DISARM-ONLY with ⛔ NO RISING EDGE ON *SHOW*
	//  (`qa/TASK-1430.md` WARN-2). `GetRegisteredNavTarget()` additionally requires
	//  `IsVisible()`. ⇒ ⛔ A REGISTRATION TAKEN AT CONSTRUCTION WHILE COLLAPSED ⛔ NEVER ARMS,
	//  and the screen would compile, review clean and ⛔ do nothing (`SC-§36.1`).
	//
	//  ⚠️ THIS ⛔ INVERTS THE FIVE MAIN-MENU SCREENS (TASK-1415/1417/1419/1425), which all
	//  register in `NativeConstruct`. ⛔ They are layered over a menu that is already up; ⛔ this
	//  one is created closed inside a live match. ⛔ Do not copy their shape here.
	//
	//  ⛔ WHAT IS DELIBERATELY ABSENT, the settings-lane list kept verbatim: no `SetInputMode`,
	//  no `bShowMouseCursor`, no `SetKeyboardFocus`, no navigation rule table, no input binding.
	//  ⚠️ The ⛔ ONE departure from that list is `NativeOnPreviewKeyDown` above, and it exists
	//  ⛔ solely to keep a shipped close route alive — its argument is at its own declaration.
	// ═══════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  Tell the menu-input subsystem this overlay now owns menu navigation, and let it place the
	 *  ring on the screen's first focus stop.
	 *
	 *  ⛔ CALLED FROM `ApplyOpenState(true)` AND ⛔ AFTER ITS `SetVisibility` — see the block
	 *  comment above, and the call site's own note on why the order is load-bearing.
	 *  ⛔ AND (TASK-1432 QA loop 1) FROM `ReturnToList` ON A ⛔ REAL detail→list EDGE, ⛔ also
	 *  AFTER the switcher write, for the ⛔ same reason in a different coordinate: one line
	 *  earlier `FindPathToWidget` still resolves through the ⛔ detail slot and the ring's focus
	 *  request fails silently. See `UnregisterAsMenuNavTarget` below for the whole contract.
	 *  Null subsystem ⇒ one `Log` line and the overlay stays exactly as mouse-driven as it was.
	 */
	void RegisterAsMenuNavTarget();

	/**
	 *  Give menu navigation back. Called from `ApplyOpenState(false)` AND from `NativeDestruct`
	 *  — the double call is deliberate and safe: the subsystem removes by IDENTITY and
	 *  logs-not-warns for a screen already gone.
	 *
	 *  🚨🚨 TASK-1432 QA LOOP 1 — ⛔ THE PARAGRAPH THAT STOOD HERE WAS ⛔ FALSE AT ENGINE SOURCE,
	 *  AND IT IS ⛔ STRUCK RATHER THAN DELETED (`SC-§120`) because it was ⛔ load-bearing guidance
	 *  telling the next editor why no unregister was needed on a view switch.
	 *
	 *  ⛔ WAS: ~~"IT IS NOT CALLED FROM `ShowDetailForAction`/`ReturnToList`. The detail page is a
	 *  view of this same screen, not a second one: the switcher collapses the list branch, the
	 *  walker's ancestor-visibility test drops `CloseButton` on its own, and the registration must
	 *  survive so that returning to the list restores the ring with no re-registration."~~
	 *
	 *  ⛔ REFUTED, AND I RE-MEASURED IT MYSELF RATHER THAN ACCEPTING THE REPORT
	 *  (`qa/TASK-1433.md` BLOCKER-1): ⛔ A `SWidgetSwitcher` DOES NOT COLLAPSE ITS INACTIVE
	 *  BRANCH. `SWidgetSwitcher::OnArrangeChildren` arranges the ACTIVE slot only and the file
	 *  ⛔ writes no child's `EVisibility` at all; `UWidgetSwitcher` forwards a clamped index and
	 *  nothing else; `UWidget::IsVisible()` reads the widget's ⛔ OWN visibility. ⇒ every
	 *  visibility flag along `CloseButton`'s chain stays visible with the detail page up, so
	 *  `HasVisibleSlateAncestry` and therefore `IsNavFocusStop` ⛔ STILL ADMIT IT — while
	 *  `FSlateApplication::SetUserFocus` ⛔ REFUSES it, because `FSlateWindowHelper::FindPathToWidget`
	 *  calls `ValidatePathToChild`, which this widget overrides as
	 *  `return InChild == GetActiveWidget().Get();` under the engine's own comment naming
	 *  `SWidgetSwitcher` as the reason that check exists.
	 *  ⇒ ⛔ THE OLD SHAPE WOULD HAVE LEFT ⛔ ONE ADMITTED-BUT-UNFOCUSABLE STOP, and the subsystem's
	 *  own words are that such a stop ⛔ SWALLOWS THE RING rather than being skipped. ⛔ Reachable
	 *  by 🧑 a mouse click on any row.
	 *
	 *  ⭐ THE CONTRACT ~~NOW~~ (⭐ `TASK-1432`), IN ONE LINE — ~~**registered ⟺ `bHelpOpen` ⛔ AND
	 *  the list view is up.**~~
	 *  ~~Called from `ApplyOpenState(false)`, from `ShowDetailForAction` (the list→detail edge), and
	 *  from `NativeDestruct`.~~ ⛔ All three are safe together: the subsystem removes by ⛔ IDENTITY,
	 *  measures "was registered" ⛔ before its purge, and ⛔ logs rather than warns for a screen
	 *  that is already gone. The full argument, the four engine measurements and the ⛔ three
	 *  rejected alternatives live at `ShowDetailForAction`'s call site in the `.cpp`.
	 *
	 *  🚨🚨 ⭐ TASK-1478 — ⛔ THE CONTRACT IS ⛔ SIMPLER AGAIN, AND ⛔ THIS FUNCTION HAS ⛔ TWO CALL
	 *  SITES, ⛔ NOT THREE. ⛔ **registered ⟺ `bHelpOpen`.** ⛔ Called from `ApplyOpenState(false)`
	 *  and from `NativeDestruct` — ⛔ `ShowDetailForAction` no longer calls it at all.
	 *  ⛔ WHY: the detail page now ⛔ HAS a ring (⭐ `TASK-1484`: `{ BackButton, DetailScrollButton }`
	 *  — ⛔ was `{ BackButton }`; the ⛔ COUNT changed, the ⛔ ARGUMENT did not), so there is ⛔ nothing to
	 *  surrender on a view switch; the inactive branch is kept out of the ring by
	 *  `ApplyActiveView`'s ⛔ collapse instead. ⇒ ⛔ THIS IS THE RESOLUTION OF `qa/TASK-1433.md`
	 *  ⛔ WARN-L1, which ruled those two edges agreed with a switcher-aware predicate only ⛔ *"by a
	 *  contingent authored fact"* and named ⛔ the `BackButton` flip as the counter-case: ⛔ the
	 *  disagreement is removed by ⛔ deleting the edge, ⛔ not by hoping it stays agreed.
	 *  ⛔ The `NativeDestruct` net is ⛔ unchanged and still earns its place.
	 */
	void UnregisterAsMenuNavTarget();

	/**
	 *  Null-safe resolve of the menu-input subsystem. ⛔ Through the WORLD, ⛔ not the game
	 *  instance — `USiegeMenuInputSubsystem` is a `UWorldSubsystem` and declines Editor worlds
	 *  outright, so a null answer here is ORDINARY rather than an error.
	 */
	USiegeMenuInputSubsystem* ResolveMenuInputSubsystem() const;

	/**
	 *  True when `InKey` is the key the player must physically press to toggle THIS overlay.
	 *
	 *  ⛔ DERIVED, ⛔ NEVER TYPED (`HELP-§1`, `HELP-§4`: *the menu documents its own key*). It
	 *  reads the ⛔ same registry row and the ⛔ same accessor the R-24 key chip and the hint
	 *  line already read, so the key this predicate answers to is ⛔ by construction the key the
	 *  screen is telling the player to press — on QWERTY, on Dvorak, and after any remap. ⛔ No
	 *  second copy of the truth exists here to drift (`HELP-§2` mechanism 1).
	 *
	 *  ⛔ APPLIED KEYS, ⛔ NOT DISPLAY KEYS, AND THE LANE IS THE WHOLE POINT (`HELP-§1`'s
	 *  double-translate warning): `QueryAppliedKeysForRow` answers out of the ACTIVE,
	 *  already-retargeted context, which is what `FKeyEvent::GetKey()` reports.
	 *  `ResolveRowDisplayKeys` is the ⛔ LABEL lane and would be wrong here — on a fallback it
	 *  answers the QWERTY REFERENCE key, not the pressed one.
	 *
	 *  ⚠️ An unresolved `IA_ControlsHelp`, no local player or a non-`MappedAction` lane all
	 *  answer ⛔ false. That is the honest degradation and it costs nothing: in exactly those
	 *  states the toggle key never opened the overlay either.
	 */
	bool IsOwnToggleKey(const FKey& InKey) const;

	// ------------------------------------------------------------------------
	// PINNED CHILDREN — all BindWidgetOptional, never BindWidget.
	// ------------------------------------------------------------------------

	/** Full-screen container and the tree root. ⛔ SelfHitTestInvisible — see §4, correctness, not styling. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BackdropBorder;

	/**
	 *  ⭐ THE LIST-vs-DETAIL SWITCH (TASK-707). Child 0 is PanelBorder (the list, inset by the
	 *  panel margin); child 1 is DetailView (the detail page, edge to edge — Jonathan's "on the
	 *  entire screen").
	 *
	 *  ⚖️ A SWITCHER RATHER THAN TWO VISIBILITY FLAGS, DELIBERATELY: SWidgetSwitcher renders
	 *  exactly one child, so "the list is never behind the detail page" is STRUCTURAL. Two
	 *  Collapsed/Visible writes would make it a pair of statements that can fall out of step,
	 *  and the failure would be two screens of text on top of each other.
	 *
	 *  🚨 ⭐ TASK-1478 — ⛔ THAT RULING ⛔ STANDS AND IS ⛔ NARROWED, ⛔ NOT OVERTURNED, AND THE
	 *  DISTINCTION IS THE WHOLE OF IT: ⛔ the switcher is ⛔ STILL what renders, ⛔ still the single
	 *  source of truth (`IsDetailViewActive()` asks ⛔ it, never a mirrored bool), and this row
	 *  ⛔ did ⛔ NOT replace it with two flags. ⛔ What was added is a ⛔ DERIVED SHADOW of the
	 *  switcher's own state: `ApplyActiveView` writes ⛔ both branch visibilities ⛔ and the index
	 *  from ⛔ one parameter in ⛔ one function, so they ⛔ cannot fall out of step — there is ⛔ no
	 *  second state to forget, only one assignment per branch on ⛔ every call.
	 *  ⛔ AND ITS ⛔ ONLY CONSUMER IS THE ⛔ FOCUS WALKER: `SWidgetSwitcher` already declines to
	 *  arrange, render and hit-test the inactive branch, so the shadow changes ⛔ no pixel — it
	 *  exists solely so `HasVisibleSlateAncestry` ⛔ stops being lied to about a branch the engine
	 *  had already switched off. ⛔ The feared failure ("two screens of text on top of each other")
	 *  is ⛔ still impossible, because the ⛔ renderer is still the switcher.
	 *  ⭐ UWidgetSwitcher constructs itself SelfHitTestInvisible (WidgetSwitcher.cpp:18), so it
	 *  changes nothing about §4's hit-test law — its CHILDREN keep deciding.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ViewSwitcher;

	/** The readable dark plate. ⛔ Visible (hit-testable) — it absorbs ITS OWN clicks and nothing else's (§4). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PanelBorder;

	/** ⭐ The full-screen detail page (TASK-707) — the switcher's second child. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<USiegeControlsDetailWidget> DetailView;

	/** title / hint / list / close. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

	/** "Controls". */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** The one line that tells the player how to close it and that rows are clickable. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	/** The scrolling list. RefreshRows() owns its children entirely. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> RowScrollBox;

	/** The overlay's own Close control — one of the TWO close routes, and ⛔ the complete list with TAB (§2). */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** "Close" — the button's content text. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|ControlsHelp", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CloseLabelText;

	/**
	 *  /Game/UI/WBP_ControlsHelpRow once it exists; null ⇒ USiegeControlsHelpRowWidget.
	 *  SOFT and null-safe, so the row asset can land arbitrarily late with ZERO C++ change
	 *  (`HELP-§3`'s escape hatch is only real if this slot exists).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|ControlsHelp")
	TSoftClassPtr<USiegeControlsHelpRowWidget> RowWidgetClass;

	/**
	 *  /Game/UI/WBP_ControlsDetail once it exists; null ⇒ USiegeControlsDetailWidget.
	 *  SOFT and null-safe, for the same reason as RowWidgetClass: `HELP-§3`'s promise that a
	 *  future WBP wins with ZERO C++ change is only real if the slot exists (TASK-706's D-3).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|ControlsHelp")
	TSoftClassPtr<USiegeControlsDetailWidget> DetailWidgetClass;

private:

	/** True while the overlay is shown. ⛔ Display state ONLY — the cursor posture is the controller's (§3). */
	bool bHelpOpen = false;

	/** The last activated row's id (§7b). NAME_None until a row is clicked. */
	FName SelectedActionId;

	/**
	 *  The live row widgets, in list order. Rooted while the list is built and DESTROYED on
	 *  every RefreshRows — ⛔ never a cache of a derived label across opens.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USiegeControlsHelpRowWidget>> RowWidgets;

	/** Latches the "no layout subsystem" warning to ONE line per widget instance. */
	bool bLoggedNoLayoutSubsystem = false;
};
