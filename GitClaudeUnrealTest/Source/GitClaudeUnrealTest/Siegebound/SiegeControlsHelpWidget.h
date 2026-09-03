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
 *  ⛔⛔ `Escape` IS UNTOUCHED HERE TOO (`AS-§6 A-2` — PERMANENT; `HELP-§5`). This class overrides
 *  NO key handler of any kind. It returns to the list on its own Back button; the OVERLAY closes
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

	/** header / summary / scrolling body / back. */
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
 *  ALL THREE classes in this pair — this one, USiegeControlsHelpRowWidget and
 *  USiegeControlsDetailWidget — override NO key handler at all: no NativeOnKeyDown, no
 *  NativeOnPreviewKeyDown, no Enhanced Input action of their own, no Slate FReply::Handled()
 *  on any key, no viewport intercept. The token `Escape` appears in this file ONLY as
 *  documentation prose (the two cancel rows' data and their detail text), never in a handler.
 *  ⇒ THE OVERLAY CLOSES ON TAB (the toggle) AND ON ITS OWN CLOSE BUTTON. That is the
 *  COMPLETE list, and it did NOT grow when the detail view landed: that view returns to the
 *  LIST on its own Back button, which is a different thing from closing (TASK-707).
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
 *  SIXTH term in that one composition (SiegePlayerController.cpp:4357). A direct posture call
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
	//~ End UUserWidget interface

	/**
	 *  ⭐ SHOWS THE FULL-SCREEN DETAIL PAGE FOR ONE ROW (§7c, TASK-707).
	 *
	 *  Composes the page purely (ComposeDetailContent), stamps DetailView, and switches the
	 *  view. ⛔ It sets NO input mode, shows NO cursor and is NOT a second cursor owner — the
	 *  overlay's ONE registration in ApplyCursorInputState() covers both views (TASK-074's law).
	 *  ⛔ It claims no key. An unknown id, a missing switcher or a missing view all leave the
	 *  LIST on screen and log — ⛔ never a blank page and ⛔ never a screen with no way out.
	 */
	virtual void ShowDetailForAction(FName InActionId);

	/**
	 *  ⭐ THE DETAIL VIEW'S "BACK TO THE LIST" (§7c, TASK-707). Idempotent and null-safe, which
	 *  is why CloseHelp() can call it unconditionally on EVERY close — including a close that
	 *  happened while the list was already up.
	 */
	virtual void ReturnToList();

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
