// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Siegebound/CardRow.h" // FCardRow + ESpellEffect + ESpellDelivery — the three types every claim here is about
#include "UObject/Class.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ THE DECK-BUILDER GLOSSARY GATE (TASK-999; law `SC-§37`, ⭐ `SC-§65`, `SC-§77`) ═══
 *
 *  ⛔⛔ WHY THIS FILE EXISTS, AND IT IS THE POINT OF THE ROW RATHER THAN A FOOTNOTE.
 *  `UDeckBuilderWidget`'s glossary composed the `Fog` card with NO effect arm at all and
 *  THEN printed, verbatim:
 *
 *      "Aimed at a spot on the ground: it goes off where you place the reticle."
 *
 *  for a spell that has NO reticle and covers the ENTIRE battlefield. Both halves were
 *  invisible to every gate this project owns: the effect `switch` carried a `default:` so
 *  a missing arm COMPILED, and the aiming guard was `if (SpellEffect != GoldSteal)` — a
 *  BLACKLIST, correct only until the next value, which by then had already failed TWICE,
 *  once per new no-reticle spell.
 *
 *  ⚖️ A BLANK LINE IS A GAP; A WRONG LINE IS A LIE — and this one lived in the surface
 *  Jonathan actually reads while building a deck.
 *
 *  ── ⭐ WHAT THIS FILE ASSERTS, AND WHY IT IS NOT A LIST OF THE TWO CARDS THAT BROKE ──
 *  A per-value list rots exactly like the blacklist it replaces. Every claim below is
 *  therefore UNIVERSALLY QUANTIFIED over `StaticEnum<ESpellEffect>()`, so the NEXT spell
 *  effect is covered by a test written before it existed:
 *
 *    TEST 1  every declared value composes at least one EFFECT clause (non-blank).
 *    TEST 2  no value ever claims a RETICLE on a row with no aim point (the lie, killed
 *            for every value at once instead of two values at a time).
 *    TEST 3  the shipped roster's five spell SHAPES, named by CardID.
 *    TEST 4  the one documented path by which a reticle claim can still be authored —
 *            an explicit `SpellDelivery` cell — pinned so it reads as design, not drift.
 *
 *  ── ⛔ HOW IT REACHES THE COMPOSER ──────────────────────────────────────────────────
 *  `UDeckBuilderWidget::AppendRuleLines` is `private:`, and its header belongs to another
 *  row this wave. TASK-999 therefore extracted the spell block into
 *  `SiegeboundCardGlossary::AppendSpellLines` — a free function with EXTERNAL LINKAGE in
 *  DeckBuilderWidget.cpp — and this file forward-declares it below. ⛔ That declaration is
 *  the gate's whole coupling: if the signature moves, or the definition is "tidied" into
 *  an anonymous namespace, this file fails to LINK. ⭐ That is the intended failure mode
 *  and must be repaired at the definition, ⛔ never by deleting the declaration.
 *
 *  ── ⚠️ THE PROBE STRINGS, AND THE RED-PROOF THEY REQUIRE (`SC-§37`) ─────────────────
 *  The two delivery sentences are `const TCHAR[]` with INTERNAL linkage in
 *  DeckBuilderWidget.cpp, so they cannot be referenced — the claims are made against a
 *  distinctive SUBSTRING of each instead. ⛔ A substring probe fails SAFE on its own: if
 *  the sentence is reworded, a "the reticle line is ABSENT" assertion passes for the
 *  wrong reason and the gate quietly dies. ⇒ EVERY negative assertion here is paired with
 *  a POSITIVE CONTROL in the SAME test that requires the same probe to be PRESENT. Reword
 *  either sentence and the control goes RED first, which is the author's instruction to
 *  re-derive the probe. ⛔ Never delete a control to make a suite green.
 *
 *  ── ⛔ NO COORDINATES (`SC-§77`) ────────────────────────────────────────────────────
 *  No line number, no `cards.csv` field index and no row count appears in any executable
 *  line. Card SHAPES are named by CardID in prose and rebuilt from their columns here;
 *  the magnitudes below are FIXTURE values chosen to exercise the branches, ⛔ NOT
 *  transcriptions of the shipped data (a fixture that mirrors the CSV becomes a second,
 *  silently-diverging copy of it).
 */

