// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Siegebound/CardHandWidget.h"
#include "Siegebound/SiegeControlsHelpWidget.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the CARD-BAR KEY-LABEL PULL SEAM ═══
 *  (CARDBAR batch, TASK-807. Subject: UCardHandWidget::GetSlotKeyLabel and its pure half
 *  UCardHandWidget::ComposeSlotKeyLabel. Law: `CARDBAR-§2`/`§3`, `KBD-§4`, `HELP-§1`,
 *  `SC-§37`. QA gate: TASK-810. Compile + suite run + the pixel check: TASK-811.)
 *
 *  ⛔ NO EXISTING TEST FILE FITS, AND THAT WAS CHECKED BEFORE THIS ONE WAS CREATED.
 *  Tests/ holds no card/hand frame at all: SiegeDeckSlotsTest.cpp is the DECK-BUILDER's fixed
 *  slots (a save-game migration subject, not the in-match hand), and SiegeControlsHelpTest.cpp
 *  is the TAB overlay's own suite — putting a card-bar subject in it would make `HELP-§6`'s
 *  "ONE FILE for the whole feature" rule mean two different features. ⇒ new frame, declared.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE PROBLEM THIS FILE EXISTS TO SOLVE, STATED FIRST BECAUSE IT IS THE WHOLE DESIGN:
 *
 *      A CARD-KEY-LABEL TEST PASSES TRIVIALLY WHETHER OR NOT THE CODE IS CORRECT.
 *
 *  The bug being guarded against is a SECOND key translation (`HELP-§1`'s named exception,
 *  SiegePlayerController.h:1222-1226). It is invisible on QWERTY, so the obvious instrument is
 *  a Dvorak fixture — and ⛔ FOR DIGITS EVEN THAT IS BLIND, because `KBD-§4` deliberately
 *  leaves the number row untranslated (SiegeKeyboardLayoutStatics.cpp:57-63 tables the 26
 *  LETTERS and nothing else). ⇒ `GetSlotKeyLabel(0)` reads "1" on US-Dvorak under a correct
 *  implementation AND under a double-translating one. A test that asserted `== "1"` would be
 *  `SC-§37`'s worked failure: green forever, discriminating nothing.
 *
 *  ⭐ SO EVERY ASSERTION BELOW IS BUILT TO BE ABLE TO FAIL, BY THREE DEVICES:
 *    (a) ⛔ NOTHING IS TRANSCRIBED. Every expected label is DERIVED — from the shipped
 *        `Cards.Play` registry row, or from FKey::GetDisplayName, or from the shipped chip
 *        composer itself. There is no literal `"1"` anywhere in this file.
 *    (b) ⭐ THE APPLIED KEYS ARE INJECTED, AND THEY ARE **LETTERS**. Test 2 pushes `F`'s
 *        US-Dvorak answer through the mapped lane, where a second translation would turn `U`
 *        into `G` — a difference a digit fixture physically cannot produce. Test 2 asserts the
 *        digit blindness explicitly, so nobody "simplifies" the letter back to a `1`.
 *    (c) ⭐ EACH SLOT GETS A **DISTINCT** APPLIED KEY (test 1), so an off-by-one, or a
 *        narrowing that always reads action 0, goes red instead of reading plausibly.
 *
 *  ⛔ WHAT THESE TESTS CANNOT PROVE (`SC-§32`): nothing here presses a key, opens PIE, builds
 *  a Slate tree or asks Windows anything. That the chip is VISIBLE, positioned where the Play
 *  button was, and hidden on an empty return closes on PIXELS and on nothing else (`AS-§6`
 *  A(e), `CARDBAR-§4`) — that is TASK-809's screenshot and TASK-811's in-PIE look. And that
 *  `GetPositionalKey` appears nowhere on this path is asserted BEHAVIOURALLY by test 2; the
 *  authoritative check is TASK-810's grep of the diff.
 *
 *  M8 DECLARATION (verbatim): adds no replicated property, no new replicated class, no new
 *  relevancy tier, no RPC. This is a test file; it adds no shipped surface at all.
 */

