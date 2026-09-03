// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Siegebound/SiegeControlsHelpWidget.h"
// TASK-821 test 14: FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes() +
// FSiegePositionalKeyProbe. The digit-holds half is asserted against the SHIPPED positional
// table rather than against this file's hand-built fixture — the table is what any real
// translation map is built from, so "a digit can never move" is read from the source of that
// fact instead of from the fixture's silence about digits.
#include "Siegebound/SiegeKeyboardLayoutStatics.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the TAB CONTROLS OVERLAY ═══
 *  (CONTROLS-MENU batch. Tests 1-8 = TASK-706: FSiegeControlsHelpRegistry + the row click seam.
 *  Tests 9-13 = TASK-707: the full-screen detail view — its prose, its token-spliced keys, its
 *  related-controls blocks and its way back. ⛔ ONE FILE for the whole feature, by `HELP-§6`.
 *  Law: `HELP-§1`/`§2`/`§4`/`§5`/`§6`. QA gate: TASK-708. Compile + suite gate: TASK-709.
 *
 *  ⭐⭐ TEST 18 = TASK-852: the `RelatedActionIds` GRAPH INTEGRITY WALK — every row's outbound
 *  edges resolve to real rows, with the negative control that proves it can go red. ⛔ IT IS AN
 *  EXTRACTION FROM TEST 9, ⛔ NOT A NEW CHECK: the walk shipped inside `EveryRowHasAuthoredDetail`
 *  for months, was correct and green the whole time, and was READ AS ABSENT BY TWO INDEPENDENT
 *  READERS because its name said "detail authoring". A law was written declaring it missing and a
 *  task boarded to duplicate it. ⇒ if you are looking for a claim in this file, ⛔ SEARCH FOR THE
 *  SYMBOL, ⛔ not for the test name you would have chosen (`SC-§40` cl. 11(b), `HELP-§7`).
 *
 *  ⭐ TEST 17 = TASK-870 (POST-GATE batch): the RIGHT-CLICK half of `CARDBAR-§8`/`STACK-§4`, and
 *  the guard on the one sentence Jonathan refuted by observation. It is test 16's shape applied to
 *  a SECOND gesture, and it is deliberately filed under a name that says what it checks
 *  (`HELP-§7`'s "a test's NAME is its discoverability surface").
 *
 *  ⭐ TEST 14 = TASK-821 (CARDBAR batch): the rewritten `Cards.Discard` row. It is the FIRST
 *  test in this file to carry `HELP-§6`'s changes/holds pair on TWO SHIPPED ROWS AT ONCE — the
 *  discard-all LETTER must move under a layout flip while the card DIGITS hold, in one test,
 *  through one code path. ⛔ No test above it was edited to accommodate the rewrite, and in
 *  particular test 1's `RequiredIds[]` still names `Cards.Discard` and was NOT touched: the row
 *  was rewritten in place precisely so it would not have to be.)
 *
 *  ⭐⭐ THE PROPERTY THAT MAKES THIS FILE POSSIBLE, AND THE FIRST THING TO CHECK IF ONE OF
 *      THESE EVER GOES RED: EVERY TEST BELOW PASSES ON A QWERTY MACHINE, WITH NO DVORAK
 *      HARDWARE ANYWHERE — and every one of them would go red on the double-translate defect
 *      that a QWERTY reviewer cannot see by looking at the screen.
 *
 *  The seam is USiegeKeyboardLayoutSubsystem::SetTranslationMapForAutomationTests
 *  (SiegeKeyboardLayoutSubsystem.h:299-322), the shipped tests-only setter that LATCHES an
 *  instance out of OS probing so a QWERTY host can drive BOTH layout states. Everything
 *  between the injected map and the asserted label is FSiegeControlsHelpRegistry's pure
 *  logic, which is the thing under test.
 *
 *  ⚠️⚠️ THE ASSERTION SHAPE IS THE LAW, NOT A STYLE (`HELP-§6`): every label claim below is
 *  made against WHAT THE ACCESSOR ANSWERS, ⛔ never against a typed letter. A test that said
 *  `TestEqual(Label, TEXT("U"))` would pass on a broken implementation that hardcoded `U`,
 *  which is the exact failure class this whole feature exists to prevent. The only place a
 *  letter is named is a FIXTURE SELF-CHECK — asserting that the injected map really does move
 *  the key, so that the "it changed" claims are not vacuous — and each one says so.
 *
 *  ⛔ WHAT THESE TESTS CANNOT PROVE, STATED SO NOBODY MISTAKES GREEN FOR DONE (`SC-§32`):
 *  nothing here presses a key, opens PIE, builds a Slate tree, or asks Windows anything.
 *    • The `Escape` guarantee is asserted here only as far as reflection can see it — the
 *      AUTHORITATIVE gate is TASK-708 criterion 4's grep of the diff (see test 6).
 *    • That the overlay LOOKS right closes on pixels or Jonathan's eyes and on nothing else
 *      (`AS-§6` A(e), `HELP-§3`).
 *    • That TAB actually reaches Enhanced Input through a viewport-added overlay is a PIE
 *      observation; the code-side mitigation (every button non-focusable, because `Tab` is
 *      Slate's own focus-next key) is declared in the TASK-706 handoff as unverified-by-test.
 *
 *  ⚠️ DECLARED DEVIATION (`SC-§15`): `HELP-§6` words the Dvorak case as "exercised via the
 *  `KBD-§7` CVar" (`siege.Input.LayoutPollEnabled`). That CVar governs the 1 Hz OS POLL, and
 *  the scratch subsystem below never has Initialize(FSubsystemCollectionBase&) called — so no
 *  timer, no probe and no Slate hook ever run, and the CVar has nothing to gate
 *  (SiegeKeyboardLayoutTest.cpp:439-443 records the same reasoning for the same fixture).
 *  Driving the state through the tests-only SETTER is what TASK-704 §1.4 specifies and is
 *  STRICTLY STRONGER: it forces both states deterministically instead of asking a machine.
 *
 *  M8 DECLARATION (verbatim): adds no replicated property, no new replicated class, no new
 *  relevancy tier, no RPC. This is a test file; it adds no shipped surface at all.
 */

namespace SiegeControlsHelpTestUtils
{
	// ════════════════════════════════════════════════════════════════════════════════════
	//  THE US-DVORAK FIXTURE (a subset, and deliberately so)
	// ════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  The US-Dvorak translation entries this file needs, taken from the SHIPPED fixture at
	 *  Tests/SiegeKeyboardLayoutTest.cpp:204-229 rather than re-derived — a second,
	 *  independently-guessed Dvorak table is exactly the drifting copy this project keeps
	 *  paying for.
	 *
	 *  ⚠️ IT IS A SUBSET AND THAT IS CORRECT, NOT LAZY: GetPositionalKey returns its input
	 *  UNCHANGED for any key with no entry (SiegeKeyboardLayoutSubsystem.h:238-240), and
	 *  identity entries are NEVER stored — so `A` and `M`, the two positions US-Dvorak leaves
	 *  in place, are represented by their ABSENCE here, which is precisely how the shipped map
	 *  represents them.
	 *
	 *  ⭐ `F -> U` is Jonathan's own worked example, and `U -> G` is the SECOND HALF OF THE
	 *  TRAP: applying the map twice turns `F` into `G` (TASK-704 §1, SiegePlayerController.h:
	 *  1222-1226). Both are needed for test 2 to be able to detect the defect at all.
	 */
	static TMap<FKey, FKey> MakeDvorakTranslation()
	{
		TMap<FKey, FKey> Translation;
		Translation.Add(EKeys::F, EKeys::U);           // ⭐ the key that MOVES (IA_CmdAmbush)
		Translation.Add(EKeys::U, EKeys::G);           // ⭐ the second hop the double-translate would take
		Translation.Add(EKeys::T, EKeys::Y);           // IA_CmdAttack
		Translation.Add(EKeys::R, EKeys::P);           // IA_CmdHold
		Translation.Add(EKeys::E, EKeys::Period);      // IA_CmdDefend
		Translation.Add(EKeys::C, EKeys::J);           // IA_CmdFollow
		Translation.Add(EKeys::Q, EKeys::Apostrophe);  // IA_Rally
		Translation.Add(EKeys::W, EKeys::Comma);       // IA_Move forward
		Translation.Add(EKeys::S, EKeys::O);           // IA_Move back
		Translation.Add(EKeys::D, EKeys::E);           // IA_Move right
		// ⭐ TASK-821: the discard-all key's POSITION (IA_DiscardAll, the Cards.Discard row).
		// ⛔ Taken from the SHIPPED table at Tests/SiegeKeyboardLayoutTest.cpp:211 ({ EKeys::H,
		// 0x44, 'D' }) rather than re-derived — this file's whole fixture rule.
		// ⭐⭐ AND IT INHERITS THE DOUBLE-TRANSLATE TRAP FOR FREE, which is why test 14 can detect
		// one: `D` already has an onward hop of its own two lines above (`D` -> `E`), so
		// `H` -> `D` -> `E` is a THREE-DISTINCT-KEY chain exactly like `F` -> `U` -> `G`.
		Translation.Add(EKeys::H, EKeys::D);
		Translation.Add(EKeys::Z, EKeys::Semicolon);   // ⭐ the assistant accept key's POSITION
		// ⛔ `A` and `M` are ABSENT ON PURPOSE — they are the two identities, and an entry for
		// either would make the "this key HOLDS" assertions vacuous.
		return Translation;
	}

	/**
	 *  A USiegeKeyboardLayoutSubsystem inside a throwaway UGameInstance.
	 *
	 *  ⚠️ THE OUTER IS NOT OPTIONAL: UGameInstanceSubsystem is UCLASS(Abstract, Within =
	 *  GameInstance), so a bare NewObject lands in the transient package and trips the
	 *  ClassWithin check in StaticAllocateObject. Same shape as SiegeKeyboardLayoutTest.cpp:
	 *  453-462, cloned rather than re-invented.
	 */
	struct FScratchLayout
	{
		TStrongObjectPtr<UGameInstance>                 GameInstance;
		TStrongObjectPtr<USiegeKeyboardLayoutSubsystem> Layout;

		bool IsValid() const { return GameInstance.IsValid() && Layout.IsValid(); }
	};

	static FScratchLayout MakeScratchLayout(const TMap<FKey, FKey>& Translation)
	{
		FScratchLayout Scratch;
		Scratch.GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackageAsObject()));
		if (Scratch.GameInstance.IsValid())
		{
			Scratch.Layout.Reset(NewObject<USiegeKeyboardLayoutSubsystem>(Scratch.GameInstance.Get()));
			if (Scratch.Layout.IsValid())
			{
				// ⛔ Tests-only setter (`KBD-§8`). It LATCHES this instance out of OS probing,
				// which is what lets an EMPTY map genuinely mean "QWERTY" on any host.
				Scratch.Layout->SetTranslationMapForAutomationTests(Translation);
			}
		}
		return Scratch;
	}

	/** Readable in a failure message: "W", "Comma", or "<invalid>". */
	static FString Describe(const FKey& Key)
	{
		return Key.IsValid() ? Key.ToString() : FString(TEXT("<invalid>"));
	}

	static FString DescribeKeys(const TArray<FKey>& Keys)
	{
		if (Keys.Num() == 0)
		{
			return FString(TEXT("<none>"));
		}

		FString Out;
		for (const FKey& Key : Keys)
		{
			if (!Out.IsEmpty())
			{
				Out += TEXT(", ");
			}
			Out += Describe(Key);
		}
		return Out;
	}

	/** A synthetic registry row, so a lane can be exercised without depending on a shipped row's data. */
	static FSiegeControlsHelpAction MakeRow(ESiegeInputLane Lane, const TArray<FKey>& ReferenceKeys, bool bLiteral, bool bPointer)
	{
		FSiegeControlsHelpAction Row;
		Row.ActionId            = FName(TEXT("Test.SyntheticRow"));
		Row.Category            = FName(TEXT("Hero"));
		Row.DisplayName         = FText::FromString(TEXT("Synthetic"));
		Row.OneLine             = FText::FromString(TEXT("A synthetic row used only by the automation suite."));
		Row.Lane                = Lane;
		Row.QwertyReferenceKeys = ReferenceKeys;
		Row.bLiteralKeyLabel    = bLiteral;
		Row.bPointerOnly        = bPointer;
		return Row;
	}

	/** True when any UPROPERTY on `Class` is an FKey (or a container of them) — see test 6. */
	static bool ClassDeclaresAnyKeyProperty(const UClass* Class, FString& OutOffender)
	{
		if (Class == nullptr)
		{
			return false;
		}

		for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIterationFlags::None); PropertyIt; ++PropertyIt)
		{
			const FString CppType = PropertyIt->GetCPPType(nullptr, 0);
			if (CppType.Contains(TEXT("FKey")))
			{
				OutOffender = FString::Printf(TEXT("%s %s"), *CppType, *PropertyIt->GetName());
				return true;
			}
		}
		return false;
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 1 — Siegebound.ControlsHelp.RegistryCoversTheActionSet
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  THE ROW SET IS NON-EMPTY, COVERS THE ENUMERATED ACTIONS, AND IS INTERNALLY CONSISTENT
 *  (`HELP-§6` criterion 1).
 *
 *  ⭐ Coverage is asserted BY ActionId, ⛔ never by key — an id survives a rebind and a layout
 *  change, which is the whole point of keying the registry to the action rather than the key.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpRegistryTest,
	"Siegebound.ControlsHelp.RegistryCoversTheActionSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpRegistryTest::RunTest(const FString& Parameters)
{
	const TArray<FSiegeControlsHelpAction>& Rows = FSiegeControlsHelpRegistry::GetActions();

	TestTrue(TEXT("The registry is non-empty"), Rows.Num() > 0);

	// ── JONATHAN'S ENUMERATED MINIMUM + THE SHIPPED SURFACE (TASK-704 §4/§6) ────────────
	// ⚠️ Asserted as a REQUIRED SUBSET, not an exact list: a later command must be free to
	// appear here (`HELP-§2` mechanism 2's whole purpose) without turning this test red.
	const TCHAR* RequiredIds[] =
	{
		TEXT("Hero.Move"), TEXT("Hero.Look"), TEXT("Hero.Jump"), TEXT("Hero.Sprint"),
		TEXT("Hero.Attack"), TEXT("Hero.Rally"),
		TEXT("Cards.Play"), TEXT("Cards.CursorHold"), TEXT("Cards.Discard"), TEXT("Cards.Cancel"),
		TEXT("Orders.Attack"), TEXT("Orders.Defend"), TEXT("Orders.Hold"), TEXT("Orders.Ambush"), TEXT("Orders.Follow"),
		TEXT("PickMode.Confirm"), TEXT("PickMode.Resize"), TEXT("PickMode.Cancel"),
		TEXT("Interface.AssistantConsole"), TEXT("Interface.AssistantAccept"),
		TEXT("Interface.WarMap"), TEXT("Interface.WarMapReveal"), TEXT("Interface.WarMapMarker"),
		// ⭐ `HELP-§4`: THE MENU DOCUMENTS ITS OWN KEY. A controls screen that omits the key
		// that opened it is the one omission a player cannot recover from.
		TEXT("Interface.ControlsHelp")
	};

	for (const TCHAR* RequiredId : RequiredIds)
	{
		TestNotNull(*FString::Printf(TEXT("The registry carries a row for '%s'"), RequiredId),
			FSiegeControlsHelpRegistry::FindAction(FName(RequiredId)));
	}

	// ── INTERNAL CONSISTENCY ────────────────────────────────────────────────────────────
	TSet<FName> SeenIds;
	int32 LiteralLabelRows = 0;

	for (const FSiegeControlsHelpAction& Row : Rows)
	{
		const FString RowName = Row.ActionId.ToString();

		TestFalse(TEXT("Every row carries a non-None ActionId"), Row.ActionId.IsNone());

		bool bAlreadySeen = false;
		SeenIds.Add(Row.ActionId, &bAlreadySeen);
		TestFalse(*FString::Printf(TEXT("ActionId '%s' appears exactly once"), *RowName), bAlreadySeen);

		TestFalse(*FString::Printf(TEXT("Row '%s' carries a category"), *RowName), Row.Category.IsNone());
		TestFalse(*FString::Printf(TEXT("Row '%s' has a display name"), *RowName), Row.DisplayName.IsEmptyOrWhitespace());

		// Lane D carries no key and no action, by definition.
		if (Row.Lane == ESiegeInputLane::PointerOnly)
		{
			TestTrue(*FString::Printf(TEXT("Pointer-only row '%s' is flagged bPointerOnly"), *RowName), Row.bPointerOnly);
			TestEqual(*FString::Printf(TEXT("Pointer-only row '%s' carries NO reference key"), *RowName),
				Row.QwertyReferenceKeys.Num(), 0);
			TestEqual(*FString::Printf(TEXT("Pointer-only row '%s' carries NO input action"), *RowName),
				Row.Actions.Num(), 0);
			continue;
		}

		TestFalse(*FString::Printf(TEXT("Non-pointer row '%s' is NOT flagged bPointerOnly"), *RowName), Row.bPointerOnly);
		TestTrue(*FString::Printf(TEXT("Row '%s' carries at least one QWERTY reference key (the fallback + the fixture)"), *RowName),
			Row.QwertyReferenceKeys.Num() > 0);

		for (const FKey& ReferenceKey : Row.QwertyReferenceKeys)
		{
			TestTrue(*FString::Printf(TEXT("Row '%s' reference key %s is a VALID FKey"),
				*RowName, *SiegeControlsHelpTestUtils::Describe(ReferenceKey)), ReferenceKey.IsValid());
		}

		if (Row.Lane == ESiegeInputLane::MappedAction)
		{
			// ⛔ A Lane-A row MUST name its action, or the one-translation primary path can
			// never run for it and it would silently live on the fallback forever.
			TestTrue(*FString::Printf(TEXT("Mapped row '%s' names at least one IA_* asset"), *RowName),
				Row.Actions.Num() > 0);
			for (const TSoftObjectPtr<UInputAction>& SoftAction : Row.Actions)
			{
				TestFalse(*FString::Printf(TEXT("Mapped row '%s' has a non-empty soft action path"), *RowName),
					SoftAction.IsNull());
			}
		}
		else
		{
			// ⛔ A raw-polled row must NOT name an action: naming one would put it on the
			// Enhanced Input query path and quietly give a raw key a translation it never had.
			TestEqual(*FString::Printf(TEXT("Raw-polled row '%s' names NO input action"), *RowName),
				Row.Actions.Num(), 0);
		}

		if (Row.bLiteralKeyLabel)
		{
			++LiteralLabelRows;
			// `KBD-§8`'s pin belongs to the raw-letter lane and to nothing else. A literal
			// label on a MAPPED row would be a hardcoded key by another name.
			TestTrue(*FString::Printf(TEXT("Row '%s' carries bLiteralKeyLabel only on the RawLetter lane"), *RowName),
				Row.Lane == ESiegeInputLane::RawLetter);
		}
	}

	// ⭐ EXACTLY ONE SANCTIONED LITERAL IN THE WHOLE REGISTRY (TASK-704 §8 F-1 — a FLAGGED
	// Jonathan decision). A second one appearing is a silent widening of an exception that was
	// argued once, on its own facts.
	TestEqual(TEXT("Exactly ONE row carries bLiteralKeyLabel (the KBD-§8 accept key, F-1)"), LiteralLabelRows, 1);

	const FSiegeControlsHelpAction* const AcceptRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.AssistantAccept")));
	if (AcceptRow != nullptr)
	{
		TestTrue(TEXT("...and it is Interface.AssistantAccept"), AcceptRow->bLiteralKeyLabel);
	}

	// FindAction is total: an unknown id and NAME_None both answer null rather than asserting.
	TestNull(TEXT("FindAction on an unknown id returns null"),
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Nope.NotAnAction"))));
	TestNull(TEXT("FindAction on NAME_None returns null"), FSiegeControlsHelpRegistry::FindAction(NAME_None));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 2 — Siegebound.ControlsHelp.MappedLaneIsNeverDoubleTranslated   ⭐⭐ THE KEYSTONE
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ THE ONE-TRANSLATION LAW, ASSERTED AS A BEHAVIOUR (`HELP-§1`; TASK-704 §1.2).
 *
 *  THE DEFECT THIS TEST EXISTS TO CATCH, SPELLED OUT: the layout subsystem rewrites the
 *  APPLIED IMC duplicate's keys wholesale, so on US-Dvorak QueryKeysMappedToAction already
 *  answers `U` for the action QWERTY binds to `F`. An implementation that "helpfully" ran
 *  GetPositionalKey over that answer would produce `G` — a key that is not bound to anything,
 *  printed on the help screen as gospel. ⛔ ON A QWERTY MACHINE BOTH IMPLEMENTATIONS LOOK
 *  IDENTICAL, which is why this must be a test and not a code review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpNoDoubleTranslateTest,
	"Siegebound.ControlsHelp.MappedLaneIsNeverDoubleTranslated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpNoDoubleTranslateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());
	if (!Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeKeyboardLayoutSubsystem inside a UGameInstance - the one-translation claim is untested."));
		return false;
	}

	// ── FIXTURE SELF-CHECK, FIRST ───────────────────────────────────────────────────────
	// ⚠️ THE ONLY PLACE THIS FILE NAMES LETTERS, AND IT IS A CLAIM ABOUT THE FIXTURE, NOT
	// ABOUT A LABEL: without a map that genuinely moves the key twice, every assertion below
	// would pass on a broken implementation for the wrong reason.
	const FKey OnceTranslated  = Dvorak.Layout->GetPositionalKey(EKeys::F);
	const FKey TwiceTranslated = Dvorak.Layout->GetPositionalKey(OnceTranslated);
	TestTrue(TEXT("FIXTURE: the injected map moves F once"), OnceTranslated != EKeys::F);
	TestTrue(TEXT("FIXTURE: applying it AGAIN moves it a second time - so a double-translate is DETECTABLE"),
		TwiceTranslated != OnceTranslated);

	// ── THE CLAIM ───────────────────────────────────────────────────────────────────────
	// A Lane-A row whose ACTIVE context has already been retargeted: QueryKeysMappedToAction
	// answered `OnceTranslated`. The resolver must hand that straight through.
	const FSiegeControlsHelpAction MappedRow = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);

	const TArray<FKey> Resolved =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(MappedRow, { OnceTranslated }, Dvorak.Layout.Get());

	TestEqual(TEXT("A mapped row with an applied key yields exactly one label"), Resolved.Num(), 1);
	if (Resolved.Num() == 1)
	{
		TestTrue(FString::Printf(
			TEXT("⭐ The applied key is handed through UNCHANGED (got %s, expected the applied %s)"),
			*Describe(Resolved[0]), *Describe(OnceTranslated)),
			Resolved[0] == OnceTranslated);

		// ⛔ THE DEFECT, ASSERTED AGAINST DIRECTLY. This is the assertion that goes red the
		// moment anyone adds a GetPositionalKey call to the Lane-A primary path.
		TestTrue(FString::Printf(
			TEXT("⛔ It is NOT translated a SECOND time (a double-translate would yield %s)"),
			*Describe(TwiceTranslated)),
			Resolved[0] != TwiceTranslated);
	}

	// ── THE APPLIED ANSWER OUTRANKS THE REGISTRY, ABSOLUTELY ────────────────────────────
	// The QWERTY column is a fallback and a fixture (TASK-704 §1.2 / D-5), ⛔ never the
	// displayed truth while the action resolves. A row whose reference key disagrees with its
	// live binding must show the LIVE one — that is what makes the route survive a rebind with
	// zero registry edits.
	const FSiegeControlsHelpAction StaleRow = MakeRow(ESiegeInputLane::MappedAction, { EKeys::T }, false, false);
	const TArray<FKey> FromApplied =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(StaleRow, { EKeys::SpaceBar }, Dvorak.Layout.Get());

	TestEqual(TEXT("A rebound action shows its LIVE key, not the registry's reference key"), FromApplied.Num(), 1);
	if (FromApplied.Num() == 1)
	{
		TestTrue(TEXT("...and the stale reference key does not appear at all"), FromApplied[0] == EKeys::SpaceBar);
	}

	// ── AND THE LIVE QUERY IS NULL-SAFE ─────────────────────────────────────────────────
	// ⛔ A missing controller / local player / subsystem is a FALLBACK, never a crash
	// (`KBD-§5`), and it is the state in which every one of these tests runs.
	TestEqual(TEXT("QueryAppliedKeysForRow with no controller answers EMPTY and never crashes"),
		USiegeControlsHelpWidget::QueryAppliedKeysForRow(MappedRow, nullptr).Num(), 0);

	// ⛔ It also refuses to answer for a non-Lane-A row, which is what stops a raw key from
	// ever acquiring a translation through the Enhanced Input path.
	const FSiegeControlsHelpAction RawRow = MakeRow(ESiegeInputLane::RawNonLetter, { EKeys::RightMouseButton }, false, false);
	TestEqual(TEXT("QueryAppliedKeysForRow answers EMPTY for a raw-polled row, by lane"),
		USiegeControlsHelpWidget::QueryAppliedKeysForRow(RawRow, nullptr).Num(), 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 3 — Siegebound.ControlsHelp.DvorakFallbackDerivesLabels   ⭐ `HELP-§6`'s named case
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ `HELP-§6`'s ACCEPTANCE CASE, VERBATIM: "flip the simulated layout and assert the label
 *  CHANGES for a key that moves and HOLDS for one that does not."
 *
 *  The pairs are TASK-704 §1.4's, derived rather than guessed: `F` -> `U` is Jonathan's own
 *  worked example (the key that moves); `M` and `A` are the two US-Dvorak identities (the keys
 *  that hold).
 *
 *  ⛔ EVERY LABEL CLAIM IS MADE AGAINST THE ACCESSOR'S OWN ANSWER, ⛔ never against a typed
 *  letter (`HELP-§6`). This exercises the Lane-A FALLBACK — the branch taken when no active
 *  context maps the action — which is the one place a translation legitimately happens.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpDvorakDerivationTest,
	"Siegebound.ControlsHelp.DvorakFallbackDerivesLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpDvorakDerivationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());   // an EMPTY map IS the QWERTY state
	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());

	if (!Qwerty.IsValid() || !Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the two scratch layout subsystems - the Dvorak case is untested."));
		return false;
	}

	TestFalse(TEXT("The QWERTY fixture reports NO active remap"), Qwerty.Layout->IsPositionalRemapActive());
	TestTrue(TEXT("The Dvorak fixture reports an ACTIVE remap"), Dvorak.Layout->IsPositionalRemapActive());

	struct FCase { const FKey& ReferenceKey; bool bExpectMove; const TCHAR* Why; };
	const FCase Cases[] =
	{
		{ EKeys::F, true,  TEXT("Orders.Ambush - Jonathan's own worked example: this key MOVES") },
		{ EKeys::T, true,  TEXT("Orders.Attack - moves") },
		{ EKeys::C, true,  TEXT("Orders.Follow - moves") },
		{ EKeys::M, false, TEXT("Interface.WarMap - US-Dvorak leaves the M position in place: this key HOLDS") },
		{ EKeys::A, false, TEXT("Hero.Move (strafe left) - the second US-Dvorak identity: HOLDS") },
		{ EKeys::Tab, false, TEXT("Interface.ControlsHelp - a non-letter is never in the 26-letter table: HOLDS") }
	};

	for (const FCase& Case : Cases)
	{
		// ⛔ Lane A with an EMPTY applied-key array = the fallback branch (the action resolved
		// to nothing in any active context), which is exactly the state on a machine where
		// IA_* has not landed.
		const FSiegeControlsHelpAction Row = MakeRow(ESiegeInputLane::MappedAction, { Case.ReferenceKey }, false, false);

		const TArray<FKey> OnQwerty = FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, TArray<FKey>(), Qwerty.Layout.Get());
		const TArray<FKey> OnDvorak = FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, TArray<FKey>(), Dvorak.Layout.Get());

		if (OnQwerty.Num() != 1 || OnDvorak.Num() != 1)
		{
			AddError(FString::Printf(TEXT("%s: the fallback did not produce exactly one label (QWERTY %s, Dvorak %s)."),
				Case.Why, *DescribeKeys(OnQwerty), *DescribeKeys(OnDvorak)));
			continue;
		}

		// ── (a) DERIVATION, ASSERTED AGAINST THE ACCESSOR ITSELF ───────────────────────
		// ⛔ NOT against a letter. This is the assertion `HELP-§6` actually asks for: the
		// label IS whatever the layout system says the physical position yields.
		TestTrue(FString::Printf(TEXT("%s: the QWERTY label IS GetPositionalKey's answer (got %s)"),
			Case.Why, *Describe(OnQwerty[0])),
			OnQwerty[0] == Qwerty.Layout->GetPositionalKey(Case.ReferenceKey));

		TestTrue(FString::Printf(TEXT("%s: the Dvorak label IS GetPositionalKey's answer (got %s)"),
			Case.Why, *Describe(OnDvorak[0])),
			OnDvorak[0] == Dvorak.Layout->GetPositionalKey(Case.ReferenceKey));

		// ── (b) THE CHANGES / HOLDS PAIR ───────────────────────────────────────────────
		if (Case.bExpectMove)
		{
			TestTrue(FString::Printf(TEXT("%s: the label CHANGED between layouts (%s -> %s)"),
				Case.Why, *Describe(OnQwerty[0]), *Describe(OnDvorak[0])),
				OnQwerty[0] != OnDvorak[0]);
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s: the label HELD across layouts (%s)"),
				Case.Why, *Describe(OnQwerty[0])),
				OnQwerty[0] == OnDvorak[0]);
		}
	}

	// ── ⛔ THE FAIL-SAFE ROW (`KBD-§5`) ─────────────────────────────────────────────────
	// No subsystem at all must degrade to the reference key, ⛔ never to nothing and ⛔ never
	// to EKeys::Invalid: the worst outcome this feature may produce is "the game behaves
	// exactly as it did yesterday".
	const FSiegeControlsHelpAction FailSafeRow = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);
	const TArray<FKey> WithNoSubsystem =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(FailSafeRow, TArray<FKey>(), nullptr);

	TestEqual(TEXT("With NO layout subsystem the fallback still produces a label"), WithNoSubsystem.Num(), 1);
	if (WithNoSubsystem.Num() == 1)
	{
		TestTrue(TEXT("...and it is the reference key, unchanged (KBD-§5's fail-safe)"), WithNoSubsystem[0] == EKeys::F);
		TestTrue(TEXT("...and it is never EKeys::Invalid"), WithNoSubsystem[0].IsValid());
	}

	// ── MULTI-KEY ROWS DERIVE EVERY KEY, NOT JUST THE FIRST ─────────────────────────────
	// Hero.Move carries four and Cards.Play carries six; deriving only [0] would be invisible
	// on a one-key row and wrong on every real one.
	const FSiegeControlsHelpAction MoveRow =
		MakeRow(ESiegeInputLane::MappedAction, { EKeys::W, EKeys::A, EKeys::S, EKeys::D }, false, false);
	const TArray<FKey> MoveOnDvorak =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(MoveRow, TArray<FKey>(), Dvorak.Layout.Get());

	TestEqual(TEXT("A four-key row derives all FOUR labels"), MoveOnDvorak.Num(), 4);
	for (int32 Index = 0; Index < MoveOnDvorak.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("Move key %d derives through the accessor"), Index),
			MoveOnDvorak[Index] == Dvorak.Layout->GetPositionalKey(MoveRow.QwertyReferenceKeys[Index]));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 4 — Siegebound.ControlsHelp.RawLanesAreIdentity
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  LANE B IS AN IDENTITY BY CONSTRUCTION, AND LANE C IS `KBD-§8`'s PINNED LITERAL — AND THE
 *  F-1 REVERSAL IS GENUINELY ONE FLAG.
 *
 *  ⭐ THE SECOND HALF IS THE ONE THAT EARNS ITS KEEP: TASK-704 §8 F-1 records that Jonathan's
 *  own `HELP-§1` (derive) and `KBD-§8` (literal `Z` on every layout) point opposite ways on
 *  exactly one row, and that TASK-706 must "keep it a one-flag reversal". This test proves the
 *  flag really is the whole reversal by driving BOTH states — so if he rules the other way,
 *  the change is one boolean and this test's second block becomes the shipped expectation.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpRawLanesTest,
	"Siegebound.ControlsHelp.RawLanesAreIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpRawLanesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());
	if (!Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the scratch layout subsystem - the raw-lane claims are untested."));
		return false;
	}

	// ── LANE B: mouse buttons, Escape and the wheel are absent from the 26-letter table ──
	const TArray<FKey> RawKeys =
	{
		EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::Escape,
		EKeys::MouseScrollUp, EKeys::MouseScrollDown, EKeys::Mouse2D
	};

	const FSiegeControlsHelpAction RawRow = MakeRow(ESiegeInputLane::RawNonLetter, RawKeys, false, false);
	const TArray<FKey> RawResolved =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(RawRow, TArray<FKey>(), Dvorak.Layout.Get());

	TestEqual(TEXT("A raw non-letter row labels every one of its keys"), RawResolved.Num(), RawKeys.Num());
	for (int32 Index = 0; Index < RawResolved.Num() && Index < RawKeys.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("%s is UNCHANGED on a Dvorak layout (it is not a letter)"),
			*Describe(RawKeys[Index])), RawResolved[Index] == RawKeys[Index]);

		// ⭐ AND THE IDENTITY IS PROVEN, NOT ASSUMED: the accessor itself agrees, which is what
		// makes "we skip the call because it would be a no-op" a fact rather than a hope.
		TestTrue(FString::Printf(TEXT("...and GetPositionalKey agrees it is an identity (%s)"),
			*Describe(RawKeys[Index])),
			Dvorak.Layout->GetPositionalKey(RawKeys[Index]) == RawKeys[Index]);
	}

	// ── LANE C, THE SHIPPED STATE: `KBD-§8`'s literal ───────────────────────────────────
	const FSiegeControlsHelpAction LiteralRow = MakeRow(ESiegeInputLane::RawLetter, { EKeys::Z }, /*bLiteral=*/true, false);
	const TArray<FKey> LiteralResolved =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(LiteralRow, TArray<FKey>(), Dvorak.Layout.Get());

	// FIXTURE SELF-CHECK: without a map that actually moves this position, the claim below is
	// vacuous. (A claim about the FIXTURE, not about a label.)
	const FKey ZTranslated = Dvorak.Layout->GetPositionalKey(EKeys::Z);
	TestTrue(TEXT("FIXTURE: the injected map DOES move the accept key's position on Dvorak"), ZTranslated != EKeys::Z);

	TestEqual(TEXT("The pinned accept row labels exactly one key"), LiteralResolved.Num(), 1);
	if (LiteralResolved.Num() == 1)
	{
		TestTrue(FString::Printf(
			TEXT("⛔ KBD-§8: the accept key's LABEL is the literal reference key on every layout (got %s, the translated position is %s)"),
			*Describe(LiteralResolved[0]), *Describe(ZTranslated)),
			LiteralResolved[0] == EKeys::Z);
	}

	// ── LANE C, THE F-1 REVERSAL: the SAME row with the ONE flag cleared ────────────────
	const FSiegeControlsHelpAction DerivedRow = MakeRow(ESiegeInputLane::RawLetter, { EKeys::Z }, /*bLiteral=*/false, false);
	const TArray<FKey> DerivedResolved =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(DerivedRow, TArray<FKey>(), Dvorak.Layout.Get());

	TestEqual(TEXT("The reversal case labels exactly one key"), DerivedResolved.Num(), 1);
	if (DerivedResolved.Num() == 1)
	{
		TestTrue(TEXT("⭐ Clearing bLiteralKeyLabel makes the row DERIVE - the F-1 reversal really is one flag"),
			DerivedResolved[0] == ZTranslated);
	}

	// ── THE "EXACTLY ONE TRANSLATION" CLAIM, MADE WHERE IT CAN ACTUALLY BE MEASURED ─────
	// ⛔⛔ IT CANNOT BE MADE AT THE `Z` POSITION, AND SAYING SO IS THE POINT (TASK-719).
	// `Z`'s image is `Semicolon`, and the translation table's domain is the 26 LETTERS and
	// nothing else (SiegeKeyboardLayoutStatics.cpp:57-63; the contract that an absent key
	// returns UNCHANGED is SiegeKeyboardLayoutSubsystem.h:238-240, and Lane B above proves
	// the same identity on non-letters). ⇒ GetPositionalKey(Semicolon) IS Semicolon, so a
	// SECOND application at that position is indistinguishable from the first: an assertion
	// written there reduces to `Semicolon != Semicolon`, which cannot pass however correct
	// the code is — and, worse, could not have caught the defect if it had.
	// ⭐ So the claim is made on the SAME Lane-C derive path, on the one key whose image has
	// a DISTINCT onward hop: `F` -> `U`, and `U` -> `G` (test 2's idiom, and the exact
	// double-translation SiegeControlsHelpWidget.cpp:1150-1152 names as the defect).
	const FSiegeControlsHelpAction DerivedHopRow =
		MakeRow(ESiegeInputLane::RawLetter, { EKeys::F }, /*bLiteral=*/false, false);
	const TArray<FKey> DerivedHopResolved =
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(DerivedHopRow, TArray<FKey>(), Dvorak.Layout.Get());

	const FKey OneHop  = Dvorak.Layout->GetPositionalKey(EKeys::F);
	const FKey TwoHops = Dvorak.Layout->GetPositionalKey(OneHop);

	// FIXTURE SELF-CHECK: the second hop must land SOMEWHERE ELSE, or the claim below is as
	// vacuous as the `Z` one was. (A claim about the FIXTURE, not about a label.)
	TestTrue(FString::Printf(
		TEXT("FIXTURE: the derived key's image has a DISTINCT onward hop, so a double translation is VISIBLE (%s -> %s -> %s)"),
		*Describe(EKeys::F), *Describe(OneHop), *Describe(TwoHops)),
		TwoHops != OneHop);

	TestEqual(TEXT("The derived raw-letter row labels exactly one key"), DerivedHopResolved.Num(), 1);
	if (DerivedHopResolved.Num() == 1)
	{
		TestTrue(FString::Printf(
			TEXT("...and it is EXACTLY ONE translation, never two (got %s; one hop is %s, two hops would be %s)"),
			*Describe(DerivedHopResolved[0]), *Describe(OneHop), *Describe(TwoHops)),
			DerivedHopResolved[0] == OneHop && DerivedHopResolved[0] != TwoHops);
	}

	// ── LANE D: no key at all ───────────────────────────────────────────────────────────
	const FSiegeControlsHelpAction PointerRow = MakeRow(ESiegeInputLane::PointerOnly, TArray<FKey>(), false, /*bPointer=*/true);
	TestEqual(TEXT("A pointer-only row yields NO keys"),
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(PointerRow, TArray<FKey>(), Dvorak.Layout.Get()).Num(), 0);

	// ⛔ And it stays keyless even if something upstream wrongly handed it applied keys - the
	// lane is the authority, not the argument.
	TestEqual(TEXT("A pointer-only row ignores applied keys entirely"),
		FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(PointerRow, { EKeys::F }, Dvorak.Layout.Get()).Num(), 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 5 — Siegebound.ControlsHelp.NoRowRendersBlank
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ NO ROW EVER RENDERS BLANK (`HELP-§2` mechanism 2, `HELP-§6` criterion 4).
 *  ⚖️ A VISIBLE GAP GETS FIXED; A SILENT OMISSION DOES NOT.
 *
 *  ⚠️ THE DETAIL CLAIM IS DELIBERATELY "NEVER EMPTY" RATHER THAN "IS THE TODO STRING": when
 *  TASK-706 wrote this test the Detail field was empty on every row by design, and a test that
 *  asserted the TODO string would have had to be EDITED by TASK-707 the moment it did its job.
 *  ⭐ It was not: TASK-707 filled all 24 rows from TASK-704 §4 and this assertion held unchanged
 *  across the change, which is the whole point of wording it this way.
 *  ⇒ The stronger claim — that every shipped row is genuinely AUTHORED rather than falling back
 *  to the TODO string — is test 9, kept separate because the two fail for different reasons.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpNoBlankRowTest,
	"Siegebound.ControlsHelp.NoRowRendersBlank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpNoBlankRowTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	const FString UndocumentedString(FSiegeControlsHelpRegistry::GetUndocumentedText());
	TestFalse(TEXT("The pinned gap string is itself non-empty"), UndocumentedString.IsEmpty());

	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		const FString RowName = Row.ActionId.ToString();

		TestFalse(*FString::Printf(TEXT("Row '%s' renders a non-empty one-liner"), *RowName),
			FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Row).IsEmptyOrWhitespace());

		TestFalse(*FString::Printf(TEXT("Row '%s' renders non-empty detail text"), *RowName),
			FSiegeControlsHelpRegistry::ComposeDetailForDisplay(Row).IsEmptyOrWhitespace());

		TestFalse(*FString::Printf(TEXT("Row '%s' has a non-empty category header"), *RowName),
			FSiegeControlsHelpRegistry::GetCategoryDisplayText(Row.Category).IsEmptyOrWhitespace());

		// ⛔ EVERY SHIPPED ROW IS ACTUALLY AUTHORED. The TODO string is a fail-safe for a
		// command added later, ⛔ not a place-holder anyone may ship a row on.
		TestNotEqual(*FString::Printf(TEXT("Row '%s' one-liner is AUTHORED, not the TODO fallback"), *RowName),
			FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Row).ToString(), UndocumentedString);
	}

	// ── THE FALLBACK ITSELF, DRIVEN DELIBERATELY ────────────────────────────────────────
	FSiegeControlsHelpAction Undocumented = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);
	Undocumented.OneLine = FText::GetEmpty();
	Undocumented.Detail  = FText::GetEmpty();

	TestEqual(TEXT("An undocumented one-liner renders the pinned TODO string, never a blank"),
		FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Undocumented).ToString(), UndocumentedString);
	TestEqual(TEXT("Undocumented detail renders the pinned TODO string, never a blank page"),
		FSiegeControlsHelpRegistry::ComposeDetailForDisplay(Undocumented).ToString(), UndocumentedString);

	// Whitespace-only is the same failure wearing a disguise.
	Undocumented.OneLine = FText::FromString(TEXT("   "));
	TestEqual(TEXT("A whitespace-only one-liner is treated as undocumented too"),
		FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Undocumented).ToString(), UndocumentedString);

	// An unknown category still renders SOMETHING rather than an invisible group of rows.
	TestFalse(TEXT("An unknown category renders its own name rather than nothing"),
		FSiegeControlsHelpRegistry::GetCategoryDisplayText(FName(TEXT("Zzz.Unknown"))).IsEmptyOrWhitespace());

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 6 — Siegebound.ControlsHelp.EscapeIsNotClaimed
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔⛔ `Escape` IS DOCUMENTATION HERE AND A HANDLER NOWHERE (`AS-§6 A-2` — a PERMANENT
 *  Jonathan ruling; `HELP-§5`; `HELP-§6` criterion 5).
 *
 *  ⚠️⚠️ WHAT THIS TEST CAN AND CANNOT PROVE, STATED PLAINLY (`SC-§32`) — because a test named
 *  "EscapeIsNotClaimed" that quietly proved less than its name would be worse than no test:
 *    ✅ IT CAN prove that `Escape` appears in the registry ONLY as the two shipped cancel
 *       rows' documented key, and nowhere else in the data.
 *    ✅ IT CAN prove, by reflection, that neither widget class declares a Blueprint-facing key
 *       handler or holds an FKey member (which would also be a `KBD-§0` ruling-2 cache).
 *    ⛔ IT CANNOT see a C++ `NativeOnKeyDown` / `NativeOnPreviewKeyDown` override — a virtual
 *       override is not reflected. ⇒ **THE AUTHORITATIVE GATE IS TASK-708 CRITERION 4's GREP
 *       OF THE DIFF**, and this test is corroboration, not the proof.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpEscapeTest,
	"Siegebound.ControlsHelp.EscapeIsNotClaimed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpEscapeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	// ── (a) `Escape` IN THE DATA: the two shipped cancel rows, and only those ───────────
	TArray<FName> RowsNamingEscape;
	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		if (Row.QwertyReferenceKeys.Contains(EKeys::Escape))
		{
			RowsNamingEscape.Add(Row.ActionId);
		}
	}

	TestTrue(TEXT("Cards.Cancel documents Escape as the shipped cancel gesture"),
		RowsNamingEscape.Contains(FName(TEXT("Cards.Cancel"))));
	TestTrue(TEXT("PickMode.Cancel documents Escape as 'how to exit the command'"),
		RowsNamingEscape.Contains(FName(TEXT("PickMode.Cancel"))));
	TestEqual(TEXT("⛔ NO OTHER row names Escape - in particular Interface.ControlsHelp does not claim it"),
		RowsNamingEscape.Num(), 2);

	const FSiegeControlsHelpAction* const HelpRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.ControlsHelp")));
	if (HelpRow != nullptr)
	{
		TestFalse(TEXT("⛔ The overlay's own row does NOT list Escape as one of its keys"),
			HelpRow->QwertyReferenceKeys.Contains(EKeys::Escape));
	}

	// ── (b) THE REFLECTION SWEEP ────────────────────────────────────────────────────────
	// ⭐ ALL THREE `HELP-§3` CLASSES, ⛔ not two: TASK-707's full-screen detail view is the newest
	// place a key handler could hide, and it is the one view a player might expect `Escape` to
	// close. The sweep is extended here rather than duplicated in a fourth test, so there stays
	// exactly ONE place that answers "does this feature claim Escape?".
	const UClass* const Classes[] =
	{
		USiegeControlsHelpWidget::StaticClass(),
		USiegeControlsHelpRowWidget::StaticClass(),
		USiegeControlsDetailWidget::StaticClass()
	};

	for (const UClass* Class : Classes)
	{
		if (Class == nullptr)
		{
			AddError(TEXT("A controls-help class did not resolve - the reflection sweep is untested."));
			continue;
		}

		// ⛔ NO FKey MEMBER ANYWHERE. This is two laws at once: a widget holding an FKey is a
		// key cached across opens (`KBD-§0` ruling 2), and it is also the shape a key handler
		// would need to compare against.
		// ⚠️ The sweep runs FIRST so its offender name can be named in the failure message.
		FString Offender;
		const bool bHasKeyProperty = ClassDeclaresAnyKeyProperty(Class, Offender);
		TestFalse(*FString::Printf(TEXT("%s declares NO FKey property (no cached key, no key to compare against) - found '%s'"),
			*Class->GetName(), *Offender),
			bHasKeyProperty);

		// ⛔ NO BLUEPRINT-FACING KEY HANDLER. UUserWidget's own OnKeyDown/OnPreviewKeyDown
		// BlueprintImplementableEvents are inherited, so the sweep is restricted to functions
		// DECLARED on these classes (EFieldIterationFlags::None excludes super-class fields).
		for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIterationFlags::None); FunctionIt; ++FunctionIt)
		{
			const FString FunctionName = FunctionIt->GetName();
			const bool bIsKeyHandler =
				FunctionName.Contains(TEXT("OnKeyDown")) ||
				FunctionName.Contains(TEXT("OnKeyUp")) ||
				FunctionName.Contains(TEXT("PreviewKey"));

			TestFalse(*FString::Printf(TEXT("%s declares no reflected key handler (found '%s')"),
				*Class->GetName(), *FunctionName), bIsKeyHandler);
		}
	}

	AddInfo(TEXT("SCOPE (SC-§32): this test proves Escape is absent from the registry's keys and that neither widget class ")
		TEXT("declares a reflected key handler or an FKey member. A C++ NativeOnKeyDown / NativeOnPreviewKeyDown override is ")
		TEXT("NOT reflected and cannot be seen from here - TASK-708 criterion 4's grep of the diff is the authoritative gate ")
		TEXT("for AS-§6 A-2, and this test is corroboration."));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 7 — Siegebound.ControlsHelp.RowClickSeamReportsActionId   ⭐ THE TASK-707 SEAM
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE SEAM TASK-707 BUILDS AGAINST, ASSERTED AS A BEHAVIOUR (spec item 7: this task ends
 *  at "the row is clickable and reports the click").
 *
 *  ⚠️ NO SLATE TREE IS BUILT HERE and none is needed: ActivateRow() is the same entry point
 *  the button's OnClicked thunk calls, so driving it directly tests the seam rather than
 *  UMG's click plumbing (which is Slate's to get right, not this project's).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpRowSeamTest,
	"Siegebound.ControlsHelp.RowClickSeamReportsActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpRowSeamTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<USiegeControlsHelpRowWidget> Row(
		NewObject<USiegeControlsHelpRowWidget>(GetTransientPackageAsObject()));

	if (!Row.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeControlsHelpRowWidget - the TASK-707 seam is untested."));
		return false;
	}

	int32 FireCount = 0;
	FName Reported  = NAME_None;

	Row->OnRowActivated.BindLambda([&FireCount, &Reported](FName ActivatedId)
	{
		++FireCount;
		Reported = ActivatedId;
	});

	// ── AN UNSTAMPED ROW REPORTS NOTHING ────────────────────────────────────────────────
	// ⛔ A construction bug must never reach the receiver looking like a player action.
	TestEqual(TEXT("An unstamped row reports NAME_None from GetActionId"), Row->GetActionId(), FName(NAME_None));
	Row->ActivateRow();
	TestEqual(TEXT("⛔ An unstamped row does NOT fire the seam"), FireCount, 0);

	// ── STAMPED: the seam carries the id, and only the id ───────────────────────────────
	const FName ExpectedId(TEXT("Orders.Ambush"));
	Row->SetRowContent(ExpectedId,
		FText::FromString(TEXT("Ambush")),
		FText::FromString(TEXT("A one-line description.")),
		FText::FromString(TEXT("A key chip composed elsewhere.")));

	TestEqual(TEXT("The stamp lands on GetActionId"), Row->GetActionId(), ExpectedId);

	Row->ActivateRow();
	TestEqual(TEXT("A stamped row fires the seam exactly once"), FireCount, 1);
	TestEqual(TEXT("⭐ ...carrying the row's registry ActionId"), Reported, ExpectedId);

	// The id the seam reports must be resolvable by the overlay's own lookup, or
	// HandleRowActivated would drop it - the two halves have to agree.
	TestNotNull(TEXT("The reported id resolves in the registry (HandleRowActivated's own guard)"),
		FSiegeControlsHelpRegistry::FindAction(Reported));

	// ── UNBINDING IS CLEAN ──────────────────────────────────────────────────────────────
	// The overlay unbinds every row before releasing it (ClearRowWidgets), so a row must not
	// keep firing after that.
	Row->OnRowActivated.Unbind();
	Row->ActivateRow();
	TestEqual(TEXT("An unbound row fires nothing"), FireCount, 1);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 8 — Siegebound.ControlsHelp.KeyChipComposition
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  THE CHIP IS ALWAYS SOMETHING, AND ON A REAL REGISTRY IT IS ALWAYS DERIVED.
 *
 *  ⭐ THE END-TO-END ASSERTION IS THE LAST BLOCK: run EVERY shipped row through the whole
 *  pipeline (lane resolve -> chip compose) on BOTH simulated layouts and require that at least
 *  one chip CHANGES. An implementation that hardcoded its letters would produce two identical
 *  lists and go red here — which is the same defect test 3 catches per-key, caught again at
 *  the level the player actually sees.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpChipTest,
	"Siegebound.ControlsHelp.KeyChipComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpChipTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	// ── LANE D: the pointer affordance, ⛔ never an empty box and ⛔ never a key ─────────
	const FSiegeControlsHelpAction PointerRow = MakeRow(ESiegeInputLane::PointerOnly, TArray<FKey>(), false, true);
	const FText PointerChip = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(PointerRow, TArray<FKey>());
	TestFalse(TEXT("A pointer-only row renders a NON-EMPTY chip"), PointerChip.IsEmptyOrWhitespace());

	// ── NO KEYS ON A KEYED ROW: the explicit "not bound" chip (HELP-§2 mechanism 2) ─────
	const FSiegeControlsHelpAction UnboundRow = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);
	const FText UnboundChip = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(UnboundRow, TArray<FKey>());
	TestFalse(TEXT("A row with no resolvable key renders a NON-EMPTY chip - the row is never hidden"),
		UnboundChip.IsEmptyOrWhitespace());
	TestNotEqual(TEXT("...and it is distinguishable from the pointer chip"),
		UnboundChip.ToString(), PointerChip.ToString());

	// ── MULTI-KEY: every key appears, F-2's declared one-chip presentation ──────────────
	const FSiegeControlsHelpAction TwoKeyRow =
		MakeRow(ESiegeInputLane::RawNonLetter, { EKeys::RightMouseButton, EKeys::Escape }, false, false);
	const FString TwoKeyChip =
		FSiegeControlsHelpRegistry::ComposeKeyChipLabel(TwoKeyRow, TwoKeyRow.QwertyReferenceKeys).ToString();

	// ⛔ Asserted by ASKING THE FKey for its own display name, ⛔ never by typing "RMB".
	TestTrue(TEXT("A two-key chip contains the first key's own display name"),
		TwoKeyChip.Contains(EKeys::RightMouseButton.GetDisplayName(false).ToString()));
	TestTrue(TEXT("...and the second key's"),
		TwoKeyChip.Contains(EKeys::Escape.GetDisplayName(false).ToString()));
	TestTrue(TEXT("...and it is longer than either alone (they are joined, not replaced)"),
		TwoKeyChip.Len() > EKeys::Escape.GetDisplayName(false).ToString().Len());

	// ── ⭐ END-TO-END OVER THE REAL REGISTRY, ON BOTH LAYOUTS ───────────────────────────
	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());
	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());

	if (!Qwerty.IsValid() || !Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the two scratch layout subsystems - the end-to-end chip claim is untested."));
		return false;
	}

	int32 ChangedChips = 0;
	int32 HeldChips    = 0;

	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		const FString RowName = Row.ActionId.ToString();

		// The fallback lane on purpose: with no world there is no applied context, which is
		// also exactly the state on a machine where the IA_* assets have not landed.
		const FString OnQwerty = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(
			Row, FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, TArray<FKey>(), Qwerty.Layout.Get())).ToString();
		const FString OnDvorak = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(
			Row, FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, TArray<FKey>(), Dvorak.Layout.Get())).ToString();

		TestFalse(*FString::Printf(TEXT("Row '%s' renders a non-empty chip on QWERTY"), *RowName), OnQwerty.IsEmpty());
		TestFalse(*FString::Printf(TEXT("Row '%s' renders a non-empty chip on Dvorak"), *RowName), OnDvorak.IsEmpty());

		if (OnQwerty != OnDvorak)
		{
			++ChangedChips;
		}
		else
		{
			++HeldChips;
		}
	}

	// ⭐ THE CLAIM THAT A HARDCODED IMPLEMENTATION CANNOT SATISFY.
	TestTrue(TEXT("⭐ At least one shipped row's CHIP changes between the two layouts - the pipeline is live end to end"),
		ChangedChips > 0);

	// ⭐ AND THE OTHER HALF, WHICH A "TRANSLATE EVERYTHING" IMPLEMENTATION CANNOT SATISFY:
	// the mouse rows, the pointer rows, the pinned accept key and the non-letter keys must all
	// hold. Both counts being non-zero is what makes the pair meaningful.
	TestTrue(TEXT("⭐ ...and at least one row's chip HOLDS - nothing is translated indiscriminately"),
		HeldChips > 0);

	AddInfo(FString::Printf(TEXT("Chip census across the simulated layout flip: %d changed, %d held, %d rows total."),
		ChangedChips, HeldChips, FSiegeControlsHelpRegistry::GetActions().Num()));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TESTS 9-13 — THE FULL-SCREEN DETAIL VIEW (TASK-707)