//
// ⛔ THE ONE COUPLING TO THE SHIPPING COMPOSER. Declared, never defined here.
//
namespace SiegeboundCardGlossary
{
	void AppendSpellLines(const FCardRow& Row, TArray<FString>& OutLines);
}

namespace
{
	/** The reticle claim — the exact sentence that was false for `Fog` and `BrightSun`. */
	const TCHAR* const ReticleProbe = TEXT("where you place the reticle");

	/** The hero-line claim, the other half of the aiming pair. */
	const TCHAR* const HeroLineProbe = TEXT("Aimed from your hero");

	/**
	 *  A spell row carrying EVERY magnitude any effect arm can ask for, so a value that
	 *  composes nothing composes nothing for its OWN reason rather than because the
	 *  fixture starved it. ⛔ `SpellDelivery` stays `Auto` — the sparse state of every
	 *  shipped row but two — because `Auto` is the state the defect lived in.
	 */
	FCardRow MakeWellFormedSpellRow(ESpellEffect Effect)
	{
		FCardRow Row;
		Row.CardType = ECardType::Spell;
		Row.SpellEffect = Effect;
		Row.SpellDelivery = ESpellDelivery::Auto;
		Row.Damage = 100.f;
		Row.AoERadius = 300.f;
		Row.EffectDuration = 5.f;
		Row.MaxTargets = 3;
		Row.GoldSteal = 10;
		return Row;
	}