namespace SiegeCardHandKeyLabelTestUtils
{
	/**
	 *  The US-Dvorak entries this file needs, taken from the SHIPPED fixture at
	 *  Tests/SiegeKeyboardLayoutTest.cpp:204-229 (and mirroring the subset
	 *  Tests/SiegeControlsHelpTest.cpp:85-101 already lifts from it) rather than re-derived —
	 *  a second, independently-guessed Dvorak table is exactly the drifting copy this project
	 *  keeps paying for.
	 *
	 *  ⭐ `F -> U` is Jonathan's own worked example and `U -> G` is THE SECOND HALF OF THE
	 *  TRAP: applying the map twice turns `F` into `G`. Both are required or test 2 cannot
	 *  detect the defect at all.
	 *
	 *  ⛔⛔ THE DIGITS ARE ABSENT ON PURPOSE AND THEIR ABSENCE IS ASSERTED (test 2, section 2).
	 *  `KBD-§4` excludes the number row BY DESIGN; GetPositionalKey returns its input unchanged
	 *  for any key with no entry (SiegeKeyboardLayoutSubsystem.h:238-240). Adding `One -> …`
	 *  here to "make the test more realistic" would be a false fixture AND would silently
	 *  destroy the one property test 2 exists to measure.
	 */
	static TMap<FKey, FKey> MakeDvorakTranslation()
	{
		TMap<FKey, FKey> Translation;
		Translation.Add(EKeys::F, EKeys::U);  // ⭐ the key that MOVES
		Translation.Add(EKeys::U, EKeys::G);  // ⭐ the second hop a double-translate would take
		Translation.Add(EKeys::T, EKeys::Y);
		Translation.Add(EKeys::R, EKeys::P);
		Translation.Add(EKeys::E, EKeys::Period);
		Translation.Add(EKeys::C, EKeys::J);
		Translation.Add(EKeys::Q, EKeys::Apostrophe);
		return Translation;
	}

	/**
	 *  A USiegeKeyboardLayoutSubsystem inside a throwaway UGameInstance.
	 *
	 *  ⚠️ THE OUTER IS NOT OPTIONAL: UGameInstanceSubsystem is UCLASS(Abstract, Within =
	 *  GameInstance), so a bare NewObject lands in the transient package and trips the
	 *  ClassWithin check in StaticAllocateObject. Cloned from SiegeControlsHelpTest.cpp:
	 *  118-133, which cloned it from SiegeKeyboardLayoutTest.cpp:453-462.
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

	/** The SHIPPED `Cards.Play` row — the single source every expectation in this file derives from. */
	static const FSiegeControlsHelpAction* PlayRow()
	{
		return FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Cards.Play")));
	}

	/**
	 *  ⛔ THE ONE PLACE A KEY BECOMES CHARACTERS IN THIS FILE, and it is the SAME call
	 *  ComposeKeyChipLabel makes (SiegeControlsHelpWidget.cpp:1211) — including the explicit
	 *  `false`, because the engine's own default is `true` and the long form would silently
	 *  make every expectation here disagree with the shipped chip.
	 */
	static FString ShortName(const FKey& Key)
	{
		return Key.GetDisplayName(/*bLongDisplayName=*/false).ToString();
	}

	static FString Describe(const FKey& Key)
	{
		return Key.IsValid() ? Key.ToString() : FString(TEXT("<invalid>"));
	}