//
//  ⚠️ WHAT THESE FIVE CANNOT PROVE, SAID FIRST (`SC-§32`): they never build a Slate tree, so
//  ⛔ they cannot prove the page RENDERS, that it fills the screen, or that it is legible.
//  That closes on pixels or Jonathan's eyes and on nothing else (`AS-§6` A(e), `HELP-§3`).
//  What they DO prove is everything that can go wrong silently: that the prose is really there,
//  that no key in it is a typed letter, that Jonathan's three named questions are reachable from
//  the pages he will click, and that the way OUT of a page exists before the page does.
// ════════════════════════════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 9 — Siegebound.ControlsHelp.EveryRowHasAuthoredDetail
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE TEST THAT WOULD HAVE BEEN RED BEFORE TASK-707 AND MUST NEVER BE RED AGAIN
 *  (`HELP-§2` mechanism 2; the board's item 6: "every row has reachable detail content").
 *
 *  Test 5 asserts detail is NEVER EMPTY — which was already true when the field was empty on all
 *  24 rows, because ComposeDetailForDisplay substitutes the TODO string. THIS test asserts the
 *  stronger claim that the substitution is not happening: every shipped row is genuinely
 *  AUTHORED. ⚖️ The two are deliberately separate, because they fail for different reasons and a
 *  reviewer should be able to tell "no page is blank" from "no page is a placeholder".
 *
 *  ⭐ IT ALSO MACHINE-CHECKS THE TRANSFER RULES from handoffs/TASK-704-programmer.md §4 that a
 *  human would otherwise have to eyeball across 24 strings: ⛔ no `file:line` citation and ⛔ no
 *  markdown markup may reach the player's screen (rules T1 and T2). Those live in C++ comments.
 *
 *  ⛔⛔ WHAT THIS TEST NO LONGER DOES, ⛔ SAID IN THE DOCSTRING BECAUSE THE DOCSTRING IS WHERE THE
 *  LAST TWO READERS LOOKED AND FOUND NOTHING: it used to ALSO carry the registry-wide
 *  `RelatedActionIds` graph walk, which is nowhere in its name and was nowhere in this comment.
 *  ⇒ TASK-852 MOVED it to TEST 18, `EveryRelatedActionIdResolvesToARealRow`. ⛔ It was not
 *  weakened and it was not duplicated — it was made findable, and given a negative control.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpAuthoredDetailTest,
	"Siegebound.ControlsHelp.EveryRowHasAuthoredDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpAuthoredDetailTest::RunTest(const FString& Parameters)
{
	const FString UndocumentedString(FSiegeControlsHelpRegistry::GetUndocumentedText());

	// ⛔ Fragments that mark prose written for a DEVELOPER rather than for a player. Every one of
	// these belongs in a C++ comment beside the string (TASK-704 §4's citations, rule T1) or is
	// markdown that only means something in a .md file (rule T2).
	const TCHAR* ForbiddenInPlayerProse[] =
	{
		TEXT(".cpp:"), TEXT(".h:"), TEXT("handoffs/"), TEXT("TASK-"), TEXT("SPC:"),
		TEXT("**"), TEXT("`"), TEXT("§")
	};

	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		const FString RowName = Row.ActionId.ToString();
		const FString Detail  = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(Row).ToString();

		TestFalse(*FString::Printf(TEXT("Row '%s' has detail text at all"), *RowName), Detail.IsEmpty());

		// ⭐ THE CLAIM THIS TEST EXISTS FOR.
		TestNotEqual(*FString::Printf(TEXT("Row '%s' detail is AUTHORED, not the TODO fallback"), *RowName),
			Detail, UndocumentedString);

		// A "detail" page that only repeated the one-liner would satisfy every other assertion in
		// this file while giving the player nothing he did not already have on the row.
		const FString OneLine = FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Row).ToString();
		TestNotEqual(*FString::Printf(TEXT("Row '%s' detail is not just its one-liner again"), *RowName),
			Detail, OneLine);
		TestTrue(*FString::Printf(TEXT("Row '%s' detail is substantially longer than its one-liner (%d vs %d chars)"),
			*RowName, Detail.Len(), OneLine.Len()), Detail.Len() > OneLine.Len());

		for (const TCHAR* Forbidden : ForbiddenInPlayerProse)
		{
			TestFalse(*FString::Printf(TEXT("Row '%s' detail carries no developer-only fragment '%s' (T1/T2: citations and markup live in comments)"),
				*RowName, Forbidden), Detail.Contains(Forbidden, ESearchCase::CaseSensitive));
		}

		// ⛔⛔ MOVED OUT 2026-09-03 (TASK-852) — ⛔ NOT DELETED. The registry-wide
		// `RelatedActionIds` walk that used to live here is now TEST 18,
		// `Siegebound.ControlsHelp.EveryRelatedActionIdResolvesToARealRow`. It walks the SAME
		// `GetActions()` set, resolves through the SAME `FSiegeControlsHelpRegistry::FindAction`,
		// and now carries a NEGATIVE CONTROL proving it can go red.
		// ⚖️ WHY IT MOVED, because it is the instructive part: it was CORRECT and it was
		// INVISIBLE. Filed under a name that says "detail authoring", it was read as absent by
		// TWO independent readers — `HELP-§7` was written declaring it did not exist, and
		// `TASK-852` was boarded to build a duplicate of it. ⇒ `SC-§40` cl. 11(b) and
		// `HELP-§7`'s own closing clause: A TEST'S NAME IS ITS DISCOVERABILITY SURFACE, and an
		// assertion filed under an unrelated name is operationally an assertion that does not
		// exist. ⛔ Do not re-add an edge loop here — the walk lives in exactly ONE place.
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 10 — Siegebound.ControlsHelp.DetailKeysDeriveThroughTheAccessor   ⭐⭐ THE KEYSTONE
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ `HELP-§1` APPLIED TO THE DETAIL LANE — the board's item 3, and the defect that would be
 *  invisible to every reviewer on this project.
 *
 *  TASK-704 §4 typed the letters `T` and `E` inside four sentences (the Attack/Defend orders on
 *  the Hold, Ambush, Follow and pick-exit pages). US-Dvorak moves those two positions to `Y` and
 *  `.`. ⇒ transferring them literally would have printed the WRONG KEYS in the middle of a
 *  paragraph on the very screen whose chips exist to print the right ones — and it would look
 *  perfect on this machine.
 *
 *  ⛔ EVERY CLAIM BELOW IS MADE AGAINST WHAT THE ACCESSOR ANSWERS, ⛔ never against a typed
 *  letter. A `TestTrue(Body.Contains(TEXT("Y")))` would pass on an implementation that hardcoded
 *  `Y`, which is the same class of defect one layout over.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpDetailDerivationTest,
	"Siegebound.ControlsHelp.DetailKeysDeriveThroughTheAccessor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpDetailDerivationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());
	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());

	if (!Qwerty.IsValid() || !Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the two scratch layout subsystems - the detail-lane derivation claim is untested."));
		return false;
	}

	// The fallback lane deliberately: with no world there is no applied context, so every key on
	// the page comes through the ONE translation ResolveRowDisplayKeys performs. That is also the
	// real state on a machine where the IA_* assets have not landed.
	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };

	// ── (a) THE PAGE JONATHAN NAMED, ON BOTH LAYOUTS ────────────────────────────────────
	const FSiegeControlsHelpAction* const AmbushRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Orders.Ambush")));
	if (AmbushRow == nullptr)
	{
		AddError(TEXT("Orders.Ambush is missing from the registry - the detail-lane derivation claim is untested."));
		return false;
	}

	const FString AmbushOnQwerty =
		FSiegeControlsHelpRegistry::ComposeDetailContent(*AmbushRow, Qwerty.Layout.Get(), NoAppliedKeys).Body.ToString();
	const FString AmbushOnDvorak =
		FSiegeControlsHelpRegistry::ComposeDetailContent(*AmbushRow, Dvorak.Layout.Get(), NoAppliedKeys).Body.ToString();

	// ⭐ THE CLAIM A LITERAL TRANSFER CANNOT SATISFY.
	TestNotEqual(TEXT("⭐ The Ambush page's PROSE changes between the two layouts - the keys inside it are derived, not typed"),
		AmbushOnQwerty, AmbushOnDvorak);

	// ⛔ Asserted by asking the accessor and then the FKey, exactly as the chip tests do. The
	// fixture self-check first, so "it changed" cannot be a vacuous claim.
	const FKey AttackOnQwerty = Qwerty.Layout->GetPositionalKey(EKeys::T);
	const FKey AttackOnDvorak = Dvorak.Layout->GetPositionalKey(EKeys::T);
	TestNotEqual(TEXT("FIXTURE SELF-CHECK: the injected map really does move the Attack order's key"),
		Describe(AttackOnQwerty), Describe(AttackOnDvorak));

	TestTrue(TEXT("The QWERTY page names the accessor's OWN answer for the Attack order"),
		AmbushOnQwerty.Contains(AttackOnQwerty.GetDisplayName(false).ToString(), ESearchCase::CaseSensitive));
	TestTrue(TEXT("⭐ ...and the Dvorak page names the accessor's DIFFERENT answer, in the same sentence"),
		AmbushOnDvorak.Contains(AttackOnDvorak.GetDisplayName(false).ToString(), ESearchCase::CaseSensitive));

	// ── (b) NO UNRESOLVED TOKEN SURVIVES ON ANY SHIPPED PAGE, ON EITHER LAYOUT ──────────
	// ⚠️ A misspelled token is deliberately NOT deleted by the resolver (a visible gap gets
	// fixed), so the only thing standing between a typo and "{Orders.Attak}" on Jonathan's screen
	// is this assertion.
	int32 PagesWithTokens = 0;
	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		const FString RowName = Row.ActionId.ToString();

		if (Row.Detail.ToString().Contains(TEXT("{"), ESearchCase::CaseSensitive))
		{
			++PagesWithTokens;
		}

		const FSiegeControlsDetailContent OnQwerty =
			FSiegeControlsHelpRegistry::ComposeDetailContent(Row, Qwerty.Layout.Get(), NoAppliedKeys);
		const FSiegeControlsDetailContent OnDvorak =
			FSiegeControlsHelpRegistry::ComposeDetailContent(Row, Dvorak.Layout.Get(), NoAppliedKeys);

		TestFalse(*FString::Printf(TEXT("Row '%s' leaves no unresolved token in its body on QWERTY"), *RowName),
			OnQwerty.Body.ToString().Contains(TEXT("{"), ESearchCase::CaseSensitive));
		TestFalse(*FString::Printf(TEXT("Row '%s' leaves no unresolved token in its body on Dvorak"), *RowName),
			OnDvorak.Body.ToString().Contains(TEXT("{"), ESearchCase::CaseSensitive));

		for (const FSiegeControlsDetailEntry& Entry : OnDvorak.Related)
		{
			TestFalse(*FString::Printf(TEXT("Row '%s' related block '%s' leaves no unresolved token"),
				*RowName, *Entry.ActionId.ToString()),
				Entry.Body.ToString().Contains(TEXT("{"), ESearchCase::CaseSensitive));
		}
	}

	// ⭐ AND THE PAIR THAT MAKES THE ABOVE MEANINGFUL: at least one page really does carry a
	// token. Without this, "no page leaves a token behind" would be trivially true of a build in
	// which the tokens had been quietly replaced with typed letters.
	TestTrue(TEXT("⭐ At least one shipped page carries a {ActionId} token - the mechanism is in use, not decorative"),
		PagesWithTokens > 0);

	// ── (c) A PAGE WITH NO TOKEN MUST HOLD (the other half, per HELP-§6) ────────────────
	const FSiegeControlsHelpAction* const JumpRow = FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Hero.Jump")));
	if (JumpRow != nullptr)
	{
		TestEqual(TEXT("⭐ A page whose prose names no movable key is IDENTICAL on both layouts - nothing is translated indiscriminately"),
			FSiegeControlsHelpRegistry::ComposeDetailContent(*JumpRow, Qwerty.Layout.Get(), NoAppliedKeys).Body.ToString(),
			FSiegeControlsHelpRegistry::ComposeDetailContent(*JumpRow, Dvorak.Layout.Get(), NoAppliedKeys).Body.ToString());
	}

	// ── (d) THE RESOLVER ITSELF, DRIVEN DIRECTLY ────────────────────────────────────────
	int32 ProviderCalls = 0;
	auto CountingProvider = [&ProviderCalls](FName ChipActionId) -> FText
	{
		++ProviderCalls;
		return FText::FromString(FString(TEXT("<chip:")) + ChipActionId.ToString() + FString(TEXT(">")));
	};

	const FString RealToken = FSiegeControlsHelpRegistry::MakeActionToken(FName(TEXT("Orders.Attack")));
	const FText Resolved = FSiegeControlsHelpRegistry::ResolveDetailTokens(
		FText::FromString(FString(TEXT("Press ")) + RealToken + FString(TEXT(" now."))), CountingProvider);

	TestFalse(TEXT("A real token is replaced"), Resolved.ToString().Contains(RealToken, ESearchCase::CaseSensitive));
	TestTrue(TEXT("...by the provider's answer"),
		Resolved.ToString().Contains(TEXT("<chip:Orders.Attack>"), ESearchCase::CaseSensitive));
	TestEqual(TEXT("...and the provider was asked exactly once"), ProviderCalls, 1);

	// ⛔ AN ID THAT IS NOT IN THE REGISTRY IS LEFT ALONE, ⛔ NOT DELETED: a typo must reach a
	// reviewer's eye rather than silently removing a sentence's subject (`HELP-§2` mechanism 2).
	const FString BadToken = FSiegeControlsHelpRegistry::MakeActionToken(FName(TEXT("Nope.NotAnAction")));
	const FText Untouched = FSiegeControlsHelpRegistry::ResolveDetailTokens(
		FText::FromString(FString(TEXT("Press ")) + BadToken + FString(TEXT(" now."))), CountingProvider);
	TestTrue(TEXT("⛔ A token naming no registry row survives VISIBLY rather than vanishing"),
		Untouched.ToString().Contains(BadToken, ESearchCase::CaseSensitive));

	// Prose with no token at all must not pay for a single provider call.
	const int32 CallsBefore = ProviderCalls;
	FSiegeControlsHelpRegistry::ResolveDetailTokens(FText::FromString(TEXT("No tokens here.")), CountingProvider);
	TestEqual(TEXT("Token-free prose asks the provider nothing"), ProviderCalls, CallsBefore);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 11 — Siegebound.ControlsHelp.DetailContentAnswersTheNamedQuestions
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ JONATHAN'S ACCEPTANCE SENTENCE, AS FAR AS A MACHINE CAN CARRY IT:
 *  "…all the controls with it such as what the first, second, and third circles do, how to
 *  resize them, how to exit the command."
 *
 *  ⚠️ WHAT THIS CAN AND CANNOT PROVE (`SC-§32`): it CAN prove that from the page for every
 *  circle-drawing order, the three pick-mode controls are reachable, keyed and non-empty, and
 *  that their text is the SAME TEXT those rows carry (⛔ not a second copy that can drift). It
 *  ⛔ CANNOT prove the prose reads well or answers the question to his satisfaction — that is his
 *  part of `HELP-§6` and no agent may claim it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpNamedQuestionsTest,
	"Siegebound.ControlsHelp.DetailContentAnswersTheNamedQuestions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpNamedQuestionsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());
	if (!Qwerty.IsValid())
	{
		AddError(TEXT("Could not construct a scratch layout subsystem - the named-questions claim is untested."));
		return false;
	}

	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };

	// The three orders that draw circles, and the three controls his sentence names.
	const TCHAR* CircleOrderIds[] = { TEXT("Orders.Hold"), TEXT("Orders.Ambush"), TEXT("Orders.Follow") };
	const TCHAR* NamedControlIds[] = { TEXT("PickMode.Confirm"), TEXT("PickMode.Resize"), TEXT("PickMode.Cancel") };

	for (const TCHAR* OrderId : CircleOrderIds)
	{
		const FSiegeControlsHelpAction* const OrderRow = FSiegeControlsHelpRegistry::FindAction(FName(OrderId));
		if (OrderRow == nullptr)
		{
			AddError(FString::Printf(TEXT("'%s' is missing from the registry."), OrderId));
			continue;
		}

		const FSiegeControlsDetailContent Page =
			FSiegeControlsHelpRegistry::ComposeDetailContent(*OrderRow, Qwerty.Layout.Get(), NoAppliedKeys);

		TestFalse(*FString::Printf(TEXT("'%s' page has a title"), OrderId), Page.Title.IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("'%s' page has a key chip"), OrderId), Page.KeyChip.IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("'%s' page has body prose"), OrderId), Page.Body.IsEmptyOrWhitespace());

		for (const TCHAR* NamedId : NamedControlIds)
		{
			const FSiegeControlsDetailEntry* const Entry = Page.Related.FindByPredicate(
				[NamedId](const FSiegeControlsDetailEntry& Candidate) { return Candidate.ActionId == FName(NamedId); });

			// ⭐ THE ASSERTION THAT IS HIS SENTENCE.
			if (!TestNotNull(*FString::Printf(TEXT("⭐ The '%s' page carries '%s' - Jonathan's \"all the controls with it\""),
				OrderId, NamedId), Entry))
			{
				continue;
			}

			TestFalse(*FString::Printf(TEXT("'%s' -> '%s' has a derived key chip"), OrderId, NamedId),
				Entry->KeyChip.IsEmptyOrWhitespace());
			TestFalse(*FString::Printf(TEXT("'%s' -> '%s' has a name"), OrderId, NamedId),
				Entry->DisplayName.IsEmptyOrWhitespace());
			TestFalse(*FString::Printf(TEXT("'%s' -> '%s' has body prose"), OrderId, NamedId),
				Entry->Body.IsEmptyOrWhitespace());

			// ⭐⭐ ONE DEFINITION, TWO RENDERINGS (`HELP-§2`). The block is the SAME TEXT that row's
			// own page shows — so the three-circle explanation cannot drift into two versions, and
			// fixing it in one place fixes it everywhere it appears.
			const FSiegeControlsHelpAction* const NamedRow = FSiegeControlsHelpRegistry::FindAction(FName(NamedId));
			if (NamedRow != nullptr)
			{
				TestEqual(*FString::Printf(TEXT("⭐ '%s' -> '%s' renders that row's OWN text, not a second copy of it"),
					OrderId, NamedId),
					Entry->Body.ToString(),
					FSiegeControlsHelpRegistry::ComposeDetailContent(*NamedRow, Qwerty.Layout.Get(), NoAppliedKeys).Body.ToString());
			}
		}

		// ⛔ ONE LEVEL DEEP. PickMode.Confirm carries related ids of its own, so if the composer
		// ever expanded transitively this count would exceed the declared list — and the Ambush
		// page would repeat the same three explanations several times over.
		TestEqual(*FString::Printf(TEXT("⛔ '%s' renders exactly its declared related controls - the composer does not recurse"), OrderId),
			Page.Related.Num(), OrderRow->RelatedActionIds.Num());
	}

	// ⭐ The pre-condition that makes the no-recursion claim above meaningful: the data really
	// does contain a cycle for the composer to have to refuse.
	const FSiegeControlsHelpAction* const ConfirmRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("PickMode.Confirm")));
	if (ConfirmRow != nullptr)
	{
		TestTrue(TEXT("FIXTURE SELF-CHECK: PickMode.Confirm itself declares related controls, so the cycle is real"),
			ConfirmRow->RelatedActionIds.Num() > 0);
	}

	// ── UNKNOWN AND SELF-REFERENTIAL IDS ARE DROPPED, ⛔ NEVER RENDERED BLANK ────────────
	FSiegeControlsHelpAction Synthetic = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);
	Synthetic.Detail = FText::FromString(TEXT("Synthetic detail prose."));
	Synthetic.RelatedActionIds = {
		FName(TEXT("Nope.NotAnAction")),        // unknown
		Synthetic.ActionId,                      // itself
		NAME_None,                               // empty
		FName(TEXT("PickMode.Resize"))           // the one real entry
	};

	const FSiegeControlsDetailContent SyntheticPage =
		FSiegeControlsHelpRegistry::ComposeDetailContent(Synthetic, Qwerty.Layout.Get(), NoAppliedKeys);

	TestEqual(TEXT("⛔ Unknown, self-referential and empty related ids are all dropped - only the real one renders"),
		SyntheticPage.Related.Num(), 1);
	if (SyntheticPage.Related.Num() == 1)
	{
		TestEqual(TEXT("...and it is the real one"),
			SyntheticPage.Related[0].ActionId, FName(TEXT("PickMode.Resize")));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 12 — Siegebound.ControlsHelp.DetailBackSeamReturnsToTheList
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE WAY OUT OF A FULL-SCREEN PAGE, ASSERTED AS A BEHAVIOUR — and it matters more here than
 *  the row seam did, because ⛔ `Escape` is permanently unavailable as a second way out
 *  (`AS-§6 A-2`). If Back does not fire, the page is only leavable by closing the whole overlay.
 *
 *  ⭐⭐ THE ASYMMETRY THIS TEST PINS: an UNSTAMPED ROW deliberately reports nothing (a
 *  construction bug must not look like a player action), but an UNSTAMPED PAGE must still let
 *  the player leave. The two rules point opposite ways on purpose and both are asserted.
 *
 *  ⚠️ No Slate tree is built and none is needed: RequestBack() is the same entry point the Back
 *  button's OnClicked thunk calls.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpDetailBackSeamTest,
	"Siegebound.ControlsHelp.DetailBackSeamReturnsToTheList",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpDetailBackSeamTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	TStrongObjectPtr<USiegeControlsDetailWidget> Page(
		NewObject<USiegeControlsDetailWidget>(GetTransientPackageAsObject()));

	if (!Page.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeControlsDetailWidget - the back seam is untested."));
		return false;
	}

	int32 BackCount = 0;
	Page->OnBackRequested.BindLambda([&BackCount]() { ++BackCount; });

	// ── ⭐⭐ AN UNSTAMPED PAGE STILL LETS THE PLAYER OUT ─────────────────────────────────
	TestEqual(TEXT("An unstamped page reports NAME_None from GetDetailActionId"),
		Page->GetDetailActionId(), FName(NAME_None));
	Page->RequestBack();
	TestEqual(TEXT("⭐⭐ An UNSTAMPED page still fires Back - a page with no way out is the one defect a help screen must not have"),
		BackCount, 1);

	// ── STAMPED: the page knows which row it shows ──────────────────────────────────────
	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());
	if (!Qwerty.IsValid())
	{
		AddError(TEXT("Could not construct a scratch layout subsystem - the stamp half is untested."));
		return false;
	}

	const FSiegeControlsHelpAction* const AmbushRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Orders.Ambush")));
	if (AmbushRow == nullptr)
	{
		AddError(TEXT("Orders.Ambush is missing from the registry - the stamp half is untested."));
		return false;
	}

	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };
	const FSiegeControlsDetailContent Content =
		FSiegeControlsHelpRegistry::ComposeDetailContent(*AmbushRow, Qwerty.Layout.Get(), NoAppliedKeys);

	// ⛔ The stamp must survive a tree that does not exist yet (the BLOCKER 674-1 ordering hole):
	// this widget has no Slate tree at all here, which is the harshest version of that timing.
	Page->SetDetailContent(Content);
	TestEqual(TEXT("The stamp lands on GetDetailActionId even with no widget tree built"),
		Page->GetDetailActionId(), AmbushRow->ActionId);

	Page->RequestBack();
	TestEqual(TEXT("A stamped page fires Back too"), BackCount, 2);

	// ── UNBINDING IS CLEAN (the overlay unbinds in NativeDestruct) ──────────────────────
	Page->OnBackRequested.Unbind();
	Page->RequestBack();
	TestEqual(TEXT("An unbound page fires nothing"), BackCount, 2);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 13 — Siegebound.ControlsHelp.DetailComposerIsFailSafe
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ THE WORST OUTCOME THIS FEATURE MAY PRODUCE IS "THE PAGE IS LESS USEFUL", ⛔ NEVER "THE PAGE
 *  IS BLANK" AND ⛔ NEVER A CRASH (`KBD-§5`'s fail-safe law; `HELP-§2` mechanism 2).
 *
 *  Every degraded input the detail composer can actually be handed in a shipped build is driven
 *  here: no layout subsystem at all, an action nothing maps, a pointer-only row with no key in
 *  existence, and a row whose prose was never written.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpDetailFailSafeTest,
	"Siegebound.ControlsHelp.DetailComposerIsFailSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpDetailFailSafeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };

	// ── (a) ⛔ NO LAYOUT SUBSYSTEM AT ALL — every page still composes ────────────────────
	// This is the `KBD-§5` state: the accessor is unreachable, so labels degrade to the reference
	// keys unchanged and the overlay behaves exactly as it would on a positionally-QWERTY host.
	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		const FString RowName = Row.ActionId.ToString();
		const FSiegeControlsDetailContent Page =
			FSiegeControlsHelpRegistry::ComposeDetailContent(Row, nullptr, NoAppliedKeys);

		TestEqual(*FString::Printf(TEXT("Row '%s' page carries its own id with a null subsystem"), *RowName),
			Page.ActionId, Row.ActionId);
		TestFalse(*FString::Printf(TEXT("Row '%s' page has a title with a null subsystem"), *RowName),
			Page.Title.IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("Row '%s' page has a chip with a null subsystem"), *RowName),
			Page.KeyChip.IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("Row '%s' page has a summary with a null subsystem"), *RowName),
			Page.Summary.IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("Row '%s' page has body prose with a null subsystem"), *RowName),
			Page.Body.IsEmptyOrWhitespace());
	}

	// ── (b) THE APPLIED KEY OUTRANKS THE REGISTRY ON THE PAGE, JUST AS IT DOES ON THE ROW ─
	// ⚠️ This is the one-translation law reaching the detail lane: a rebound action's page must
	// name the key the player actually presses, ⛔ not the QWERTY reference the registry holds.
	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());
	if (!Qwerty.IsValid())
	{
		AddError(TEXT("Could not construct a scratch layout subsystem - the applied-key claim is untested."));
		return false;
	}

	const FSiegeControlsHelpAction* const AmbushRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Orders.Ambush")));
	if (AmbushRow != nullptr)
	{
		// A key nothing in the registry names for this row — so a chip carrying it can only have
		// come from the applied-key lane.
		auto ReboundProvider = [](const FSiegeControlsHelpAction& QueryRow) -> TArray<FKey>
		{
			return QueryRow.Lane == ESiegeInputLane::MappedAction
				? TArray<FKey>{ EKeys::NumPadSeven }
				: TArray<FKey>();
		};

		const FSiegeControlsDetailContent Rebound =
			FSiegeControlsHelpRegistry::ComposeDetailContent(*AmbushRow, Qwerty.Layout.Get(), ReboundProvider);

		TestEqual(TEXT("⭐ A rebound action's page chip is the APPLIED key, not the registry's reference key"),
			Rebound.KeyChip.ToString(), EKeys::NumPadSeven.GetDisplayName(false).ToString());
	}

	// ── (c) A POINTER-ONLY ROW HAS NO KEY AND STILL HAS A CHIP ──────────────────────────
	const FSiegeControlsHelpAction* const RevealRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.WarMapReveal")));
	if (RevealRow != nullptr)
	{
		const FSiegeControlsDetailContent RevealPage =
			FSiegeControlsHelpRegistry::ComposeDetailContent(*RevealRow, Qwerty.Layout.Get(), NoAppliedKeys);
		TestFalse(TEXT("A pointer-only row's page still renders a non-empty chip"),
			RevealPage.KeyChip.IsEmptyOrWhitespace());
	}

	// ── (d) ⛔ AN UNDOCUMENTED ROW YIELDS THE PINNED TODO STRING, ⛔ NEVER A BLANK PAGE ──
	// The board's item 6, driven deliberately: this is the state every row was in before TASK-707
	// filled the prose, and it must stay survivable for a command someone adds later.
	FSiegeControlsHelpAction Undocumented = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);
	Undocumented.Detail = FText::GetEmpty();

	const FSiegeControlsDetailContent UndocumentedPage =
		FSiegeControlsHelpRegistry::ComposeDetailContent(Undocumented, Qwerty.Layout.Get(), NoAppliedKeys);

	TestEqual(TEXT("⛔ An undocumented row's PAGE shows the pinned TODO string, never a blank screen"),
		UndocumentedPage.Body.ToString(), FString(FSiegeControlsHelpRegistry::GetUndocumentedText()));

	// Whitespace-only is the same failure wearing a disguise.
	Undocumented.Detail = FText::FromString(TEXT("   \n  "));
	TestEqual(TEXT("...and whitespace-only detail is treated identically"),
		FSiegeControlsHelpRegistry::ComposeDetailContent(Undocumented, Qwerty.Layout.Get(), NoAppliedKeys).Body.ToString(),
		FString(FSiegeControlsHelpRegistry::GetUndocumentedText()));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 14 — Siegebound.ControlsHelp.DiscardAllLetterMovesWhileCardDigitsHold