	/** True when ANY composed line carries Probe. */
	bool LinesContain(const TArray<FString>& Lines, const TCHAR* Probe)
	{
		for (const FString& Line : Lines)
		{
			if (Line.Contains(Probe, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 *  The count of composed lines that are NOT one of the two aiming sentences — i.e.
	 *  the EFFECT clauses.
	 *
	 *  ⛔⛔ THIS FUNCTION IS THE REASON TEST 1 IS NOT VACUOUS, AND THE TRAP IT AVOIDS IS
	 *  THE EXACT ONE THAT LET THE DEFECT SHIP: a row with a missing effect arm still
	 *  produced a non-empty description, because the DELIVERY line was appended anyway.
	 *  A test asserting "the composer produced something" would have passed on the very
	 *  card that was broken. ⇒ the aiming lines are subtracted before anything is counted.
	 */
	int32 CountEffectClauses(const TArray<FString>& Lines)
	{
		int32 Count = 0;
		for (const FString& Line : Lines)
		{
			if (Line.IsEmpty())
			{
				continue;
			}
			if (Line.Contains(ReticleProbe, ESearchCase::CaseSensitive)
				|| Line.Contains(HeroLineProbe, ESearchCase::CaseSensitive))
			{
				continue;
			}
			++Count;
		}
		return Count;
	}

	/**
	 *  Every DECLARED `ESpellEffect` value, `None` excluded (it is not a spell) and UHT's
	 *  hidden `_MAX` sentinel skipped BY NAME.
	 *
	 *  ⚠️ The sentinel is identified by name rather than by position or by
	 *  `HasMetaData("Hidden")`: the position is a UHT detail, and that accessor is
	 *  `WITH_EDITOR`-only while this file compiles wherever WITH_DEV_AUTOMATION_TESTS is
	 *  on. (Same idiom as Tests/SiegeFogVolumeTest.cpp, deliberately — one shape for one
	 *  problem.)
	 */
	TArray<ESpellEffect> GatherDeclaredSpellEffects(const UEnum* EffectEnum)
	{
		TArray<ESpellEffect> Values;
		if (!EffectEnum)
		{
			return Values;
		}

		for (int32 Index = 0; Index < EffectEnum->NumEnums(); ++Index)
		{
			if (EffectEnum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"), ESearchCase::CaseSensitive))
			{
				continue;
			}

			const int64 Value = EffectEnum->GetValueByIndex(Index);
			if (Value == static_cast<int64>(ESpellEffect::None))
			{
				continue;
			}

			Values.AddUnique(static_cast<ESpellEffect>(Value));
		}
		return Values;
	}

	/**
	 *  The floor every sweep self-checks against, so a null reflection object or an empty
	 *  sweep can never make a universally-quantified claim pass VACUOUSLY.
	 *  ⛔ It is a FLOOR (`>=`), not an equality: appending a spell effect must not turn
	 *  this file red for the wrong reason — it must turn it red at the CLAIM.
	 *  ⭐ Today's declared set is None + 7 spell effects; the seven are what is swept.
	 */
	constexpr int32 MinimumDeclaredSpellEffects = 7;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — EVERY `ESpellEffect` VALUE COMPOSES AN EFFECT CLAUSE.
//  ⛔ The assertion TASK-999 item (3) asks for: the missing-arm defect, closed for
//  every value that exists AND every value that does not exist yet.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardGlossaryEveryEffectComposesTextTest,
	"Siegebound.CardGlossary.EverySpellEffectValueComposesANonEmptyEffectClause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardGlossaryEveryEffectComposesTextTest::RunTest(const FString& Parameters)
{
	const UEnum* const EffectEnum = StaticEnum<ESpellEffect>();
	if (!EffectEnum)
	{
		AddError(TEXT("SELF-CHECK FAILED: StaticEnum<ESpellEffect>() returned null — every claim in this file would be meaningless."));
		return false;
	}

	const TArray<ESpellEffect> DeclaredEffects = GatherDeclaredSpellEffects(EffectEnum);

	TestTrue(
		TEXT("SELF-CHECK: the sweep found the declared ESpellEffect values (a zero here would make the claim below ")
		TEXT("vacuous — the whole file would pass by describing nothing)."),
		DeclaredEffects.Num() >= MinimumDeclaredSpellEffects);

	for (const ESpellEffect Effect : DeclaredEffects)
	{
		const FCardRow Row = MakeWellFormedSpellRow(Effect);

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestTrue(
			*FString::Printf(
				TEXT("⛔ ESpellEffect::%s composes NO effect clause — the card renders in the deck builder with a BLANK ")
				TEXT("effect line, which is exactly how `Fog` shipped. ⛔ Add its arm to ")
				TEXT("SiegeboundCardGlossary::AppendSpellLines; ⛔ do NOT relax this test. ⚠️ The delivery sentences are ")
				TEXT("subtracted before counting, so a row that only ever printed its AIMING line still fails here."),
				*EffectEnum->GetNameStringByValue(static_cast<int64>(Effect))),
			CountEffectClauses(Lines) >= 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐⭐ — NO VALUE EVER CLAIMS A RETICLE IT DOES NOT HAVE.
//  ⛔ The LIE, killed universally. This is the assertion the `!= GoldSteal`
//  blacklist could never make: it is quantified over the ENUM, so the next
//  no-reticle spell is defended by a test written before it was designed.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardGlossaryNoUnaimedReticleClaimTest,
	"Siegebound.CardGlossary.NoSpellEffectClaimsAReticleOnARowWithNoAimPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardGlossaryNoUnaimedReticleClaimTest::RunTest(const FString& Parameters)
{
	const UEnum* const EffectEnum = StaticEnum<ESpellEffect>();
	if (!EffectEnum)
	{
		AddError(TEXT("SELF-CHECK FAILED: StaticEnum<ESpellEffect>() returned null — every claim below would be meaningless."));
		return false;
	}

	// ⛔⛔ THE POSITIVE CONTROL COMES FIRST, DELIBERATELY. Every claim in this test is an
	// ABSENCE, and an absence assertion against a substring passes for free the moment the
	// sentence is reworded. This control requires the SAME probe to be PRESENT on a row
	// that genuinely is reticle-placed (the `Lightning` shape: a real AoERadius, no
	// authored delivery cell), so a reworded sentence turns this row RED before the
	// absences can go quietly green.
	{
		FCardRow AimedRow = MakeWellFormedSpellRow(ESpellEffect::TopTargetsDamage);
		AimedRow.AoERadius = 700.f;

		TArray<FString> AimedLines;
		SiegeboundCardGlossary::AppendSpellLines(AimedRow, AimedLines);

		TestTrue(
			TEXT("RED-PROOF CONTROL: a ground-placed spell WITH an aim point still prints the reticle sentence. ")
			TEXT("⛔ If this fails, the sentence was reworded and every ABSENCE claim below has silently stopped ")
			TEXT("testing anything — re-derive ReticleProbe from the shipped string, ⛔ never delete this row."),
			LinesContain(AimedLines, ReticleProbe));
	}

	const TArray<ESpellEffect> DeclaredEffects = GatherDeclaredSpellEffects(EffectEnum);

	TestTrue(
		TEXT("SELF-CHECK: the sweep found the declared ESpellEffect values (a zero here would make the claim below vacuous)."),
		DeclaredEffects.Num() >= MinimumDeclaredSpellEffects);

	for (const ESpellEffect Effect : DeclaredEffects)
	{
		// ⛔ NO AIM EVIDENCE OF ANY KIND: no radius to place a circle in, and the sparse
		// `Auto` delivery every shipped row but two carries. This is the `Pickpocket` /
		// `Fog` / `BrightSun` shape, applied to EVERY value.
		FCardRow Row = MakeWellFormedSpellRow(Effect);
		Row.AoERadius = 0.f;

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestFalse(
			*FString::Printf(
				TEXT("⛔⛔ ESpellEffect::%s tells the player to place a RETICLE on a row that carries no aim point at all. ")
				TEXT("⛔ That is the `Fog` lie. ⛔ The repair is NEVER to add this value to an exclusion list — a blacklist ")
				TEXT("is a defect generator and this one had already failed twice. ⛔ Derive the aiming claim from the ")
				TEXT("delivery cell and the row's own aim evidence (SiegeboundCardGlossary::AppendSpellLines)."),
				*EffectEnum->GetNameStringByValue(static_cast<int64>(Effect))),
			LinesContain(Lines, ReticleProbe));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐ — THE SHIPPED ROSTER'S SPELL SHAPES, NAMED BY CardID.
//  ⛔ Shapes, not transcriptions: each row is rebuilt from the COLUMNS that decide
//  the branch, never from a copy of the CSV's numbers (`SC-§77`).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardGlossaryRosterShapesTest,
	"Siegebound.CardGlossary.TheShippedSpellShapesEachDescribeTheirOwnAiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardGlossaryRosterShapesTest::RunTest(const FString& Parameters)
{
	// ── the `Fireball` shape: an area effect with an AUTHORED HeroLine cell ──────────
	{
		FCardRow Row = MakeWellFormedSpellRow(ESpellEffect::AoEDamage);
		Row.SpellDelivery = ESpellDelivery::HeroLine;

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestTrue(
			TEXT("The `Fireball` shape (AoEDamage + an authored HeroLine cell) describes the HERO-LINE aiming. ")
			TEXT("⛔ A failure here is also the RED-PROOF for HeroLineProbe used by the absence claims below."),
			LinesContain(Lines, HeroLineProbe));

		TestFalse(
			TEXT("The `Fireball` shape must NOT also claim a ground reticle — one spell, one aiming sentence."),
			LinesContain(Lines, ReticleProbe));
	}

	// ── the `Pickpocket` shape: the ONE case the old blacklist got right, which must
	//    now hold BY CONSTRUCTION rather than by being named ───────────────────────────
	{
		FCardRow Row = MakeWellFormedSpellRow(ESpellEffect::GoldSteal);
		Row.AoERadius = 0.f;

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestFalse(
			TEXT("⛔ REGRESSION GUARD: the `Pickpocket` shape claims no reticle. It was the blacklist's single correct ")
			TEXT("entry, and deleting the blacklist must not have cost it — it is now carried by the DERIVATION, with ")
			TEXT("this card's name appearing nowhere in the composer."),
			LinesContain(Lines, ReticleProbe));

		TestFalse(
			TEXT("⛔ The `Pickpocket` shape claims no hero line either — it resolves instantly, with no aim of any kind."),
			LinesContain(Lines, HeroLineProbe));

		TestTrue(
			TEXT("SELF-CHECK: the `Pickpocket` shape still DESCRIBES itself. ⛔ Two absences and no presence would ")
			TEXT("also be satisfied by a composer that had simply stopped composing."),
			CountEffectClauses(Lines) >= 1);
	}

	// ── the `Fog` shape: FogCover, no radius, a duration cell ────────────────────────
	{
		FCardRow Row = MakeWellFormedSpellRow(ESpellEffect::FogCover);
		Row.AoERadius = 0.f;
		Row.EffectDuration = 42.f; // ⛔ a FIXTURE, chosen so it can only come from this row

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestFalse(
			TEXT("⛔⛔ THE DEFECT ITSELF: the `Fog` shape must NOT tell the player to place a reticle. Jonathan read ")
			TEXT("*\"Aimed at a spot on the ground: it goes off where you place the reticle\"* for a no-reticle, ")
			TEXT("map-wide fog (FOG-§10.1)."),
			LinesContain(Lines, ReticleProbe));

		TestFalse(
			TEXT("The `Fog` shape claims no hero line either."),
			LinesContain(Lines, HeroLineProbe));

		TestTrue(
			TEXT("⛔ THE OTHER HALF: the `Fog` shape composes an EFFECT clause. This is the blank line the row was ")
			TEXT("originally boarded for — the card rendered describing nothing at all."),
			CountEffectClauses(Lines) >= 1);

		TestTrue(
			TEXT("The `Fog` shape reads its duration from ITS OWN ROW (GDD §3.0: never a hardcoded magnitude). ")
			TEXT("⚠️ The MECHANISM reads AFogVolume::FogDurationSeconds today and the cell is inert until TASK-1016 ")
			TEXT("wires it up; FogVolume.h requires the two to agree, so the row is the value that stays correct."),
			LinesContain(Lines, TEXT("42")));
	}

	// ── the `BrightSun` shape: FogClear, no radius. ⛔ Correct BEFORE its card row
	//    exists — the derivation needed no data to be right about it ────────────────────
	{
		FCardRow Row = MakeWellFormedSpellRow(ESpellEffect::FogClear);
		Row.AoERadius = 0.f;
		Row.EffectDuration = 37.f; // ⛔ a FIXTURE, not FOG-§10.1's pinned base

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestFalse(
			TEXT("⛔ The `BrightSun` shape must NOT claim a reticle — the identical defect `Fog` carried (FOG-§10.1)."),
			LinesContain(Lines, ReticleProbe));

		TestFalse(
			TEXT("The `BrightSun` shape claims no hero line either."),
			LinesContain(Lines, HeroLineProbe));

		TestTrue(
			TEXT("The `BrightSun` shape composes its effect clauses — the clear, the prevention window's rules, and ")
			TEXT("the base window from the row. ⛔ The window RULES are unconditional: a blank EffectDuration cell must ")
			TEXT("cost a sentence, never the description."),
			CountEffectClauses(Lines) >= 2);

		TestTrue(
			TEXT("The `BrightSun` shape reads its base window from ITS OWN ROW."),
			LinesContain(Lines, TEXT("37")));
	}

	// ── the same `BrightSun` shape with NO duration cell at all — the state its card
	//    row is in today, since TASK-983 has not written it ────────────────────────────
	{
		FCardRow Row = MakeWellFormedSpellRow(ESpellEffect::FogClear);
		Row.AoERadius = 0.f;
		Row.EffectDuration = 0.f;

		TArray<FString> Lines;
		SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

		TestTrue(
			TEXT("⛔ A fog effect with EVERY row magnitude blank STILL describes itself. ⚖️ That is the difference ")
			TEXT("between this arm and the six above it: they all gate on a magnitude and can compose nothing, which ")
			TEXT("is precisely the blank-line failure this row exists to delete."),
			CountEffectClauses(Lines) >= 2);

		TestFalse(
			TEXT("...and it still claims no reticle with every magnitude blank."),
			LinesContain(Lines, ReticleProbe));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐ — THE AUTHORED CELL WINS, AND THAT IS DESIGN RATHER THAN DRIFT.
//  ⛔ Pinned so the ONE remaining path to a wrong aiming claim is a DELIBERATE,
//  reviewable data decision instead of an oversight somebody rediscovers.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardGlossaryAuthoredDeliveryWinsTest,
	"Siegebound.CardGlossary.AnAuthoredSpellDeliveryCellDecidesTheAimingSentence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardGlossaryAuthoredDeliveryWinsTest::RunTest(const FString& Parameters)
{
	// ⛔⛔ READ THIS BEFORE "FIXING" IT. The row below is a FogCover row whose author has
	// explicitly pinned `GroundCircle` in its SpellDelivery cell, and the glossary agrees
	// with it. ⛔ NO SHIPPED ROW DOES THIS — every row but `Fireball` and `FrostNova`
	// leaves the cell blank, which deserialises to `Auto`.
	//
	// ⚖️ WHY IT IS CORRECT: `SpellDelivery` exists as the per-card DATA OVERRIDE LEVER, and
	// USpellLibrary::GetEffectiveDelivery branches on the CELL FIRST for exactly that
	// reason. A glossary that quietly overruled an authored cell would be describing a
	// delivery the data does not ask for — the same class of falsehood, pointed the other
	// way. ⇒ a wrong cell is a DATA defect, caught by a data gate; the composer's job is
	// to agree with the column.
	FCardRow Row;
	Row.CardType = ECardType::Spell;
	Row.SpellEffect = ESpellEffect::FogCover;
	Row.SpellDelivery = ESpellDelivery::GroundCircle;
	Row.AoERadius = 0.f;

	TArray<FString> Lines;
	SiegeboundCardGlossary::AppendSpellLines(Row, Lines);

	TestTrue(
		TEXT("An AUTHORED SpellDelivery cell decides the aiming sentence even with no radius — the cell wins first, ")
		TEXT("in the same precedence order USpellLibrary::GetEffectiveDelivery itself uses. ⛔ If this ever needs to ")
		TEXT("change, it changes in the DERIVATION and in this row together — never by re-adding an effect blacklist."),
		LinesContain(Lines, ReticleProbe));

	// ⛔ And the control that keeps the row above honest: the SAME effect, with the cell
	// left in its shipped `Auto` state, claims nothing. Without this pair, a composer that
	// had simply gone back to printing the reticle sentence unconditionally would satisfy
	// the assertion above.
	FCardRow SparseRow = Row;
	SparseRow.SpellDelivery = ESpellDelivery::Auto;

	TArray<FString> SparseLines;
	SiegeboundCardGlossary::AppendSpellLines(SparseRow, SparseLines);

	TestFalse(
		TEXT("PAIRED CONTROL: the same effect with the cell left blank (`Auto`, the state of every shipped row but ")
		TEXT("two) claims NO reticle. ⛔ The two rows are only meaningful together — one shows the cell is read, the ")
		TEXT("other shows it is not being ignored in favour of printing the sentence regardless."),
		LinesContain(SparseLines, ReticleProbe));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