	/**
	 *  A provider that always answers "nothing is mapped" — drives the reference-key fallback.
	 *
	 *  ⚠️ BIND TO THE FUNCTION, ⛔ NEVER TO ITS ADDRESS. TFunctionRef deliberately does
	 *  not coerce a function type to a pointer, so the ampersand form passes a PRVALUE POINTER
	 *  and does not compile — recorded verbatim at SiegeKeyboardLayoutStatics.h:42-46, which
	 *  had to learn it once already.
	 */
	static TArray<FKey> NoAppliedKeys(const FSiegeControlsHelpAction& /*Row*/)
	{
		return TArray<FKey>();
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 1 — Siegebound.CardHand.SlotKeyLabelNarrowsToItsOwnCardAction
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE WIRING CLAIM: slot N's label comes from `IA_Card{N+1}` AND FROM NOTHING ELSE.
 *
 *  THE DEFECT THIS CATCHES: `Cards.Play` is ONE registry row carrying SIX actions
 *  (SiegeControlsHelpWidget.cpp:420-427). Asking the resolver for that row unnarrowed would
 *  answer all six keys and every card would read "1 / 2 / 3 / 4 / 5 / 6"; narrowing to a fixed
 *  index would put slot 0's key on all six; an off-by-one would shift the whole bar by one.
 *  ⛔ ALL THREE READ PLAUSIBLY ON SCREEN and all three are wrong.
 *
 *  ⭐ HOW IT IS MADE FALSIFIABLE: every slot is given a DISTINCT applied key, keyed off the
 *  action the implementation actually asked about. If the narrowing picks the wrong action,
 *  the label is another slot's letter and the assertion names both.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardHandSlotKeyLabelNarrowingTest,
	"Siegebound.CardHand.SlotKeyLabelNarrowsToItsOwnCardAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardHandSlotKeyLabelNarrowingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardHandKeyLabelTestUtils;

	const FSiegeControlsHelpAction* const Row = PlayRow();
	if (!TestNotNull(TEXT("The shipped Cards.Play registry row exists"), Row))
	{
		return false;
	}

	// ── FIXTURE SELF-CHECK ──────────────────────────────────────────────────────────────
	// ⚠️ THE ONLY PLACE THIS FILE NAMES AN ASSET, AND IT IS A CLAIM ABOUT THE REGISTRY, NOT
	// ABOUT A LABEL: the whole seam rests on `Cards.Play`'s action list being in SLOT ORDER
	// (index 0 = IA_Card1). If someone reorders that array, every label below would still
	// "match its action" while the bar lied — so the ordering is asserted directly, once.
	TestEqual(TEXT("FIXTURE: Cards.Play carries exactly six card actions"), Row->Actions.Num(), 6);
	if (Row->Actions.Num() != 6)
	{
		return false;
	}

	for (int32 Slot = 0; Slot < 6; ++Slot)
	{
		const FString ExpectedAssetToken = FString::Printf(TEXT("IA_Card%d"), Slot + 1);
		TestTrue(FString::Printf(TEXT("FIXTURE: Cards.Play action %d is %s (the registry list is in SLOT ORDER)"),
			Slot, *ExpectedAssetToken),
			Row->Actions[Slot].ToSoftObjectPath().ToString().Contains(ExpectedAssetToken));
	}

	// ── THE CLAIM ───────────────────────────────────────────────────────────────────────
	// Six distinct letters, one per action. ⛔ Deliberately NOT digits: distinct answers are
	// what makes a mis-narrowing visible, and the digit row is exactly where every wrong
	// implementation still looks right.
	const TArray<FKey> DistinctPerSlot = { EKeys::F, EKeys::T, EKeys::R, EKeys::E, EKeys::C, EKeys::Q };
	TestEqual(TEXT("FIXTURE: one distinct probe key per slot"), DistinctPerSlot.Num(), Row->Actions.Num());

	for (int32 Slot = 0; Slot < Row->Actions.Num(); ++Slot)
	{
		// What the implementation handed the provider — captured so the NARROWING itself, not
		// just its result, is under test.
		int32 SeenActionCount = -1;
		FString SeenActionPath;
		bool bSeenPointerOnly = true;
		ESiegeInputLane SeenLane = ESiegeInputLane::PointerOnly;

		const FString Label = UCardHandWidget::ComposeSlotKeyLabel(Slot, /*LayoutSubsystem=*/nullptr,
			[&](const FSiegeControlsHelpAction& SlotRow)
			{
				SeenActionCount  = SlotRow.Actions.Num();
				bSeenPointerOnly = SlotRow.bPointerOnly;
				SeenLane         = SlotRow.Lane;

				TArray<FKey> Applied;
				if (SlotRow.Actions.Num() == 1)
				{
					SeenActionPath = SlotRow.Actions[0].ToSoftObjectPath().ToString();

					// Answer the DISTINCT key for whichever action was actually asked about.
					for (int32 Probe = 0; Probe < Row->Actions.Num(); ++Probe)
					{
						if (SlotRow.Actions[0].ToSoftObjectPath() == Row->Actions[Probe].ToSoftObjectPath())
						{
							Applied.Add(DistinctPerSlot[Probe]);
							break;
						}
					}
				}
				return Applied;
			});

		TestEqual(FString::Printf(TEXT("Slot %d is narrowed to EXACTLY ONE action (not the row's six)"), Slot),
			SeenActionCount, 1);

		TestTrue(FString::Printf(TEXT("Slot %d asks about IA_Card%d (got '%s')"), Slot, Slot + 1, *SeenActionPath),
			SeenActionPath == Row->Actions[Slot].ToSoftObjectPath().ToString());

		// ⭐ The narrowed row must INHERIT the shipped row's lane and pointer flag — that is
		// what keeps the card bar and the TAB screen resolving through the same rules.
		TestFalse(FString::Printf(TEXT("Slot %d's narrowed row is not pointer-only"), Slot), bSeenPointerOnly);
		TestTrue(FString::Printf(TEXT("Slot %d's narrowed row keeps Cards.Play's own lane"), Slot),
			SeenLane == Row->Lane);

		// ⭐⭐ THE ASSERTION THAT GOES RED ON AN OFF-BY-ONE. ⛔ The expected string is DERIVED
		// from the probe key, never typed.
		TestTrue(FString::Printf(TEXT("Slot %d's label is IA_Card%d's applied key '%s' (got '%s')"),
			Slot, Slot + 1, *ShortName(DistinctPerSlot[Slot]), *Label),
			Label == ShortName(DistinctPerSlot[Slot]));

		// ...and it is ONE key's name, ⛔ never the six-key chip the unnarrowed row would compose.
		for (int32 Other = 0; Other < DistinctPerSlot.Num(); ++Other)
		{
			if (Other == Slot)
			{
				continue;
			}
			TestFalse(FString::Printf(TEXT("Slot %d's label does not carry slot %d's key '%s'"),
				Slot, Other, *ShortName(DistinctPerSlot[Other])),
				Label.Contains(ShortName(DistinctPerSlot[Other])));
		}
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 2 — Siegebound.CardHand.SlotKeyLabelIsNeverDoubleTranslated   ⭐⭐ THE KEYSTONE
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ THE ONE-TRANSLATION LAW FOR THE CARD BAR (`CARDBAR-§2`, `HELP-§1`).
 *
 *  THE DEFECT, SPELLED OUT: the layout subsystem rewrites the APPLIED IMC duplicate's keys
 *  wholesale, so on US-Dvorak QueryKeysMappedToAction already answers `U` for an action QWERTY
 *  binds to `F`. An implementation that "helpfully" ran GetPositionalKey over that answer would
 *  print `G` — a key bound to nothing. ⛔ ON QWERTY BOTH IMPLEMENTATIONS LOOK IDENTICAL.
 *
 *  ⛔⛔ AND FOR THIS FEATURE THERE IS A SECOND BLIND SPOT ON TOP OF THE FIRST, WHICH IS WHY
 *  SECTION 2 BELOW EXISTS: the card keys are DIGITS, and `KBD-§4` excludes digits from the
 *  translation table BY DESIGN. ⇒ a Dvorak fixture built from `1`..`6` would ALSO pass either
 *  way. Section 2 asserts that blindness as a property, so the letter probe in section 3 can
 *  never be "tidied" back into a digit by someone who thinks Dvorak alone is the safeguard.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardHandSlotKeyLabelNoDoubleTranslateTest,
	"Siegebound.CardHand.SlotKeyLabelIsNeverDoubleTranslated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardHandSlotKeyLabelNoDoubleTranslateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardHandKeyLabelTestUtils;

	const FSiegeControlsHelpAction* const Row = PlayRow();
	if (!TestNotNull(TEXT("The shipped Cards.Play registry row exists"), Row))
	{
		return false;
	}

	FScratchLayout Qwerty = MakeScratchLayout(TMap<FKey, FKey>());
	FScratchLayout Dvorak = MakeScratchLayout(MakeDvorakTranslation());
	if (!Qwerty.IsValid() || !Dvorak.IsValid())
	{
		AddError(TEXT("Could not construct the scratch layout subsystems - the one-translation claim is UNTESTED."));
		return false;
	}

	// ── SECTION 1: FIXTURE SELF-CHECK ───────────────────────────────────────────────────
	// Without a map that genuinely moves the key TWICE, every assertion below would pass on a
	// broken implementation for the wrong reason.
	TestTrue(TEXT("FIXTURE: the Dvorak fixture reports an ACTIVE remap"), Dvorak.Layout->IsPositionalRemapActive());

	const FKey OnceTranslated  = Dvorak.Layout->GetPositionalKey(EKeys::F);
	const FKey TwiceTranslated = Dvorak.Layout->GetPositionalKey(OnceTranslated);
	TestTrue(TEXT("FIXTURE: the injected map moves F once"), OnceTranslated != EKeys::F);
	TestTrue(TEXT("FIXTURE: applying it AGAIN moves it again - a double-translate is DETECTABLE"),
		TwiceTranslated != OnceTranslated);

	// ── SECTION 2: ⭐⭐ WHY A DIGIT FIXTURE PROVES NOTHING (`KBD-§4`) ─────────────────────
	// This is not commentary — it is the assertion that keeps section 3's letter probe honest.
	// `KBD-§4` excludes the number row BY DESIGN ("a task or QA finding that 'the hotkeys
	// weren't remapped' is WRONG and is answered by this clause"), so the card bar's own six
	// reference keys are FIXED POINTS of the translation. A test written over them could not
	// tell a correct implementation from a double-translating one, at any number of hops.
	for (int32 Slot = 0; Slot < Row->QwertyReferenceKeys.Num(); ++Slot)
	{
		const FKey ReferenceKey = Row->QwertyReferenceKeys[Slot];
		const FKey DigitOnce    = Dvorak.Layout->GetPositionalKey(ReferenceKey);
		const FKey DigitTwice   = Dvorak.Layout->GetPositionalKey(DigitOnce);

		TestTrue(FString::Printf(
			TEXT("`KBD-§4`: the card key %s is UNMOVED by a Dvorak layout (it is not a letter)"),
			*Describe(ReferenceKey)),
			DigitOnce == ReferenceKey);

		TestTrue(FString::Printf(
			TEXT("⛔ ...and therefore a DIGIT fixture cannot detect a double translation on %s - which is why section 3 uses a LETTER"),
			*Describe(ReferenceKey)),
			DigitTwice == DigitOnce);
	}

	// ⭐ The same fact, asserted through the seam rather than through the subsystem: with
	// nothing mapped, the fallback derives the card key and a Dvorak layout leaves it alone.
	// ⛔ The expectation is the REGISTRY's reference key, never a typed digit.
	for (int32 Slot = 0; Slot < Row->Actions.Num(); ++Slot)
	{
		const FString OnQwerty = UCardHandWidget::ComposeSlotKeyLabel(Slot, Qwerty.Layout.Get(), NoAppliedKeys);
		const FString OnDvorak = UCardHandWidget::ComposeSlotKeyLabel(Slot, Dvorak.Layout.Get(), NoAppliedKeys);

		const FString Expected = Row->QwertyReferenceKeys.IsValidIndex(Slot)
			? ShortName(Row->QwertyReferenceKeys[Slot])
			: FString();

		TestTrue(FString::Printf(TEXT("Slot %d's unmapped fallback is the registry's own reference key '%s' (got '%s')"),
			Slot, *Expected, *OnQwerty),
			OnQwerty == Expected);

		TestTrue(FString::Printf(TEXT("Slot %d's label is IDENTICAL on QWERTY and Dvorak ('%s' vs '%s') - `KBD-§4`"),
			Slot, *OnQwerty, *OnDvorak),
			OnQwerty == OnDvorak);
	}

	// ── SECTION 3: ⭐⭐ THE CLAIM, ON A LETTER, THROUGH THE MAPPED LANE ───────────────────
	// The scenario: IA_Card1 has been rebound to the `F` position, and the layout subsystem has
	// ALREADY retargeted the applied context — so QueryKeysMappedToAction answers `OnceTranslated`.
	// The seam must hand that straight through.
	const FString MappedOnDvorak = UCardHandWidget::ComposeSlotKeyLabel(0, Dvorak.Layout.Get(),
		[OnceTranslated](const FSiegeControlsHelpAction& /*SlotRow*/)
		{
			return TArray<FKey>{ OnceTranslated };
		});

	TestTrue(FString::Printf(
		TEXT("⭐ The applied key is handed through UNCHANGED (got '%s', expected the applied %s)"),
		*MappedOnDvorak, *Describe(OnceTranslated)),
		MappedOnDvorak == ShortName(OnceTranslated));

	// ⛔⛔ THE DEFECT, ASSERTED AGAINST DIRECTLY. This line goes red the moment anyone adds a
	// GetPositionalKey call to CardHandWidget.cpp's label path.
	TestTrue(FString::Printf(
		TEXT("⛔ It is NOT translated a SECOND time (a double-translate would read '%s')"),
		*ShortName(TwiceTranslated)),
		MappedOnDvorak != ShortName(TwiceTranslated));

	// ⭐ AND THE SAME APPLIED KEY MUST READ THE SAME ON EVERY LAYOUT. The mapped lane is
	// layout-INDEPENDENT by construction (the translation already happened upstream); any
	// residual GetPositionalKey call would make these two diverge.
	const FString MappedOnQwerty = UCardHandWidget::ComposeSlotKeyLabel(0, Qwerty.Layout.Get(),
		[OnceTranslated](const FSiegeControlsHelpAction& /*SlotRow*/)
		{
			return TArray<FKey>{ OnceTranslated };
		});

	TestTrue(FString::Printf(TEXT("⭐ An APPLIED key reads identically on both layouts ('%s' vs '%s')"),
		*MappedOnQwerty, *MappedOnDvorak),
		MappedOnQwerty == MappedOnDvorak);

	// ⛔ A NULL SUBSYSTEM IS A FAIL-SAFE, NEVER A CRASH (`KBD-§5`) - and it must not change the
	// mapped lane's answer either, because that lane never consults the subsystem at all.
	const FString MappedWithNoSubsystem = UCardHandWidget::ComposeSlotKeyLabel(0, nullptr,
		[OnceTranslated](const FSiegeControlsHelpAction& /*SlotRow*/)
		{
			return TArray<FKey>{ OnceTranslated };
		});

	TestTrue(TEXT("⛔ A null layout subsystem leaves the mapped lane's answer untouched"),
		MappedWithNoSubsystem == MappedOnDvorak);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 3 — Siegebound.CardHand.SlotKeyLabelPrefersTheLiveBindingOverTheReferenceKey
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE REBIND CLAIM: the label follows IMC_Hero, ⛔ not the registry's QWERTY column.
 *
 *  THE DEFECT THIS CATCHES, AND IT IS THE ONE `CARDBAR-§2`'s headline is really about: an
 *  implementation that skipped the live query and rendered `Row.QwertyReferenceKeys[Slot]`
 *  would look PERFECT today — it would print 1..6, because that is what IMC_Hero binds. It is
 *  a hardcoded digit wearing a resolver's clothes, and it would go on printing `1` after
 *  Jonathan rebound slot 1 to something else. ⇒ asserted by rebinding in the fixture.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardHandSlotKeyLabelFollowsTheBindingTest,
	"Siegebound.CardHand.SlotKeyLabelPrefersTheLiveBindingOverTheReferenceKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardHandSlotKeyLabelFollowsTheBindingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardHandKeyLabelTestUtils;

	const FSiegeControlsHelpAction* const Row = PlayRow();
	if (!TestNotNull(TEXT("The shipped Cards.Play registry row exists"), Row))
	{
		return false;
	}

	if (!TestTrue(TEXT("FIXTURE: Cards.Play has a reference key for slot 0"),
		Row->QwertyReferenceKeys.IsValidIndex(0)))
	{
		return false;
	}

	const FKey ReferenceKey = Row->QwertyReferenceKeys[0];

	// The rebind: something that is emphatically NOT the reference key and NOT a digit.
	const FKey ReboundKey = EKeys::SpaceBar;
	TestTrue(TEXT("FIXTURE: the rebound key differs from the registry's reference key"),
		ReboundKey != ReferenceKey);

	const FString Label = UCardHandWidget::ComposeSlotKeyLabel(0, /*LayoutSubsystem=*/nullptr,
		[ReboundKey](const FSiegeControlsHelpAction& /*SlotRow*/)
		{
			return TArray<FKey>{ ReboundKey };
		});

	TestTrue(FString::Printf(TEXT("⭐ A rebound slot shows its LIVE key '%s' (got '%s')"),
		*ShortName(ReboundKey), *Label),
		Label == ShortName(ReboundKey));

	// ⛔ ...and the stale reference key does not appear AT ALL. This is the half that fails on
	// an implementation which merely APPENDS the live key to the reference one.
	TestFalse(FString::Printf(TEXT("⛔ The stale reference key '%s' does not appear in the label"),
		*ShortName(ReferenceKey)),
		Label.Contains(ShortName(ReferenceKey)));

	// ⭐ AND THE FALLBACK STILL WORKS, so "prefers the live key" has not become "ignores the
	// registry": with nothing mapped, every slot derives its own reference key.
	for (int32 Slot = 0; Slot < Row->Actions.Num(); ++Slot)
	{
		if (!Row->QwertyReferenceKeys.IsValidIndex(Slot))
		{
			continue;
		}

		const FString Fallback = UCardHandWidget::ComposeSlotKeyLabel(Slot, /*LayoutSubsystem=*/nullptr, NoAppliedKeys);
		TestTrue(FString::Printf(TEXT("Slot %d with nothing mapped falls back to its own reference key '%s' (got '%s')"),
			Slot, *ShortName(Row->QwertyReferenceKeys[Slot]), *Fallback),
			Fallback == ShortName(Row->QwertyReferenceKeys[Slot]));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 4 — Siegebound.CardHand.SlotKeyLabelDegradesToEmptyNeverAPlaceholder
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ DEGRADE-OPEN, AND THE DISTINCTION IS SUBSTANTIVE, NOT STYLISTIC (`CARDBAR-§3`).
 *
 *  The shipped chip composer answers "(not bound)" or the pointer affordance when it has no
 *  key, and that is CORRECT FOR THE TAB SCREEN — `HELP-§2` mechanism 2 wants a visible gap,
 *  because a visible gap gets fixed. It is WRONG FOR A CARD CHIP: the card bar's degrade is to
 *  HIDE the chip, and prose glued under a card is a broken bar. ⇒ every fault path here must
 *  return EXACTLY empty.
 *
 *  ⭐ HOW THE PLACEHOLDERS ARE NAMED WITHOUT TYPING THEM: both strings live as file-local
 *  constants in SiegeControlsHelpWidget.cpp and are unreachable from here — so the test ASKS
 *  THE SAME COMPOSER what it says with nothing to say, and refuses that answer. Retype either
 *  constant and this test still holds; that is the point.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardHandSlotKeyLabelDegradesEmptyTest,
	"Siegebound.CardHand.SlotKeyLabelDegradesToEmptyNeverAPlaceholder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardHandSlotKeyLabelDegradesEmptyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardHandKeyLabelTestUtils;

	const FSiegeControlsHelpAction* const Row = PlayRow();
	if (!TestNotNull(TEXT("The shipped Cards.Play registry row exists"), Row))
	{
		return false;
	}

	// ── THE PLACEHOLDERS, DERIVED FROM THE SHIPPED COMPOSER ─────────────────────────────
	FSiegeControlsHelpAction MappedProbe;
	MappedProbe.Lane = ESiegeInputLane::MappedAction;
	const FString NotBoundAnswer =
		FSiegeControlsHelpRegistry::ComposeKeyChipLabel(MappedProbe, TArray<FKey>()).ToString();

	FSiegeControlsHelpAction PointerProbe;
	PointerProbe.Lane         = ESiegeInputLane::PointerOnly;
	PointerProbe.bPointerOnly = true;
	const FString PointerAnswer =
		FSiegeControlsHelpRegistry::ComposeKeyChipLabel(PointerProbe, TArray<FKey>()).ToString();

	// FIXTURE SELF-CHECK: if either placeholder were empty, every "is not the placeholder"
	// assertion below would be vacuously satisfied by the empty string it is checking for.
	TestFalse(TEXT("FIXTURE: the composer's no-key answer is a NON-empty placeholder"), NotBoundAnswer.IsEmpty());
	TestFalse(TEXT("FIXTURE: the composer's pointer answer is a NON-empty placeholder"), PointerAnswer.IsEmpty());

	// ── THE FAULT PATHS ─────────────────────────────────────────────────────────────────
	const TArray<int32> OutOfRange = { -1, -7, Row->Actions.Num(), Row->Actions.Num() + 1, MAX_int32, MIN_int32 };

	for (const int32 BadSlot : OutOfRange)
	{
		bool bProviderWasCalled = false;

		const FString Label = UCardHandWidget::ComposeSlotKeyLabel(BadSlot, /*LayoutSubsystem=*/nullptr,
			[&bProviderWasCalled](const FSiegeControlsHelpAction& /*SlotRow*/)
			{
				bProviderWasCalled = true;
				return TArray<FKey>{ EKeys::F };
			});

		TestTrue(FString::Printf(TEXT("Slot %d (out of range) returns EXACTLY empty (got '%s')"), BadSlot, *Label),
			Label.IsEmpty());

		// ⛔ Each of these can fail independently, and each names a real wrong answer.
		TestTrue(FString::Printf(TEXT("⛔ Slot %d does not return the '%s' placeholder"), BadSlot, *NotBoundAnswer),
			Label != NotBoundAnswer);
		TestTrue(FString::Printf(TEXT("⛔ Slot %d does not return the '%s' pointer chip"), BadSlot, *PointerAnswer),
			Label != PointerAnswer);

		// ⭐ AND THE RANGE CHECK PRECEDES THE LIVE QUERY. Asserted because the live provider
		// touches Enhanced Input and LoadSynchronous's the IA_* asset — work an out-of-range
		// slot must never trigger, and a reordering that broke it would be invisible otherwise
		// (the provider here even answers a valid key, so a late range check would still
		// return empty and look fine).
		TestFalse(FString::Printf(TEXT("⛔ Slot %d never reaches the live applied-key query"), BadSlot),
			bProviderWasCalled);
	}

	// ── AND THE VALID SLOTS ARE NOT DEGRADED ────────────────────────────────────────────
	// ⚠️ Without this, "always return empty" would pass every assertion above.
	for (int32 Slot = 0; Slot < Row->Actions.Num(); ++Slot)
	{
		const FString Label = UCardHandWidget::ComposeSlotKeyLabel(Slot, /*LayoutSubsystem=*/nullptr, NoAppliedKeys);

		TestFalse(FString::Printf(TEXT("Slot %d (in range) produces a REAL label, not an empty one"), Slot),
			Label.IsEmpty());
		TestTrue(FString::Printf(TEXT("⛔ Slot %d's real label is not the '%s' placeholder either"), Slot, *NotBoundAnswer),
			Label != NotBoundAnswer);
		TestTrue(FString::Printf(TEXT("⛔ Slot %d's real label is not the pointer chip"), Slot),
			Label != PointerAnswer);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 5 — Siegebound.CardHand.SlotKeyLabelWarnsOncePerSlotNotPerCall
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⚠️ THE SPAM GUARD, AND IT IS A REAL BUG CLASS ON THIS WIDGET, NOT TIDINESS.
 *
 *  WBP_CardHand calls GetSlotKeyLabel from its OnHandSlotUpdated handler, and
 *  UCardHandWidget re-pushes EVERY slot on EVERY gold change (RefreshAllHandSlots, bound to
 *  ASiegePlayerState::OnGoldChanged). Gold ticks continuously in a match ⇒ an unguarded
 *  warning is thousands of lines a minute, which is how a real diagnostic becomes unfindable.
 *
 *  ⭐ MEASURED, NOT ARGUED: the framework is told to expect the message EXACTLY twice across
 *  ten calls spanning two distinct bad slots. A per-call log makes it ten and the test fails;
 *  a removed log makes it zero and the test fails.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardHandSlotKeyLabelWarnsOnceTest,
	"Siegebound.CardHand.SlotKeyLabelWarnsOncePerSlotNotPerCall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardHandSlotKeyLabelWarnsOnceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardHandKeyLabelTestUtils;

	const FSiegeControlsHelpAction* const Row = PlayRow();
	if (!TestNotNull(TEXT("The shipped Cards.Play registry row exists"), Row))
	{
		return false;
	}

	// REQUIRED, EXACTLY TWICE: one warning per DISTINCT bad slot, however many times each is
	// asked. Anything else - silence, or a warning per call - is a failure.
	AddExpectedMessagePlain(TEXT("GetSlotKeyLabel: slot"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 2);

	TStrongObjectPtr<UCardHandWidget> Hand(NewObject<UCardHandWidget>(GetTransientPackageAsObject()));
	if (!TestTrue(TEXT("A card-hand widget object was created"), Hand.IsValid()))
	{
		return false;
	}

	// Two distinct out-of-range slots, asked five times each. ⛔ The widget has no world, no
	// controller and no layout subsystem here - which is exactly `KBD-§5`'s fail-safe state and
	// must not crash.
	const TArray<int32> BadSlots = { -1, Row->Actions.Num() + 3 };

	for (int32 Repeat = 0; Repeat < 5; ++Repeat)
	{
		for (const int32 BadSlot : BadSlots)
		{
			const FString Label = Hand->GetSlotKeyLabel(BadSlot);
			TestTrue(FString::Printf(TEXT("GetSlotKeyLabel(%d) call %d returns empty"), BadSlot, Repeat),
				Label.IsEmpty());
		}
	}

	// ⭐ AND THE VALID SLOTS STAY SILENT. Included in the SAME test deliberately: the expected
	// count of 2 above is what proves these six calls logged NOTHING, and a spam guard that
	// warned on a healthy slot would push the count to eight.
	for (int32 Slot = 0; Slot < Row->Actions.Num(); ++Slot)
	{
		const FString Label = Hand->GetSlotKeyLabel(Slot);

		// With no controller the live query answers empty, so this is the single-translation
		// fallback over the registry's own reference key. ⛔ Derived, never typed.
		const FString Expected = Row->QwertyReferenceKeys.IsValidIndex(Slot)
			? ShortName(Row->QwertyReferenceKeys[Slot])
			: FString();

		TestTrue(FString::Printf(TEXT("The LIVE GetSlotKeyLabel(%d) composes through the same seam ('%s' vs '%s')"),
			Slot, *Label, *Expected),
			Label == Expected);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