//            ⭐⭐ TASK-821, and `HELP-§6`'s ACCEPTANCE SENTENCE WITH BOTH OPERANDS SHIPPED
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ `HELP-§6`, VERBATIM: "flip the simulated layout and assert the label CHANGES for a key
 *  that moves and HOLDS for one that does not." Test 3 already does that per-key on SYNTHETIC
 *  rows. ⭐ THIS IS THE FIRST TIME THE PAIR CAN BE MADE ON TWO SHIPPED ROWS AT ONCE, because
 *  the card bar only just acquired a LETTER (`Cards.Discard` -> IA_DiscardAll) to sit beside
 *  its DIGITS (`Cards.Play` -> IA_Card1..6) — and both are on the SAME lane, resolved by the
 *  SAME function, in the SAME call shape.
 *
 *  ⛔⛔ WHY THE "ONE CODE PATH" PART IS THE REAL CLAIM: the two rows must produce OPPOSITE
 *  outcomes with ⛔ no conditional layout logic anywhere. The letter moves because the shipped
 *  positional table has an entry for it; the digits hold because that table carries the 26
 *  letters and ⛔ nothing else. ⇒ an `if (bIsDigit)` or any per-key special case would be a
 *  second copy of a fact the table already owns, and this test asserts the property (both rows
 *  through one resolver) rather than the outcome alone.
 *
 *  ⛔ `SC-§37` GOVERNS EVERY ASSERTION BELOW. A test asserting the chip `== "H"` would be a
 *  transcription of the registry: it would pass on an implementation that hardcoded `H`, which
 *  is the exact defect this feature exists to prevent, and it could never fail on the machine
 *  that matters. ⇒ every claim here is made against WHAT THE ACCESSOR ANSWERS, against the
 *  SHIPPED TABLE, or against a DIFFERENCE between two layouts.
 *
 *  ⚠️ WHAT IT CANNOT PROVE (`SC-§32`): nothing here presses a key or opens PIE. That `H`
 *  actually reaches ASiegePlayerController::OnDiscardAllPressed is TASK-811's pixel row; that
 *  the page READS well is Jonathan's part of `HELP-§6` and no agent may claim it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpDiscardAllLayoutTest,
	"Siegebound.ControlsHelp.DiscardAllLetterMovesWhileCardDigitsHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpDiscardAllLayoutTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());   // an EMPTY map IS the QWERTY state
	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());

	if (!Qwerty.IsValid() || !Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the two scratch layout subsystems - the changes/holds pair is untested."));
		return false;
	}

	const FSiegeControlsHelpAction* const DiscardRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Cards.Discard")));
	const FSiegeControlsHelpAction* const PlayRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Cards.Play")));

	if (DiscardRow == nullptr || PlayRow == nullptr)
	{
		// ⛔ A null Cards.Discard means the row was DELETED rather than rewritten in place - the
		// failure test 1's RequiredIds[] also catches, said again here in the task's own words.
		AddError(TEXT("Cards.Discard and/or Cards.Play is missing from the registry - the row must be REWRITTEN in place, never removed."));
		return false;
	}

	// ── (a) THE LANE FLIP, ASSERTED AS THE PRECONDITION IT IS ───────────────────────────
	// ⛔ Not decoration. On the PointerOnly lane this row shipped as, ComposeKeyChipLabel
	// answers the pointer affordance REGARDLESS of the keys it is handed
	// (SiegeControlsHelpWidget.cpp:1205-1208) — so its label could not move with the layout at
	// all, and (d) below would be unreachable rather than merely red.
	TestEqual(TEXT("Cards.Discard is on the MAPPED lane - a pointer row's chip can never follow the layout"),
		static_cast<int32>(DiscardRow->Lane), static_cast<int32>(ESiegeInputLane::MappedAction));
	TestFalse(TEXT("...and it is no longer flagged pointer-only"), DiscardRow->bPointerOnly);
	TestTrue(TEXT("...and it names an IA_* asset, so its chip is READ BACK from the applied context rather than translated here"),
		DiscardRow->Actions.Num() > 0);
	TestEqual(TEXT("⭐ Cards.Play is on the SAME lane - the two OPPOSITE outcomes below come out of ONE code path"),
		static_cast<int32>(PlayRow->Lane), static_cast<int32>(ESiegeInputLane::MappedAction));

	// ── (b) ⭐⭐ THE PROPERTY BEHIND "THE DIGITS HOLD", READ FROM THE SHIPPED TABLE ───────
	// ⛔ `SC-§37`: "the digits held" measured against THIS FILE'S fixture would be a
	// transcription of my own omission — the fixture is a TMap written by hand, and it has no
	// digit in it because nobody put one there. The load-bearing fact is structural and lives
	// one layer down: the positional table that every real translation map is BUILT from
	// carries A..Z and nothing else. ⇒ ask the table, not the fixture.
	const TArray<FSiegePositionalKeyProbe> Probes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();

	auto TableCarries = [&Probes](const FKey& Key) -> bool
	{
		return Probes.ContainsByPredicate(
			[&Key](const FSiegePositionalKeyProbe& Probe) { return Probe.QwertyKey == Key; });
	};

	TestTrue(TEXT("FIXTURE SELF-CHECK: the shipped positional table is non-empty, so asking it means something"),
		Probes.Num() > 0);

	for (const FKey& ReferenceKey : DiscardRow->QwertyReferenceKeys)
	{
		TestTrue(*FString::Printf(
			TEXT("⭐ The discard-all row's reference key %s IS in the positional table - it is a letter, so it CAN move"),
			*Describe(ReferenceKey)), TableCarries(ReferenceKey));
	}

	for (const FKey& ReferenceKey : PlayRow->QwertyReferenceKeys)
	{
		TestFalse(*FString::Printf(
			TEXT("⛔ The card row's reference key %s is NOT in the positional table - a digit is immune BY CONSTRUCTION, never by a special case"),
			*Describe(ReferenceKey)), TableCarries(ReferenceKey));
	}

	// ── (c) FIXTURE SELF-CHECK ON THE INJECTED MAP ──────────────────────────────────────
	// ⚠️ A claim about the FIXTURE, ⛔ not about a label. Without a map that genuinely moves
	// this row's key, "the chip changed" would be vacuous; without a DISTINCT onward hop, a
	// DOUBLE translation would be undetectable. (Test 2's idiom, reused rather than reinvented.)
	if (DiscardRow->QwertyReferenceKeys.Num() != 1)
	{
		AddError(FString::Printf(TEXT("Cards.Discard carries %d reference keys; this test is written for exactly one."),
			DiscardRow->QwertyReferenceKeys.Num()));
		return false;
	}

	const FKey DiscardReference = DiscardRow->QwertyReferenceKeys[0];
	const FKey OneHop  = Dvorak.Layout->GetPositionalKey(DiscardReference);
	const FKey TwoHops = Dvorak.Layout->GetPositionalKey(OneHop);

	TestTrue(*FString::Printf(TEXT("FIXTURE: the injected map moves the discard-all key once (%s -> %s)"),
		*Describe(DiscardReference), *Describe(OneHop)), OneHop != DiscardReference);
	TestTrue(*FString::Printf(TEXT("FIXTURE: it moves AGAIN from there (%s -> %s), so a double translation is DETECTABLE"),
		*Describe(OneHop), *Describe(TwoHops)), TwoHops != OneHop);

	// ── (d) ⭐⭐ THE CHANGES / HOLDS PAIR, IN ONE TEST, THROUGH ONE RESOLVER ─────────────
	auto ChipFor = [](const FSiegeControlsHelpAction& Row, const USiegeKeyboardLayoutSubsystem* Layout) -> FString
	{
		// The FALLBACK lane deliberately (an EMPTY applied-key array): with no world there is no
		// active context, which is also the real state on a machine where IA_DiscardAll has not
		// landed. ⛔ ONE lambda serves BOTH rows — there is no branch here to get wrong.
		return FSiegeControlsHelpRegistry::ComposeKeyChipLabel(
			Row, FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, TArray<FKey>(), Layout)).ToString();
	};

	const FString DiscardOnQwerty = ChipFor(*DiscardRow, Qwerty.Layout.Get());
	const FString DiscardOnDvorak = ChipFor(*DiscardRow, Dvorak.Layout.Get());
	const FString PlayOnQwerty    = ChipFor(*PlayRow,    Qwerty.Layout.Get());
	const FString PlayOnDvorak    = ChipFor(*PlayRow,    Dvorak.Layout.Get());

	TestFalse(TEXT("The discard-all chip is non-empty on QWERTY"), DiscardOnQwerty.IsEmpty());
	TestFalse(TEXT("The discard-all chip is non-empty on Dvorak"), DiscardOnDvorak.IsEmpty());
	TestFalse(TEXT("The card-play chip is non-empty on QWERTY"), PlayOnQwerty.IsEmpty());

	// ⭐ HALF ONE — THE LETTER MOVES.
	TestNotEqual(*FString::Printf(TEXT("⭐ THE LETTER MOVES: Cards.Discard's chip CHANGES across the layout flip (%s -> %s)"),
		*DiscardOnQwerty, *DiscardOnDvorak), DiscardOnQwerty, DiscardOnDvorak);

	// ⛔ And it moved to the ACCESSOR'S OWN ANSWER, ⛔ not to a letter anyone typed.
	TestEqual(TEXT("...and the Dvorak chip IS GetPositionalKey's one-hop answer, asked of the accessor itself"),
		DiscardOnDvorak, OneHop.GetDisplayName(/*bLongDisplayName=*/false).ToString());
	TestNotEqual(TEXT("⛔ ...and it is NOT the two-hop answer a double translation would produce"),
		DiscardOnDvorak, TwoHops.GetDisplayName(/*bLongDisplayName=*/false).ToString());
	TestEqual(TEXT("...while the QWERTY chip is the accessor's answer on an empty map - the reference key, unchanged"),
		DiscardOnQwerty, Qwerty.Layout->GetPositionalKey(DiscardReference).GetDisplayName(/*bLongDisplayName=*/false).ToString());

	// ⭐ HALF TWO — THE DIGITS HOLD. ⛔ Same lane, same resolver, same lambda, opposite outcome.
	TestEqual(*FString::Printf(TEXT("⭐ THE DIGITS HOLD: Cards.Play's chip is IDENTICAL on both layouts (%s)"), *PlayOnQwerty),
		PlayOnQwerty, PlayOnDvorak);

	// ⭐⭐ AND THE PAIR IS ASSERTED AS A PAIR: both halves must hold in the SAME run, or one of
	// them is being satisfied by an implementation that translates nothing (or everything).
	TestTrue(TEXT("⭐⭐ ONE FLIP, TWO OPPOSITE OUTCOMES, ZERO CONDITIONAL LAYOUT LOGIC"),
		DiscardOnQwerty != DiscardOnDvorak && PlayOnQwerty == PlayOnDvorak);

	// ⛔ THE DETAIL PAGE MOVES WITH THE CHIP, because its prose names the key as a {Cards.Discard}
	// token rather than as a typed letter. A page that held still here would mean a letter had
	// been typed into the prose one line below the chip that exists to avoid exactly that.
	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };
	const FString PageOnQwerty =
		FSiegeControlsHelpRegistry::ComposeDetailContent(*DiscardRow, Qwerty.Layout.Get(), NoAppliedKeys).Body.ToString();
	const FString PageOnDvorak =
		FSiegeControlsHelpRegistry::ComposeDetailContent(*DiscardRow, Dvorak.Layout.Get(), NoAppliedKeys).Body.ToString();

	TestNotEqual(TEXT("⭐ The discard-all DETAIL PAGE changes with the layout too - the key inside its prose is derived, not typed"),
		PageOnQwerty, PageOnDvorak);

	// ⛔ AND THE DIFFERENCE IS THE TOKEN MECHANISM, NOT AN ACCIDENT — asserted at the source
	// rather than by hunting the composed letter in the output. ⚠️ A `Body.Contains("D")` here
	// would be VACUOUS: the prose also names DiscardAllCost, so that letter is present whatever
	// the implementation does. The claim that can actually fail is that the RAW prose names the
	// key as a {ActionId} token — i.e. that no letter was typed into the sentence at all.
	TestTrue(TEXT("⛔ The page names its own key as a {ActionId} token, never as a typed letter (HELP-§1 in the detail lane)"),
		DiscardRow->Detail.ToString().Contains(
			FSiegeControlsHelpRegistry::MakeActionToken(DiscardRow->ActionId), ESearchCase::CaseSensitive));

	// ── (e) ⛔ THE FEE IS NAMED, ⛔ NEVER TYPED (`HELP-§2`'s M7.7 rule) ──────────────────
	// ⚠️ SCOPED TO THIS ROW ON PURPOSE: other rows legitimately carry digits in prose (Cards.Play
	// names key 1's legacy quirk, and a digit provably cannot move). The claim here is narrower
	// and exact — the discard-all fee has ONE definition, DiscardAllCost, and this page must not
	// hold a second copy of its VALUE. A typed number rots the moment that property is retuned,
	// and neither the compiler nor a reviewer would notice.
	const FString DiscardOneLine = FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(*DiscardRow).ToString();
	const FString DiscardDetail  = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*DiscardRow).ToString();

	TestTrue(TEXT("⭐ The page NAMES its fee property instead of restating its value"),
		DiscardDetail.Contains(TEXT("DiscardAllCost"), ESearchCase::CaseSensitive));

	// ⛔ THE WRONG FEE, ASSERTED AGAINST DIRECTLY. "DiscardAllCost" does NOT contain the
	// substring "DiscardCost" (the `C` never follows the `d`), so this is a real, independent
	// claim: the per-card fee — retired with DiscardHandSlot — must not be named here.
	const TCHAR* RetiredVocabulary[] = { TEXT("DiscardCost"), TEXT("DiscardHandSlot"), TEXT("RequestDiscardSlot") };
	for (const TCHAR* Retired : RetiredVocabulary)
	{
		TestFalse(*FString::Printf(TEXT("⛔ The page does not name the retired per-card route '%s'"), Retired),
			DiscardDetail.Contains(Retired, ESearchCase::CaseSensitive));
	}

	auto CarriesADigit = [](const FString& Prose) -> bool
	{
		for (const TCHAR Character : Prose)
		{
			if (FChar::IsDigit(Character))
			{
				return true;
			}
		}
		return false;
	};

	// FIXTURE SELF-CHECK first: a scanner that can never answer true would make both claims
	// below vacuous. (A claim about the SCANNER, not about the prose.)
	TestTrue(TEXT("FIXTURE SELF-CHECK: the digit scanner does find a digit when one is present"),
		CarriesADigit(FString(TEXT("a fee of 20 gold"))));

	TestFalse(TEXT("⛔ The discard-all one-liner types NO number"), CarriesADigit(DiscardOneLine));
	TestFalse(TEXT("⛔ ...and neither does its detail page - the fee is read from DiscardAllCost, never typed"),
		CarriesADigit(DiscardDetail));

	// ── (f) ⛔ THE PAGE TEACHES THE KEY AND NOTHING ELSE ────────────────────────────────
	// Jonathan cut the right-click route on 2026-09-03, BEFORE it was written. A surviving
	// right-click sentence would teach a control that does not exist — which `HELP-§2` calls
	// worse than no help screen, and which is the one defect this whole row exists to remove.
	const TCHAR* ScrappedRouteFragments[] = { TEXT("right-click"), TEXT("right click"), TEXT("discard button") };
	for (const TCHAR* Fragment : ScrappedRouteFragments)
	{
		TestFalse(*FString::Printf(TEXT("⛔ The discard-all one-liner does not mention '%s'"), Fragment),
			DiscardOneLine.Contains(Fragment, ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("⛔ The discard-all detail page does not mention '%s'"), Fragment),
			DiscardDetail.Contains(Fragment, ESearchCase::IgnoreCase));
	}

	// ⭐ THE ALT-CURSOR CAVEAT WENT WITH IT, ASSERTED AS DATA RATHER THAN AS A SUBSTRING GAMBLE:
	// Cards.CursorHold was this row's related control ONLY because the discard used to be a HUD
	// button you had to raise the cursor to click. Nothing in this gesture needs a cursor now.
	TestFalse(TEXT("⛔ The Alt-cursor row is no longer a related control here - nothing in this gesture needs a cursor"),
		DiscardRow->RelatedActionIds.Contains(FName(TEXT("Cards.CursorHold"))));

	AddInfo(FString::Printf(
		TEXT("Layout flip: discard-all chip %s -> %s (MOVED); card chips %s (HELD). Both rows: lane MappedAction, one resolver, no per-key branch."),
		*DiscardOnQwerty, *DiscardOnDvorak, *PlayOnQwerty));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 15 — Siegebound.ControlsHelp.TowerAndMapMarkRowsAreAuthoredAndRawLaned
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ TASK-823 (the TOWER half of `CARDBAR-§9`): the three rows this task appended —
 *  Cards.StackUpgrade, Cards.PlacementResize and Interface.MapMarks — exist, are AUTHORED, and
 *  sit on the lane their gestures actually belong to.
 *
 *  ⛔ WHY A NEW TEST RATHER THAN THREE ENTRIES IN TEST 1's `RequiredIds[]`: that array is a
 *  SHIPPED assertion two other tasks are currently serialised against, and `CARDBAR-§9` makes
 *  editing it to accommodate a row change an automatic fail. Appending a test costs it nothing
 *  and keeps the failure legible — a red here names THIS task's rows and no one else's.
 *
 *  ⛔ `SC-§37` GOVERNS EVERY ASSERTION BELOW. There is no `TestEqual(Chip, "LMB")` anywhere: a
 *  transcription of the registry passes on an implementation that hardcoded the letter, which is
 *  the defect the whole feature exists to prevent. The claims here are made against WHAT THE
 *  ACCESSOR ANSWERS and against a DIFFERENCE between two simulated layouts.
 *
 *  ⭐ THE `GetPositionalKey`-FREE CLAIM IS ASSERTED AS A BEHAVIOUR, ⛔ NOT AS A GREP: these rows
 *  must label their keys VERBATIM on every layout. If someone later routes them through the
 *  translator, or flips one to the mapped lane, block (d) goes red — a grep of the diff cannot
 *  say that, and a grep is what a reviewer would otherwise be relying on.
 *
 *  ⚠️ WHAT IT CANNOT PROVE (`SC-§32`): nothing here scrolls a wheel, opens PIE or paints a row.
 *  That the pages READ well is Jonathan's part of `HELP-§6` and no agent may claim it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpTowerRowsTest,
	"Siegebound.ControlsHelp.TowerAndMapMarkRowsAreAuthoredAndRawLaned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpTowerRowsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());
	if (!Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the scratch layout subsystem - the raw-lane claim is untested."));
		return false;
	}

	// FIXTURE SELF-CHECK FIRST: without a map that genuinely moves SOMETHING, "these keys did not
	// move" is vacuous — it would hold on a layout system that translated nothing at all.
	// (A claim about the FIXTURE, ⛔ not about a row.)
	TestTrue(TEXT("FIXTURE SELF-CHECK: the injected map really does move a letter, so 'unchanged' means something"),
		Dvorak.Layout->GetPositionalKey(EKeys::F) != EKeys::F);

	const FString UndocumentedString(FSiegeControlsHelpRegistry::GetUndocumentedText());

	// ⭐ The three rows and the keys each one's gesture is REALLY made of, read off the shipped
	// paths: the upgrade is the placement CONFIRM (a polled left button), the placement wheel is
	// the polled wheel, and the map marks are all four mouse gestures on the war map widget.
	struct FExpectedRow
	{
		const TCHAR*  ActionId;
		const TCHAR*  Category;
		TArray<FKey>  Keys;
	};

	const TArray<FExpectedRow> NewRows =
	{
		{ TEXT("Cards.StackUpgrade"),     TEXT("Cards"),     { EKeys::LeftMouseButton } },
		{ TEXT("Cards.PlacementResize"),  TEXT("Cards"),     { EKeys::MouseScrollUp, EKeys::MouseScrollDown } },
		{ TEXT("Interface.MapMarks"),     TEXT("Interface"), { EKeys::LeftMouseButton, EKeys::RightMouseButton,
		                                                       EKeys::MouseScrollUp, EKeys::MouseScrollDown } }
	};

	// ⛔ NO TUNABLE'S VALUE MAY BE TYPED INTO THESE PAGES (`HELP-§2`, the M7.7 "in 400" lesson).
	// ⚠️ SCOPED TO THIS TASK'S THREE ROWS ON PURPOSE: other rows legitimately carry digits — the
	// war map's 30 gold is quoted from Jonathan's own sentence at its property. The claim here is
	// exact: the height cap, the health step, the wheel step, the wheel's two ends and the mark
	// cap have ONE definition each, and these pages must not hold a second copy of any VALUE. A
	// typed number rots the moment one is retuned, and neither the compiler nor a reviewer notices.
	auto CarriesADigit = [](const FString& Prose) -> bool
	{
		for (const TCHAR Character : Prose)
		{
			if (FChar::IsDigit(Character))
			{
				return true;
			}
		}
		return false;
	};

	// FIXTURE SELF-CHECK: a scanner that can never answer true would make every claim below it
	// vacuous. (A claim about the SCANNER, ⛔ not about the prose.)
	TestTrue(TEXT("FIXTURE SELF-CHECK: the digit scanner finds a digit when one is present"),
		CarriesADigit(FString(TEXT("up to 5 times taller"))));

	for (const FExpectedRow& Expected : NewRows)
	{
		const FSiegeControlsHelpAction* const Row = FSiegeControlsHelpRegistry::FindAction(FName(Expected.ActionId));

		// ── (a) IT EXISTS ───────────────────────────────────────────────────────────────
		if (Row == nullptr)
		{
			AddError(FString::Printf(
				TEXT("Row '%s' is missing from the registry - the tower half of the controls menu is not shipped."),
				Expected.ActionId));
			continue;
		}

		TestEqual(*FString::Printf(TEXT("Row '%s' is filed under the category its gesture belongs to"), Expected.ActionId),
			Row->Category.ToString(), FString(Expected.Category));

		// ── (b) IT IS AUTHORED, ⛔ NOT THE TODO FALLBACK (`HELP-§2` mechanism 2) ─────────
		// ⚠️ Both halves, and BOTH are needed: an empty one-liner and an empty detail render the
		// same pinned string, so "the row exists" says nothing about whether anyone wrote it.
		const FString OneLine = FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(*Row).ToString();
		const FString Detail  = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*Row).ToString();

		TestFalse(*FString::Printf(TEXT("Row '%s' renders a non-empty one-liner"), Expected.ActionId), OneLine.IsEmpty());
		TestFalse(*FString::Printf(TEXT("Row '%s' renders non-empty detail"), Expected.ActionId), Detail.IsEmpty());
		TestNotEqual(*FString::Printf(TEXT("⛔ Row '%s' one-liner does NOT render the pinned '(undocumented - TODO)' string"), Expected.ActionId),
			OneLine, UndocumentedString);
		TestNotEqual(*FString::Printf(TEXT("⛔ Row '%s' detail page does NOT render the pinned '(undocumented - TODO)' string"), Expected.ActionId),
			Detail, UndocumentedString);

		// ⭐ EACH ROW SAYS **WHEN** ITS GESTURE APPLIES, and that is the whole reason these three
		// rows are separable at all (`STACK-§4` / `HELP-§2`: a screen that conflates the modes is
		// worse than no screen). Asserted as a PROPERTY of the pair rather than as a substring
		// hunt: the detail page must add real material to the one-liner, not repeat it.
		TestNotEqual(*FString::Printf(TEXT("Row '%s' detail is not just its one-liner again"), Expected.ActionId),
			Detail, OneLine);
		TestTrue(*FString::Printf(TEXT("Row '%s' detail is substantially longer than its one-liner (%d vs %d chars)"),
			Expected.ActionId, Detail.Len(), OneLine.Len()), Detail.Len() > OneLine.Len());

		// ── (c) THE LANE AND ITS KEYS ───────────────────────────────────────────────────
		// ⛔ RawNonLetter, ⛔ never PointerOnly: on the pointer lane ComposeKeyChipLabel answers
		// the single "Mouse click" affordance REGARDLESS of the keys it is handed, so the WHEEL
		// could not appear on the row at all — and making the wheel visible in the LIST is the
		// entire reason two of these rows were boarded.
		TestEqual(*FString::Printf(TEXT("Row '%s' is on the RAW non-letter lane (its keys are labelled verbatim)"), Expected.ActionId),
			static_cast<int32>(Row->Lane), static_cast<int32>(ESiegeInputLane::RawNonLetter));
		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' is NOT pointer-only - a pointer chip cannot show a wheel"), Expected.ActionId),
			Row->bPointerOnly);
		TestEqual(*FString::Printf(TEXT("⛔ Row '%s' names NO input action - naming one would put a raw key on the Enhanced Input lane"), Expected.ActionId),
			Row->Actions.Num(), 0);

		TestEqual(*FString::Printf(TEXT("Row '%s' carries exactly the keys its shipped gesture is made of (got %s)"),
			Expected.ActionId, *DescribeKeys(Row->QwertyReferenceKeys)),
			Row->QwertyReferenceKeys.Num(), Expected.Keys.Num());

		for (const FKey& RequiredKey : Expected.Keys)
		{
			TestTrue(*FString::Printf(TEXT("Row '%s' carries %s"), Expected.ActionId, *Describe(RequiredKey)),
				Row->QwertyReferenceKeys.Contains(RequiredKey));
		}

		// ── (d) ⭐⭐ ZERO TRANSLATION, ASSERTED AS A BEHAVIOUR ───────────────────────────
		// The chip must be the reference keys themselves on a Dvorak layout: a mouse button and a
		// wheel notch are absent from the 26-letter table, so this is a PROVABLE identity — and
		// the accessor is asked to agree, which is what turns "we skip the call because it would
		// be a no-op" into a fact rather than a hope.
		const TArray<FKey> Resolved =
			FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(*Row, TArray<FKey>(), Dvorak.Layout.Get());

		TestEqual(*FString::Printf(TEXT("Row '%s' labels every one of its keys"), Expected.ActionId),
			Resolved.Num(), Row->QwertyReferenceKeys.Num());

		for (int32 Index = 0; Index < Resolved.Num() && Index < Row->QwertyReferenceKeys.Num(); ++Index)
		{
			TestTrue(*FString::Printf(TEXT("Row '%s': %s is UNCHANGED on Dvorak"),
				Expected.ActionId, *Describe(Row->QwertyReferenceKeys[Index])),
				Resolved[Index] == Row->QwertyReferenceKeys[Index]);

			TestTrue(*FString::Printf(TEXT("Row '%s': ...and GetPositionalKey agrees it is an identity (%s)"),
				Expected.ActionId, *Describe(Row->QwertyReferenceKeys[Index])),
				Dvorak.Layout->GetPositionalKey(Row->QwertyReferenceKeys[Index]) == Row->QwertyReferenceKeys[Index]);
		}

		// The chip therefore renders SOMETHING for every one of these rows - never the "(not
		// bound)" fallback, which is what a keyless row would produce.
		const FString Chip = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(*Row, Resolved).ToString();
		TestFalse(*FString::Printf(TEXT("Row '%s' composes a non-empty key chip"), Expected.ActionId), Chip.IsEmpty());

		// ── (e) ⛔ NO TUNABLE'S VALUE IS TYPED INTO THIS PROSE (see the scanner above) ───
		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' one-liner types NO number - the tunables are NAMED"), Expected.ActionId),
			CarriesADigit(OneLine));
		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' detail page types NO number either"), Expected.ActionId),
			CarriesADigit(Detail));

		// ── (f) EVERY OUTBOUND EDGE RESOLVES (`HELP-§7`) ────────────────────────────────
		// ⚠️ SCOPED TO THIS TASK'S OWN ROWS. The registry-wide walk with its negative control is
		// TASK-852's, and this is deliberately NOT a substitute for it: a dangling id renders
		// NOTHING and logs nothing, so the graph needs a total check, not three local ones.
		TestTrue(*FString::Printf(TEXT("Row '%s' carries at least one related control (Jonathan's 'all the controls with it')"),
			Expected.ActionId), Row->RelatedActionIds.Num() > 0);

		for (const FName RelatedId : Row->RelatedActionIds)
		{
			TestNotNull(*FString::Printf(TEXT("Row '%s' related id '%s' resolves to a REAL row"),
				Expected.ActionId, *RelatedId.ToString()),
				FSiegeControlsHelpRegistry::FindAction(RelatedId));
			TestNotEqual(*FString::Printf(TEXT("Row '%s' does not list itself"), Expected.ActionId),
				RelatedId, Row->ActionId);
		}
	}

	// ⭐ THE NEGATIVE CONTROL FOR (f), so the resolution claim above is provably able to go red:
	// the same lookup answers NULL for an id that is not in the registry.
	TestNull(TEXT("NEGATIVE CONTROL: a fabricated related id does NOT resolve, so (f) can actually fail"),
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Cards.NoSuchUpgradeRow"))));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 16 — Siegebound.ControlsHelp.TheThreeWheelMeaningsAreThreeDistinctRows   ⭐⭐
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ `STACK-§4`'s INSTRUCTION, MADE MACHINE-CHECKABLE: *"`HELP-§`'s controls screen MUST
 *  distinguish all three [wheel meanings]"* — the group-pick circles (world-space radii), the war
 *  map's numbered circles (widget space), and the placement footprint (a scale factor).
 *
 *  ⛔ THE STANDARD IT ENFORCES IS `HELP-§2`'s OWN: a help screen that conflates them is WORSE
 *  than no help screen. One physical gesture with three meanings and one row to cover them would
 *  teach the player that the wheel does something it does not do in the mode he is in.
 *
 *  ⭐ `SC-§37` — IT MEASURES THE PROPERTY, ⛔ IT DOES NOT TRANSCRIBE A LIST. The wheel-bearing
 *  rows are found by SCANNING every row's reference keys for a wheel key; the expected ids are
 *  then checked against what the scan found. ⇒ it goes red on a wheel row that is missing, on a
 *  wheel row nobody documented, and on two meanings collapsed into one row.
 *
 *  ⚠️⚠️ AND THE CEILING IS DELIBERATE, ⛔ not incidental: `MARK-§4` (as amended by `STACK-§4`)
 *  pins the wheel at EXACTLY THREE consumers and says no fourth may be added without amending
 *  that line again. A fourth wheel row appearing here SHOULD turn this red — that is the law
 *  firing, ⛔ not the test being brittle.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpWheelMeaningsTest,
	"Siegebound.ControlsHelp.TheThreeWheelMeaningsAreThreeDistinctRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpWheelMeaningsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	// ── (a) FIND THE WHEEL ROWS BY SCANNING, ⛔ NEVER BY A HAND-TYPED LIST ───────────────
	auto CarriesAWheelKey = [](const FSiegeControlsHelpAction& Row) -> bool
	{
		return Row.QwertyReferenceKeys.Contains(EKeys::MouseScrollUp)
			|| Row.QwertyReferenceKeys.Contains(EKeys::MouseScrollDown);
	};

	TArray<const FSiegeControlsHelpAction*> WheelRows;
	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		if (CarriesAWheelKey(Row))
		{
			WheelRows.Add(&Row);
		}
	}

	// ⭐ THE CEILING, FROM `MARK-§4` AS AMENDED. Three consumers, three meanings, three rows.
	TestEqual(TEXT("⭐⭐ The screen teaches EXACTLY THREE wheel meanings (MARK-§4's pinned ceiling of three consumers)"),
		WheelRows.Num(), 3);

	// ── (b) AND THEY ARE THE THREE THE LAW NAMES ────────────────────────────────────────
	// ⚠️ Checked against what the SCAN found, so this cannot pass by describing rows that are not
	// there: a missing row fails here AND in (a).
	const TCHAR* ExpectedWheelIds[] =
	{
		TEXT("PickMode.Resize"),        // 1 - the group-pick circles, world uu
		TEXT("Cards.PlacementResize"),  // 2 - the placement footprint, a scale factor
		TEXT("Interface.MapMarks")      // 3 - the war map's numbered circles, widget space
	};

	for (const TCHAR* ExpectedId : ExpectedWheelIds)
	{
		const bool bFound = WheelRows.ContainsByPredicate(
			[ExpectedId](const FSiegeControlsHelpAction* Row) { return Row->ActionId == FName(ExpectedId); });

		TestTrue(*FString::Printf(TEXT("Wheel meaning '%s' has a row of its own"), ExpectedId), bFound);
	}

	// ── (c) ⭐⭐ THEY ARE TOLD APART — the claim `HELP-§2` actually cares about ──────────
	// Three rows that read identically would satisfy (a) and (b) and still be exactly the
	// conflation the law forbids. ⇒ id, headline, one-liner and detail must all be pairwise
	// DISTINCT, and the three must sit in DIFFERENT CATEGORIES, which is what puts them under
	// three different headings on screen instead of in one indistinguishable run.
	for (int32 Left = 0; Left < WheelRows.Num(); ++Left)
	{
		for (int32 Right = Left + 1; Right < WheelRows.Num(); ++Right)
		{
			const FSiegeControlsHelpAction& A = *WheelRows[Left];
			const FSiegeControlsHelpAction& B = *WheelRows[Right];

			const FString Pair = FString::Printf(TEXT("'%s' vs '%s'"), *A.ActionId.ToString(), *B.ActionId.ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: different ids"), *Pair),
				A.ActionId.ToString(), B.ActionId.ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: different headlines"), *Pair),
				A.DisplayName.ToString(), B.DisplayName.ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: ⭐ different one-liners - each says WHEN its wheel applies"), *Pair),
				FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(A).ToString(),
				FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(B).ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: different detail pages"), *Pair),
				FSiegeControlsHelpRegistry::ComposeDetailForDisplay(A).ToString(),
				FSiegeControlsHelpRegistry::ComposeDetailForDisplay(B).ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: ⭐ different categories, so they land under different headings"), *Pair),
				A.Category.ToString(), B.Category.ToString());
		}
	}

	// ── (d) ⛔ AND NONE OF THEM IS A POINTER ROW ────────────────────────────────────────
	// A wheel meaning on the pointer lane would render the single "Mouse click" affordance and the
	// wheel would be invisible in the list - the conflation happening in the CHIP rather than in
	// the prose, which is the version of this defect nobody would spot in a diff.
	for (const FSiegeControlsHelpAction* Row : WheelRows)
	{
		TestFalse(*FString::Printf(TEXT("⛔ Wheel row '%s' is not pointer-only"), *Row->ActionId.ToString()),
			Row->bPointerOnly || Row->Lane == ESiegeInputLane::PointerOnly);
	}

	// ── (e) FIXTURE SELF-CHECK: the scanner can answer BOTH ways ────────────────────────
	// ⛔ Without this, (a) would be satisfied by a scanner that matched everything or nothing.
	// (A claim about the SCANNER, ⛔ not about the registry.)
	TestTrue(TEXT("FIXTURE SELF-CHECK: the wheel scanner says YES to a synthetic wheel row"),
		CarriesAWheelKey(MakeRow(ESiegeInputLane::RawNonLetter, { EKeys::MouseScrollUp }, false, false)));
	TestFalse(TEXT("FIXTURE SELF-CHECK: ...and NO to a synthetic row with no wheel key"),
		CarriesAWheelKey(MakeRow(ESiegeInputLane::RawNonLetter, { EKeys::LeftMouseButton }, false, false)));

	if (WheelRows.Num() > 0)
	{
		FString Found;
		for (const FSiegeControlsHelpAction* Row : WheelRows)
		{
			if (!Found.IsEmpty())
			{
				Found += TEXT(", ");
			}
			Found += Row->ActionId.ToString();
		}
		AddInfo(FString::Printf(TEXT("Wheel meanings documented: %s"), *Found));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 17 — Siegebound.ControlsHelp.RightClickIsTaughtOnlyByTheRowsThatOwnIt   ⭐⭐
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ TASK-870. TEST 16's SHAPE, APPLIED TO THE SECOND GESTURE — and carrying the guard on the
 *  one sentence a PLAYER, not a reader, proved false.
 *
 *  ⛔ THE DEFECT IT EXISTS TO KEEP DEAD. `Interface.WarMap`'s detail SHIPPED claiming the map is
 *  "closed by right-click or Escape, polled every frame", while `Interface.MapMarks` said "a
 *  right-click that hits no circle does nothing at all". Both were on screen at once and one of
 *  them was false. 🧑 Jonathan settled it in one click: "opening the war map and right clicking
 *  empty ground does not cause it to close, the map seems to function exactly as it should"
 *  ⇒ `Interface.MapMarks` is TRUE and the war-map page's right-click claim was the false one.
 *
 *  ⭐⭐ AND THE REASON THIS HAS TO BE A TEST RATHER THAN A REVIEW NOTE — `SC-§42`: THREE careful
 *  readers each opened `WarMapWidget.cpp`, each saw `NativeOnMouseButtonDown` return
 *  `FReply::Handled()` on every right button, and each reasoned to the RIGHT answer with an
 *  instrument that COULD NOT CONFIRM IT. A HANDLED EVENT IS NOT AN ACTIONED EVENT. ⇒ nothing in
 *  the source can re-derive this; what a test CAN do is refuse to let the sentence come back.
 *
 *  ⭐ THE REMEDY IS THE SHIPPED ONE, ⛔ NOT AN INVENTED ONE: test 14 keeps a scrapped right-click
 *  route off `Cards.Discard` by asserting the page NEVER NAMES IT, rather than by asserting a
 *  denial sentence. Same here — the war-map page is SILENT on right-click and DELEGATES to the
 *  row that owns the gesture, which is `HELP-§2`'s "one definition, two renderings".
 *
 *  ⛔ `SC-§37` — IT MEASURES PROPERTIES. The right-click rows are found by SCANNING every row's
 *  reference keys; the expected ids are then checked against what the scan found. ⇒ it goes red
 *  on a right-click row that is missing, on one nobody documented, and on two meanings collapsed
 *  into one row.
 *
 *  ⚠️⚠️ THE CEILING IS DELIBERATE, ⛔ not incidental — the same way test 16's three is.
 *  `CARDBAR-§8` is the right-click CONSUMER REGISTRY and its standing rule is that "the next
 *  proposal to consume RMB starts from THIS TABLE, not from memory". A fourth right-click row
 *  appearing here SHOULD turn this red: that is the law firing and asking for the table to be
 *  amended, ⛔ not the test being brittle.
 *
 *  ⚠️ WHAT IT CANNOT PROVE (`SC-§32`): nothing here clicks a mouse, opens PIE or paints a row.
 *  That right-click behaves as documented closed on Jonathan's own observation and on nothing
 *  else — which is precisely `SC-§42`'s point.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpRightClickMeaningsTest,
	"Siegebound.ControlsHelp.RightClickIsTaughtOnlyByTheRowsThatOwnIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpRightClickMeaningsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	// ── (a) FIND THE RIGHT-CLICK ROWS BY SCANNING, ⛔ NEVER BY A HAND-TYPED LIST ─────────
	auto CarriesTheRightButton = [](const FSiegeControlsHelpAction& Row) -> bool
	{
		return Row.QwertyReferenceKeys.Contains(EKeys::RightMouseButton);
	};

	TArray<const FSiegeControlsHelpAction*> RightClickRows;
	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		if (CarriesTheRightButton(Row))
		{
			RightClickRows.Add(&Row);
		}
	}

	// ⭐ THE CEILING, FROM `CARDBAR-§8` AS IT STANDS AFTER JONATHAN'S OBSERVATION. Its table
	// lists consumers 1 (group-pick cancel), 2 (spell-targeting cancel), 3 (war-map close),
	// 4 (placement cancel) and 4b (mark delete), with row 5 struck when he cut right-click
	// discard. THREE rows cover them: consumers 1, 2 and 4 are ONE BOUND ACTION serving three
	// mutually-exclusive modes and are documented as one row each side of the pick boundary
	// (Cards.Cancel + PickMode.Cancel), and 4b is Interface.MapMarks. ⛔ CONSUMER 3 HAS NO ROW
	// ON THIS LANE ON PURPOSE — it is the poll Jonathan's right-click never reaches.
	TestEqual(TEXT("⭐⭐ The screen teaches EXACTLY THREE right-click meanings (CARDBAR-§8's registry, after the war-map close was refuted by observation)"),
		RightClickRows.Num(), 3);

	// ── (b) AND THEY ARE THE THREE THE REGISTRY NAMES ───────────────────────────────────
	// ⚠️ Checked against what the SCAN found, so this cannot pass by describing rows that are
	// not there: a missing row fails here AND in (a).
	const TCHAR* ExpectedRightClickIds[] =
	{
		TEXT("Cards.Cancel"),       // consumers 2 + 4 - back out of placing or targeting
		TEXT("PickMode.Cancel"),    // consumer 1      - abandon a group-order pick
		TEXT("Interface.MapMarks")  // consumer 4b     - delete one of your own map circles
	};

	for (const TCHAR* ExpectedId : ExpectedRightClickIds)
	{
		const bool bFound = RightClickRows.ContainsByPredicate(
			[ExpectedId](const FSiegeControlsHelpAction* Row) { return Row->ActionId == FName(ExpectedId); });

		TestTrue(*FString::Printf(TEXT("Right-click meaning '%s' has a row of its own"), ExpectedId), bFound);
	}

	// ── (c) ⭐⭐ THEY ARE TOLD APART — the claim `HELP-§2` actually cares about ──────────
	// Rows that read identically would satisfy (a) and (b) and still be exactly the conflation
	// the law forbids. ⇒ id, headline, one-liner and detail must all be pairwise DISTINCT, and
	// the three must sit in DIFFERENT CATEGORIES, which is the STRUCTURAL form of "each says
	// WHEN its gesture applies": three headings, three modes, no run of look-alike rows.
	for (int32 Left = 0; Left < RightClickRows.Num(); ++Left)
	{
		for (int32 Right = Left + 1; Right < RightClickRows.Num(); ++Right)
		{
			const FSiegeControlsHelpAction& A = *RightClickRows[Left];
			const FSiegeControlsHelpAction& B = *RightClickRows[Right];

			const FString Pair = FString::Printf(TEXT("'%s' vs '%s'"), *A.ActionId.ToString(), *B.ActionId.ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: different ids"), *Pair),
				A.ActionId.ToString(), B.ActionId.ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: different headlines"), *Pair),
				A.DisplayName.ToString(), B.DisplayName.ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: ⭐ different one-liners - each says WHEN its right-click applies"), *Pair),
				FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(A).ToString(),
				FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(B).ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: different detail pages"), *Pair),
				FSiegeControlsHelpRegistry::ComposeDetailForDisplay(A).ToString(),
				FSiegeControlsHelpRegistry::ComposeDetailForDisplay(B).ToString());

			TestNotEqual(*FString::Printf(TEXT("%s: ⭐ different categories, so they land under different headings"), *Pair),
				A.Category.ToString(), B.Category.ToString());
		}
	}

	// ── (d) ⛔⛔ THE REPAIR ITSELF — THE WAR-MAP PAGE TEACHES NO RIGHT-CLICK AT ALL ──────
	const FSiegeControlsHelpAction* const WarMapRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.WarMap")));

	if (!TestNotNull(TEXT("The war-map row is still in the registry (it is named in test 1's RequiredIds)"), WarMapRow))
	{
		return false;
	}

	// (d1) DATA: it never carried the right button and it must not acquire one. A chip claiming
	// the right button OPENS the map would be the same falsehood one field over.
	TestFalse(TEXT("⛔ The war-map row carries NO right-button reference key"),
		CarriesTheRightButton(*WarMapRow));

	// (d2) PROSE: the fragment scan, the same instrument test 14 uses to keep the scrapped
	// discard route off `Cards.Discard`. ⛔ Checked on the COMPOSED text (so a future TODO
	// fallback cannot smuggle anything past it), and on the one-liner as well as the detail.
	const FString WarMapOneLine = FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(*WarMapRow).ToString();
	const FString WarMapDetail  = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*WarMapRow).ToString();

	const TCHAR* RefutedRouteFragments[] = { TEXT("right-click"), TEXT("right click"), TEXT("right button") };
	for (const TCHAR* Fragment : RefutedRouteFragments)
	{
		TestFalse(*FString::Printf(TEXT("⛔ The war-map one-liner does not mention '%s'"), Fragment),
			WarMapOneLine.Contains(Fragment, ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("⛔ The war-map detail page does not mention '%s' - Jonathan measured that it does not close the map"), Fragment),
			WarMapDetail.Contains(Fragment, ESearchCase::IgnoreCase));
	}

	// ⭐⭐ THE POSITIVE CONTROL FOR (d2), AND IT IS A SAME-ROLE ONE (`SC-§39`): the identical
	// scan, run over the COMPOSED DETAIL OF A REAL REGISTRY ROW, must find the fragment where
	// one legitimately exists. `Interface.MapMarks` is the row that OWNS right-click on the war
	// map and its page says so out loud. ⛔ Without this, the six zeros above would be
	// indistinguishable from a scanner that matches nothing.
	const FSiegeControlsHelpAction* const MarksRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.MapMarks")));

	if (TestNotNull(TEXT("The map-marks row is in the registry (it is the positive control's subject)"), MarksRow))
	{
		TestTrue(TEXT("⭐ POSITIVE CONTROL: the SAME scan DOES find 'right-click' on the row that owns the gesture, so the zeros above are a measurement and not a blind instrument"),
			FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*MarksRow).ToString()
				.Contains(TEXT("right-click"), ESearchCase::IgnoreCase));
	}

	// (d3) ⭐ AND IT DELEGATES RATHER THAN GOING SILENT. Removing a false sentence without
	// putting the true owner on the page would answer the player with nothing. The related
	// block renders `Interface.MapMarks`'s own chip and prose underneath this page, so the
	// gesture is explained exactly once in the whole registry.
	TestTrue(TEXT("⭐ The war-map page DELEGATES right-click to the row that owns it (Interface.MapMarks is a related control)"),
		WarMapRow->RelatedActionIds.Contains(FName(TEXT("Interface.MapMarks"))));

	// ⛔ THE EDGE MUST RESOLVE, or it renders NOTHING and logs nothing (ComposeDetailContent
	// `continue`s past a dangling id). Test 9 walks the whole registry for this; the claim is
	// repeated here because THIS test is the one that would be read if the delegation broke.
	for (const FName RelatedId : WarMapRow->RelatedActionIds)
	{
		TestNotNull(*FString::Printf(TEXT("War-map related id '%s' resolves to a REAL row"), *RelatedId.ToString()),
			FSiegeControlsHelpRegistry::FindAction(RelatedId));
	}

	// ── (e) FIXTURE SELF-CHECKS: both scanners can answer BOTH ways ─────────────────────
	// ⛔ Without these, (a) would be satisfied by a key scanner that matched everything or
	// nothing, and (d2) by a fragment scanner that could never say yes.
	// (Claims about the SCANNERS, ⛔ not about the registry.)
	TestTrue(TEXT("FIXTURE SELF-CHECK: the right-button scanner says YES to a synthetic right-click row"),
		CarriesTheRightButton(MakeRow(ESiegeInputLane::RawNonLetter, { EKeys::RightMouseButton, EKeys::Escape }, false, false)));
	TestFalse(TEXT("FIXTURE SELF-CHECK: ...and NO to a synthetic row with only the LEFT button"),
		CarriesTheRightButton(MakeRow(ESiegeInputLane::RawNonLetter, { EKeys::LeftMouseButton }, false, false)));

	// ⛔ THE NEGATIVE CONTROL FOR (d3): the same lookup answers NULL for an id that is not in
	// the registry, so "the edge resolves" is provably able to go red.
	TestNull(TEXT("NEGATIVE CONTROL: a fabricated related id does NOT resolve"),
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.NoSuchMapRow"))));

	if (RightClickRows.Num() > 0)
	{
		FString Found;
		for (const FSiegeControlsHelpAction* Row : RightClickRows)
		{
			if (!Found.IsEmpty())
			{
				Found += TEXT(", ");
			}
			Found += Row->ActionId.ToString();
		}
		AddInfo(FString::Printf(TEXT("Right-click meanings documented: %s"), *Found));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 18 — Siegebound.ControlsHelp.EveryRelatedActionIdResolvesToARealRow   ⭐⭐
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ TASK-852. `HELP-§7`'s TRIPWIRE, ⛔ FILED UNDER A NAME THAT SAYS WHAT IT CHECKS — and the
 *  ONE thing it never had: a proof that it can go RED.
 *
 *  ⛔⛔ THIS IS AN EXTRACTION, ⛔ NOT A NEW CHECK. The walk below is the one that used to sit
 *  inside test 9 (`EveryRowHasAuthoredDetail`). It was MOVED here, ⛔ not copied — the registry
 *  edge walk exists in exactly ONE place in this file, and test 9 now carries a pointer to it.
 *
 *  ⚖️⭐⭐ WHY THE MOVE WAS WORTH A TASK, because it is the reusable lesson and it cost real work:
 *  the walk was CORRECT, SHIPPED and GREEN — and it was READ AS ABSENT BY TWO INDEPENDENT
 *  READERS, because it lived under a name about detail authoring and its own docstring never
 *  mentioned it. `HELP-§7` was written declaring *"there is no referential-integrity check on
 *  this field"*; a QA verdict ruled a new test in on that premise; `TASK-852` was boarded to
 *  build it. A third reader found it in TEN SECONDS — and only because he was handed the symbol.
 *  ⇒ `SC-§40` cl. 11(b): AN ASSERTION FILED UNDER AN UNRELATED NAME IS, OPERATIONALLY, AN
 *  ASSERTION THAT DOES NOT EXIST. It will be re-implemented, or a LAW will be written declaring
 *  it missing and a TASK boarded on that law. Both happened here.
 *
 *  ⛔ WHAT IT GUARDS, and why the runtime cannot be trusted to tell you: a `RelatedActionIds`
 *  entry naming a row that does not exist renders NOTHING and logs NOTHING. The composer does
 *  `const FSiegeControlsHelpAction* const RelatedRow = FindAction(RelatedId); if (RelatedRow ==
 *  nullptr) { continue; }` — so a dead link is a control silently missing from Jonathan's page,
 *  with no warning anywhere. ⇒ the DATA must be right, because nothing downstream will complain.
 *  (⚠️ That half of `HELP-§7` is TRUE and was re-measured at source; only the "no check exists"
 *  half was struck.)
 *
 *  ⛔ `SC-§37` — DERIVED, ⛔ NEVER TRANSCRIBED. There is no hand-typed edge list anywhere below.
 *  The rows come from `GetActions()` and the ids come from the live `Row.RelatedActionIds`
 *  field, so this test covers edges added AFTER it was written without anyone remembering to
 *  come back. ⛔ It does not know, and must not know, how many edges there are.
 *
 *  ⭐⭐ AND THE PART THAT WAS GENUINELY MISSING — THE NEGATIVE CONTROL (`SC-§39` / `SHIP-§9`).
 *  Every edge in the registry resolves today and always has, so this walk had NEVER been
 *  observed failing: a green from an instrument that cannot go red is indistinguishable from no
 *  test at all. Section (c) drives a fabricated id and a self-edge through the SAME lambda and
 *  the SAME equality this walk uses, and asserts each condition fires exactly once.
 *  ⭐ The precedent is shipped, ⛔ not invented: test 15(f) and test 17 already `TestNull` a
 *  fabricated id. This generalises that from one task's rows to the whole graph.
 *
 *  ⚠️ WHAT IT CANNOT PROVE (`SC-§32`): nothing here renders a page. That a resolved edge
 *  actually PAINTS its related block closes on pixels or Jonathan's eyes (`HELP-§3`). This test
 *  proves the graph is sound, ⛔ not that the screen drew it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeControlsHelpRelatedEdgeIntegrityTest,
	"Siegebound.ControlsHelp.EveryRelatedActionIdResolvesToARealRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeControlsHelpRelatedEdgeIntegrityTest::RunTest(const FString& Parameters)
{
	using namespace SiegeControlsHelpTestUtils;

	// ⭐⭐ THE ONE PREDICATE, AND IT IS ONE ON PURPOSE. The registry walk in (a) and the negative
	// control in (c) both run through THIS lambda and through nothing else. ⇒ "the walk can go
	// red" is a claim about the code that ACTUALLY WALKS, ⛔ not about a lookalike written
	// beside it — which is the failure mode `SC-§39` exists to catch.
	auto EdgeResolves = [](const FName RelatedId) -> bool
	{
		return FSiegeControlsHelpRegistry::FindAction(RelatedId) != nullptr;
	};

	const TArray<FSiegeControlsHelpAction>& Rows = FSiegeControlsHelpRegistry::GetActions();

	if (!TestTrue(TEXT("The registry is non-empty (there is a graph to check at all)"), Rows.Num() > 0))
	{
		return false;
	}

	// ⛔ BY VALUE, ⛔ not through `Rows` — see the closing assertion for why the distinction is
	// load-bearing rather than stylistic.
	const int32 RowCountBefore = Rows.Num();

	// ── (a) THE WALK — ⛔ EVERY ROW, ⛔ EVERY OUTBOUND EDGE ──────────────────────────────
	// ⚠️ This loop is `GetActions()` and NOT test 1's `RequiredIds[]`. That array is a required
	// SUBSET of rows and validates no edges at all — naming it as though it were this check is
	// exactly how "the wrong test does not do it" got written down as "no test does it".
	int32 EdgesWalked   = 0;
	int32 RowsWithEdges = 0;

	for (const FSiegeControlsHelpAction& Row : Rows)
	{
		const FString RowName = Row.ActionId.ToString();

		if (Row.RelatedActionIds.Num() > 0)
		{
			++RowsWithEdges;
		}

		for (const FName RelatedId : Row.RelatedActionIds)
		{
			++EdgesWalked;

			// ⭐ THE FAILURE MESSAGE NAMES BOTH ENDS — the OWNING ROW and the MISSING ID — and
			// says what the player loses. The old message was one line among a page of
			// detail-authoring failures; a reader who saw it had to go and find out which row
			// and which id, which is half of why nobody knew this check was here.
			TestTrue(*FString::Printf(
				TEXT("⛔ DANGLING EDGE - row '%s' lists related control '%s', and NO SUCH ROW IS REGISTERED. ComposeDetailContent will 'continue' past it: that block renders NOTHING and logs NOTHING, so the player silently loses a control off this page."),
				*RowName, *RelatedId.ToString()),
				EdgeResolves(RelatedId));

			// ⛔ AND THE SECOND DEFECT, which a resolve check ALONE cannot see: a row pointing at
			// ITSELF resolves perfectly and still renders nothing (the composer drops it), so it
			// needs its own condition rather than riding on the one above.
			TestNotEqual(*FString::Printf(
				TEXT("⛔ SELF-REFERENCE - row '%s' lists ITSELF as a related control; it resolves, and the composer still drops it."),
				*RowName),
				RelatedId, Row.ActionId);
		}
	}

	// ── (b) ⛔ THE VACUITY GUARD — a walk over ZERO edges is green and proves nothing ────
	// ⚠️ Deliberately NOT a count: `SC-§37`. It asserts that edges EXIST, never how many, so
	// adding or removing a related control cannot turn this red for no reason.
	TestTrue(TEXT("⭐ At least one row carries an outbound edge, so (a) is a measurement and not a loop over an empty list"),
		EdgesWalked > 0);

	AddInfo(FString::Printf(TEXT("Walked %d outbound edge(s) across %d of %d registry rows. ⛔ Derived from GetActions() - this test does not know the number and must not."),
		EdgesWalked, RowsWithEdges, Rows.Num()));

	// ── (c) ⭐⭐ THE NEGATIVE CONTROL — ⛔ THE POINT OF THIS TASK ────────────────────────
	// ⛔ DERIVED, ⛔ NOT PICKED (`SC-§40` cl. 10): the fabricated id is BUILT FROM a real row's
	// id by appending a suffix, so it cannot quietly become a real id the day somebody ships a
	// row whose name a previous author happened to guess. And the proof of its absence is asked
	// of the LIVE REGISTRY below, ⛔ not of a text scan that could go stale.
	const FName   RealId          = Rows[0].ActionId;
	const FString FabricatedText  = RealId.ToString() + TEXT(".NoSuchRelatedRow");
	const FName   FabricatedId(*FabricatedText);

	// ⭐ POSITIVE CONTROL FIRST, same lambda, same call (`SC-§39`): without it, a "does not
	// resolve" answer would be indistinguishable from a lookup that resolves NOTHING.
	TestTrue(*FString::Printf(TEXT("⭐ POSITIVE CONTROL: the SAME lookup DOES resolve a real registry id ('%s'), so the rejection below is a measurement and not a blind instrument"),
		*RealId.ToString()), EdgeResolves(RealId));

	TestFalse(*FString::Printf(TEXT("⭐⭐ NEGATIVE CONTROL: ...and the SAME lookup REJECTS the fabricated id '%s'"),
		*FabricatedText), EdgeResolves(FabricatedId));

	// ⭐⭐ AND THE CONTROL AT FULL STRENGTH — a SYNTHETIC ROW carrying BOTH defects, driven
	// through the walk's OWN two conditions. ⛔ The self-edge is deliberately an id that
	// RESOLVES, so the two conditions are shown to fire on DIFFERENT edges: neither is masking
	// the other, and a self-reference is caught even though the lookup is happy with it.
	FSiegeControlsHelpAction Dangler = MakeRow(ESiegeInputLane::MappedAction, { EKeys::F }, false, false);
	Dangler.ActionId         = RealId;
	Dangler.RelatedActionIds = { RealId, FabricatedId };

	int32 Unresolved = 0;
	int32 SelfEdges  = 0;
	for (const FName RelatedId : Dangler.RelatedActionIds)
	{
		if (!EdgeResolves(RelatedId))
		{
			++Unresolved;
		}
		if (RelatedId == Dangler.ActionId)
		{
			++SelfEdges;
		}
	}

	TestEqual(TEXT("⭐⭐ NEGATIVE CONTROL: the walk's RESOLVE condition rejects exactly the one fabricated edge on the synthetic row - so a dangling edge in the real registry WOULD turn (a) red"),
		Unresolved, 1);
	TestEqual(TEXT("⭐⭐ NEGATIVE CONTROL: the walk's SELF-REFERENCE condition catches exactly the one self-edge - on a DIFFERENT edge, so the two conditions are independent"),
		SelfEdges, 1);

	// ⛔ THE SYNTHETIC ROW NEVER ENTERED THE REGISTRY — it is a local, and `GetActions()` is a
	// function-local static built once. Stated because a control that mutated the thing it is
	// controlling would poison every test that runs after it.
	// ⚠️ Compared against a COUNT SNAPSHOTTED BY VALUE at the top, ⛔ never against `Rows.Num()`:
	// `Rows` is a REFERENCE to that same static array, so `GetActions().Num() == Rows.Num()` is
	// `X == X` and could never go red. That would be this test's own `SC-§39` defect, written
	// into the very section that exists to prove an assertion CAN fail.
	TestEqual(TEXT("⛔ The negative control left the registry untouched (row count unchanged from the snapshot taken before it ran)"),
		FSiegeControlsHelpRegistry::GetActions().Num(), RowCountBefore);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
