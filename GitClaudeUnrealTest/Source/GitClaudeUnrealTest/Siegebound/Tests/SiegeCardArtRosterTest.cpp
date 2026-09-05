// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Siegebound/CardHandWidget.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/SiegePlayerController.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ THE CARD-ART ROSTER GATE (TASK-957; law: CONVENTIONS `SC-§51` cl. 6,
 *      `SC-§50` cl. 2 + cl. 4, `SC-§54` cl. 3, `SHIP-§9` + `SHIP-§9c` cl. 2,
 *      `SC-§39`, `SC-§49`, `SC-§38`, `SC-§40` cl. 10, `SC-§53` cl. 3) ═══
 *
 *  ⛔⛔ WHY THIS FILE EXISTS: IT IS THE WITCH DEFECT'S EXACT SHAPE ON A SECOND
 *  SURFACE. A 50-gold card shipped whose actor Blueprint did not exist; the play
 *  was refused, no gold was deducted, and nothing errored — the code was CORRECT
 *  and the GRACEFUL DEGRADE was what hid it (`SC-§50` cl. 2). The sibling gate
 *  `Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath`
 *  closes the SPAWN half. This file closes the CARD-FACE half, and the degrade
 *  there is quieter still: `UCardHandWidget::ResolveCardArtTexture` returns
 *  nullptr on THREE separate misses — no row, an unset `CardArt` cell, and an
 *  unresolvable path — each with a once-per-CardID warning and NEVER a crash.
 *  ⇒ A CARD WITH BROKEN ART DOES NOT ERROR. IT QUIETLY RENDERS AS WORDS, and it
 *  will keep doing so for as long as nobody happens to look at that one card.
 *
 *  ── ⭐⭐ THE SCOPE SENTENCE, QUOTED VERBATIM FROM `TASK-957` ITEM (1) ────────
 *     (`SC-§49` cl. 4(a): quoted, NOT restated in tidier words. The board's own
 *      emphasis marks are reproduced; not one word is changed, added or reordered.)
 *
 *      "⛔ `SHIP-§9c` cl. 2 says ⛔ PREFER THE PREDICATE THE ⛔ CONSUMER ITSELF
 *       USES — ⛔ and here the consumer's ⛔ OWN predicate ⛔ IS
 *       `Row->CardArt.LoadSynchronous()`. ⇒ ⛔⛔ THERE IS ⛔ NO COMPOSER TO DRIFT
 *       (⛔ the path is a ⛔ DATA COLUMN in `DT_Cards`, ⛔ not a string this code
 *       builds) ⇒ ⛔ this row carries ⛔ NONE of `TASK-947`'s declared
 *       composer-drift residual."
 *
 *  ⭐ THAT IS THE WHOLE POINT AND IT IS WHY THIS GATE IS STRUCTURALLY STRONGER
 *  THAN THE ONE THAT BOUGHT IT. The sibling has to BUILD a path
 *  (`/Game/Blueprints/Units/BP_Unit_<CardID>`) and hope it still matches the four
 *  other hand-composed copies in the tree, so it declares a composer-drift
 *  residual it cannot detect. THIS FILE COMPOSES NOTHING. It reads the same
 *  `TSoftObjectPtr<UTexture2D>` cell the shipped widget reads, off the same table,
 *  and calls the same two member functions on it. ⛔ THERE IS NO `/Game/...` STRING
 *  LITERAL IN ANY EXECUTABLE LINE OF THIS FILE.
 *
 *  ⚠️ STATED AS AN INVARIANT RATHER THAN AS A HIT COUNT, BECAUSE A READER WILL
 *  GREP IT: a bare search for the mount point returns a NON-ZERO number here and
 *  always will — this header discusses paths in prose, and that count drifts every
 *  time the prose is edited (`SC-§41`: pin the SHAPE, not a bare token; `TL-§5b`:
 *  an absolute cannot survive contact).
 *
 *      ⛔ THE INVARIANT, IN WORDS: no line of this file may contain the four
 *         characters of the string macro, an open parenthesis and a double quote,
 *         immediately followed by the game mount point. That combination is what a
 *         hand-written asset-path literal looks like, and there must be none.
 *
 *      ⚠️⭐ IT IS SPELLED OUT IN WORDS ON PURPOSE, AND THAT IS A LESSON RATHER
 *         THAN A STYLE CHOICE: the first draft of this paragraph wrote the pattern
 *         out as a runnable one-liner, and THE PARAGRAPH THEN MATCHED ITSELF —
 *         the check reported 1 and the only hit was the sentence asserting it was
 *         0. ⛔ A GREP GATE WHOSE PATTERN APPEARS IN ITS OWN DOCUMENTATION CAN
 *         NEVER READ CLEAN, and a reader who "fixes" the count instead of the
 *         self-reference will conclude this file is dirty when it is not.
 *
 *      ✅ MEASURED 2026-09-03 at `f050caf`, with the pattern typed at a shell
 *         prompt rather than stored in this file: ZERO. Every occurrence of the
 *         mount point in this file resolves to a `*` or `//` comment line —
 *         verified line by line, not by subtraction.
 *
 *  ⛔ This file therefore adds NO sixth copy of anything for `TASK-959`'s
 *  path-composer extraction to have to find, and an edit that trips the invariant
 *  above has broken this file's premise rather than merely changed its style.
 *
 *  ── ⛔ THE PREDICATE, QUOTED VERBATIM FROM `TASK-957` ITEM (2) ──────────────
 *
 *      "⛔ THE TEST: ⛔ WALK ⛔ EVERY `DT_Cards` ROW (⛔ ALL card types — ⛔ card
 *       art is ⛔ NOT spawnable-only) AND ⛔ ASSERT `Row->CardArt` ⛔ RESOLVES TO
 *       A ⛔ LOADABLE `UTexture2D`. ⛔ Use the consumer's predicate, ⛔ not a
 *       re-implementation. ⛔ An ⛔ UNSET cell and an ⛔ UNRESOLVABLE path are
 *       ⛔ TWO different findings — ⛔ report them ⛔ separately and ⛔ assert
 *       ⛔ both counts"
 *
 *  ⛔ THE DENOMINATOR IS **EVERY ROW**, NOT THE SPAWNABLE SUBSET, and it is
 *  derived rather than declared: `CardTable->GetRowNames()` over the table the
 *  shipped CDOs point at. Card art is a property of a Spell and a HeroUpgrade
 *  exactly as much as of a Unit — the sibling gate's 22-row spawnable filter would
 *  have left ten cards' faces ungated. ⛔ There is NO type filter in the walk
 *  below, deliberately, and that is a difference from the sibling worth keeping.
 *
 *  ── ⛔⛔ WHAT THIS GATE DOES **NOT** COVER — READ BEFORE TRUSTING A GREEN BAR ─
 *     (`SC-§49`: understating is the safe direction; a gate published at a wider
 *      scope than it measures is the defect that law exists to catch.)
 *
 *  ⛔ (R1) IT DOES NOT CALL THE CONSUMER **FUNCTION**. `UCardHandWidget::
 *      ResolveCardArtTexture` is `private` and instance-scoped, so a test cannot
 *      invoke it without standing up a widget. What this file evaluates is the
 *      two EXPRESSIONS that function is built out of — `Row->CardArt.IsNull()`
 *      and `Row->CardArt.LoadSynchronous()` — on the SAME soft pointer, read out
 *      of the SAME table. ⇒ the function's THIRD degrade branch (`!Row` — the
 *      lookup itself failing) is not exercised by the walk; it is reported per
 *      row as a hard error instead, which is the stronger treatment.
 *  ⛔ (R2) IT DOES NOT ASSERT THE FACE **LOOKS RIGHT**. A texture that loads but
 *      is blank, wrong-sized or the wrong card's art passes here. That bar is the
 *      art lane's and it has its own law (CONVENTIONS "an acceptance bar for a
 *      card face is derived from the shipped corpus").
 *  ⛔ (R3) IT READS THE **C++ CDO**. A Blueprint subclass of `UCardHandWidget`
 *      that OVERRODE `CardTableAsset` would be walked at its C++ default rather
 *      than its shipped value. ⚠️ MEASURED 2026-09-03 at `f050caf`, and it is
 *      inert BY COINCIDENCE OF CONTENT rather than structurally impossible
 *      (`SC-§37` cl. 2): `Content/UI/WBP_CardHand.uasset` contains ZERO
 *      occurrences of the string `CardTableAsset` — i.e. it carries no override —
 *      on an instrument that returns 1 for `CardHandWidget.h` (`SC-§39`).
 *
 *  ── ⭐⭐ EVERYTHING IS DERIVED. NOTHING IS TRANSCRIBED. ──────────────────────
 *
 *  ⛔ `TestEqual(Rows, 32)` IS ABSENT ON PURPOSE, and `qa/TASK-948.md` §2 already
 *  ruled the general case: a transcribed count "rebuilds the `TASK-874` trap
 *  inside the gate written to close it". Card #33 would turn this file red while
 *  the roster is perfectly healthy, and the obvious repair is to bump the number —
 *  by the person least likely to re-read the 33 rows. Every assertion below pins
 *  a RELATIONSHIP (`SC-§40` cl. 10).
 *
 *   (a) THE ROSTER comes from the DataTable the CONSUMER'S OWN CDO points at —
 *       `UCardHandWidget::CardTableAsset`, read by reflection because the property
 *       is `protected`. ⭐ This file walks the WIDGET's table rather than the
 *       controller's because the widget is the card-art consumer; the controller's
 *       is ALSO read and the two are ASSERTED EQUAL, which pins a real and
 *       otherwise-unwatched drift surface — the path literal `/Game/Data/DT_Cards`
 *       is written TWICE in shipped C++ (`CardHandWidget.cpp`'s constructor and
 *       `SiegePlayerController.cpp`'s), and nothing else in the tree notices if
 *       one of them moves.
 *   (b) THE ROWS come from `GetRowNames()` on the loaded asset, so a row that
 *       exists in `DT_Cards.uasset` but NOT in `Docs/Data/cards.csv` is still
 *       walked. This gate reads what the GAME loads, never the spreadsheet.
 *   (c) THE CARD-ART PATH is never composed. It is the authored value of the
 *       `CardArt` column, which `CardRow.h` types `TSoftObjectPtr<UTexture2D>`.
 *   (d) THE NEGATIVE CONTROL'S ABSENT PATH is DERIVED FROM THE POSITIVE CONTROL'S
 *       AUTHORED PATH by appending a measured-absent suffix — so it is guaranteed
 *       well-formed and in the real card-art package directory WITHOUT this file
 *       ever naming that directory. If the art folder moves, the control moves
 *       with it.
 *
 *  ⚠️ Both reflection lookups check the POINTED-TO TYPE, not merely the field
 *  name, and a miss is a hard error return rather than a skip. A renamed or
 *  retyped property that merely made this file walk zero rows would otherwise
 *  produce THE SAME GREEN BAR as a healthy roster.
 *
 *  ⚠️ `CardTypeSymbol` and `FindSoftObjectField` below are DELIBERATE DUPLICATES
 *  of helpers in `SiegeCardRosterTest.cpp`. Extracting a shared header would put
 *  this row inside a file `TASK-959` is rewriting; the same ruling was made
 *  explicitly for `TASK-962` (its item (7), and `TASK-963` item (7) is instructed
 *  NOT to flag it). The namespaces are what keep the two sets apart in a unity
 *  build — ⛔ do not flatten them.
 *
 *  ── ⭐⭐ THE LEDGERS, KEPT APART (`SC-§51` cl. 4 — never one number) ─────────
 *
 *  ⛔ THE TEST APPLIED BELOW IS `SC-§51` cl. 2's STRICT ONE, NOT A HEADCOUNT OF
 *  ASSERTIONS: for each candidate, IS THERE A WORLD IN WHICH IT ALONE FIRES? If
 *  `X` can never be false while `Y` is true, `X` and `Y` are ONE tell. ⚠️ Applying
 *  that honestly gives SMALLER numbers than this file's assertion count, and the
 *  smaller numbers are the true ones. ⛔ Nothing subsumed is DELETED — a subsumed
 *  assertion is free, it names the failure more precisely, and deleting it is how
 *  the live one gets deleted next. It is simply ⛔ NOT COUNTED.
 *
 *  ⛔ LEDGER (a) — VACUITY ("did the walk actually walk the real roster?").
 *     ⛔⛔ **EXACTLY ONE TELL, WEARING FIVE MESSAGES.** This file has no type
 *     filter, so the count assertions and the positive-control assertions form a
 *     single implication CHAIN — each one false forces the next one false:
 *         `TotalRows > 0`  ⟹  `ProbesExecuted > 0`  ⟹  `CONTROL WALKED`
 *                          ⟹  `CONTROL SET`  ⟹  `CONTROL RESOLVED`
 *       • ⭐ THE LIVE MEMBER IS `POSITIVE CONTROL RESOLVED` — the only one of the
 *         five with a world in which it ALONE fires (the control is walked, its
 *         cell is set, and its texture will not load).
 *       • The other four are ⛔ STRICTLY SUBSUMED and are ⛔ NOT COUNTED. In
 *         particular `CONTROL WALKED` cannot fire while `CONTROL SET` passes, and
 *         `CONTROL SET` cannot fire while `CONTROL RESOLVED` passes.
 *     ⛔ The partition, probe-per-row, coverage, distinctness, table-parity and
 *        non-destructiveness assertions are ⛔ NOT vacuity tells at all: every one
 *        of them PASSES AT `0 == 0` (`SC-§51` cl. 1). They are tells for OTHER
 *        properties — ledger (c) — and are labelled as such at their site.
 *
 *  ⛔ LEDGER (b) — INSTRUMENT CONTROLS (`SC-§39`). About the PREDICATE, not the
 *     data. ⛔⛔ **THREE TELLS.**
 *       • POSITIVE — the known-present card must be SET and must LOAD. (This is
 *         the SAME fact as ledger (a)'s live member; it is counted ONCE here and
 *         is why the two ledgers touch at one point rather than being disjoint.)
 *         Without it, "no card art is missing" and "the loader never ran" are the
 *         same green.
 *       • SYNTHESISED (a) — a CLEARED cell must classify UNSET CELL. Fires alone
 *         if the predicate's `IsNull()` branch is ever "simplified" away, while
 *         the loader half still works and all rows still pass.
 *       • ⭐⭐ SYNTHESISED (b) / NEGATIVE — **THE LOAD-BEARING ASSERTION OF THIS
 *         ENTIRE FILE.** A derived, measured-absent, well-formed path must NOT
 *         load. It is the ONLY assertion here that catches a `LoadSynchronous`
 *         GONE BLIND and answering non-null for everything — a mode in which
 *         EVERY row passes, EVERY count is healthy and EVERY partition balances.
 *         ⚖️ A FULL ROW SET AND A BLIND PROBE PRODUCE THE SAME GREEN; only this
 *         distinguishes them (`SC-§51` cl. 3).
 *       • ⚠️ The DISCRIMINATION assertion is ⛔ NOT a fourth tell — it cannot fire
 *         while the three above pass, so it is SUBSUMED. It is kept because it
 *         states the property in one line for a reader.
 *
 *  ⛔ LEDGER (c) — OTHER PROPERTIES (each passes at `0 == 0`; ⛔ none may be
 *     counted as a vacuity tell). Six, each named at its site with the world it
 *     covers: probe-per-row · outcome partition · row-name coverage · art-path
 *     distinctness · card-table PARITY between the two shipped CDOs · and
 *     ⭐ NON-DESTRUCTIVENESS, which fires alone if the synthesis below ever starts
 *     mutating the loaded DataTable instead of a stack copy (`SC-§54` cl. 3).
 *
 *  ── ⛔⛔⭐ `SC-§51` cl. 6 — THIS GATE WAS **BORN GREEN**, AND `SHIP-§9` IS NOT
 *     WAIVED BY THAT ───────────────────────────────────────────────────────────
 *
 *  ⛔ `TASK-947` was LUCKY: `BP_Unit_Witch` was genuinely absent, so its red was
 *  real and unsynthesised. THERE IS NO SUCH FREE RED HERE. Measured 2026-09-03 at
 *  `f050caf`: 32 rows in `Docs/Data/cards.csv`, 32 `T_CardArt_*.uasset` in
 *  `Content/UI/CardArt/`, an EXACT bijection with zero orphans in both directions,
 *  and `Content/Data/DT_Cards.uasset` carries `/Game/UI/CardArt` 32 times.
 *  ⇒ 32/32 PRESENT. ⛔ A gate that has only ever been seen passing is
 *  `SHIP-§9c` cl. 1's "a status line wearing a gate's clothes".
 *
 *  ⭐⭐ THE ANSWER IS THE SYNTHESIS BLOCK AT THE FOOT OF THIS TEST, AND ITS METHOD
 *  IS FIXED BY `SC-§54` cl. 3: the failure is synthesised FROM A MEASURED-ABSENT
 *  SYNTHETIC INPUT, ⛔ NEVER by disturbing a real deliverable. ⛔ NOT ONE BYTE of
 *  `Content/Data/DT_Cards.uasset`, `Docs/Data/cards.csv` or `Content/UI/CardArt/`
 *  is written, moved or renamed — the two failing rows are LOCAL `FCardRow` COPIES
 *  on this function's stack. (A build-master tried the move-the-asset method on
 *  the sibling gate; the permission system refused it, and a new task ID does not
 *  make it a different move.)
 *
 *  ⭐ AND THE SYNTHESIS IS **PERMANENT**, WHICH IS THE WHOLE POINT
 *  (`SC-§54` cl. 3(c)): a disturbed asset buys ONE transcript that decays into a
 *  screenshot; a synthetic input buys an assertion that RE-PROVES ON EVERY RUN,
 *  FOREVER, that the predicate this gate rests on still returns the failing value
 *  for both miss kinds. ⛔ IT IS INVERTED, NOT RE-RUN: it asserts the MISS, so it
 *  is GREEN **BECAUSE** the probe answered MISS. ⛔ A control that turns the suite
 *  red is a bug wearing a control's clothes. ⛔ NOTHING below calls `AddError` on
 *  an EXPECTED miss — every `AddError` in this file is a self-check failure or a
 *  real roster defect.
 *
 *  ── ⛔ `SC-§40` cl. 10 — THE SYNTHETIC WAS **MEASURED** ABSENT, NOT ASSUMED ──
 *
 *  ⚠️ The suffix had to survive a trap that nearly caught `TASK-964`: A SYNTHETIC
 *  NAME THAT IS A SUBSTRING OF SOMETHING ALREADY IN THE TREE CANNOT BE PROVEN
 *  ABSENT BY THE VERY GREP USED TO PROVE IT. Measured 2026-09-03 at `f050caf`,
 *  repo-wide (`.git`, `Binaries`, `Intermediate`, `DerivedDataCache`, `Saved`
 *  excluded), and in BOTH substring directions:
 *      `QqSyntheticAbsentQq`   0 files / 0 hits   ⇐ THE SUFFIX USED HERE
 *      `ZzNoSuchCardZz`        6 files / 30 hits  ⇐ the sibling file's constant
 *      `Sorcerer`            157 files / 2189 hits   (instrument positive control)
 *      `T_CardArt_`           86 files / 798 hits    (instrument positive control)
 *  and separately in `Content/Data/DT_Cards.uasset` as a binary blob:
 *      `QqSyntheticAbsentQq` 0   vs   `Sorcerer` 4 (the same instrument, non-zero).
 *  ⛔ Neither `ZzNoSuchCardZz` nor any real sentinel is a substring of
 *  `QqSyntheticAbsentQq`, and it is a substring of none of them.
 *
 *  ── MECHANISM — read-only; ⛔ zero writes, ⛔ no world, ⛔ no PIE, ⛔ no spawns
 *
 *  The walk LOADS, because loading is the consumer's predicate and a texture that
 *  is present-but-unloadable is exactly the defect a mere existence probe would
 *  miss. The one deliberate failing load in the synthesis block emits an engine
 *  WARNING (`LogUObjectGlobals: Failed to find object '…'` — verified at source to
 *  be `ELogVerbosity::Warning`, never an Error, absent `-TREATLOADWARNINGSASERRORS`);
 *  it is declared to the harness by MY OWN derived suffix rather than by the
 *  engine's prose (`SC-§38` — an engine message reword must not break this file),
 *  with `Occurrences = -1`, which the framework documents as "silently ignored"
 *  and which therefore ⛔ CANNOT ITSELF TURN THE SUITE RED IF THE MESSAGE NEVER
 *  APPEARS. The same call shape is already in the executed suite
 *  (`SiegeAssistantSelectionTest.cpp`).
 */

namespace SiegeCardArtRosterTestFixture
{
	/**
	 *  ⭐ THE POSITIVE CONTROL (`SC-§39`). A known-present card whose art must be
	 *  SET and must LOAD. If this card is ever retired the control goes red and a
	 *  new one must be chosen — that cost is deliberate and is what a control is
	 *  for. ⛔ It must NOT be "fixed" by deleting the control.
	 */
	static const TCHAR* const PositiveControlCardID = TEXT("Sorcerer");

	/**
	 *  ⭐ THE NEGATIVE CONTROL'S SUFFIX (`SC-§39`, `SC-§40` cl. 10). Appended to a
	 *  REAL authored card-art path to make one that is well-formed, in the real
	 *  card-art directory, and MEASURED ABSENT — see the file header for the
	 *  four-instrument census and its positive controls, and for why a name that is
	 *  a SUBSTRING of an existing sentinel would have been unmeasurable.
	 */
	static const TCHAR* const SyntheticAbsentSuffix = TEXT("QqSyntheticAbsentQq");

	/**
	 *  The three outcomes of the shipped card-art resolution, mirrored from
	 *  `UCardHandWidget::ResolveCardArtTexture`'s branch structure. ⛔ AN UNSET CELL
	 *  AND AN UNRESOLVABLE PATH ARE TWO DIFFERENT FINDINGS with two different
	 *  repairs — one is a data-entry gap, the other is a missing or renamed
	 *  texture — and collapsing them into "no art" is how the second one gets
	 *  repaired by filling in a cell that was already correct.
	 */
	enum class ECardArtOutcome : uint8
	{
		Resolved,
		UnsetCell,
		UnresolvablePath
	};

	/**
	 *  ⭐⭐ THE PREDICATE. These are the consumer's own two expressions, on the
	 *  consumer's own `TSoftObjectPtr<UTexture2D>`, in the consumer's own order —
	 *  `Row.CardArt.IsNull()` then `Row.CardArt.LoadSynchronous()`
	 *  (`CardHandWidget.cpp`, `UCardHandWidget::ResolveCardArtTexture`, located BY
	 *  SYMBOL per `SC-§38`). ⛔ Nothing is composed and nothing is re-implemented:
	 *  the only thing omitted is the once-per-CardID warning, which is presentation.
	 *
	 *  ⛔ ONE FUNCTION, TWO CALLERS — the roster walk AND the synthesis block. That
	 *  is deliberate: a synthesis that drove a private copy of the predicate would
	 *  be a test of its own scratch work and would prove nothing about the walk.
	 */
	static ECardArtOutcome ResolveCardArt(const FCardRow& Row, UTexture2D*& OutTexture)
	{
		OutTexture = nullptr;

		if (Row.CardArt.IsNull())
		{
			return ECardArtOutcome::UnsetCell;
		}

		OutTexture = Row.CardArt.LoadSynchronous();
		return OutTexture ? ECardArtOutcome::Resolved : ECardArtOutcome::UnresolvablePath;
	}

	/** The outcome NAME for human-readable messages — every enumerator named, no `default:` label (`SC-§51` cl. 5). */
	static const TCHAR* CardArtOutcomeSymbol(ECardArtOutcome Outcome)
	{
		switch (Outcome)
		{
		case ECardArtOutcome::Resolved:         return TEXT("RESOLVED");
		case ECardArtOutcome::UnsetCell:        return TEXT("UNSET CELL");
		case ECardArtOutcome::UnresolvablePath: return TEXT("UNRESOLVABLE PATH");
		}

		// Reached only by a value the enum does not name. Reported, never silently
		// folded into one of the three above.
		return TEXT("<ECardArtOutcome value nobody named>");
	}

	/**
	 *  ⚠️ THE LOOKUP CHECKS THE POINTED-TO TYPE, NOT JUST THE FIELD NAME. A property
	 *  renamed OR retyped must be reported BY NAME, because the alternative is a
	 *  roster that stays empty and a gate that passes because there was nothing to
	 *  check. (Deliberate duplicate of the sibling's helper — see the file header.)
	 */
	static const FSoftObjectPtr* FindSoftObjectField(const UObject* Object, const TCHAR* FieldName, const UClass* ExpectedPointeeClass)
	{
		const FSoftObjectProperty* const SoftProperty = FindFProperty<FSoftObjectProperty>(Object->GetClass(), FieldName);
		// `.Get()` rather than comparing the TObjectPtr directly — the raw-pointer
		// comparison is well-defined across every build configuration.
		if (!SoftProperty || SoftProperty->PropertyClass.Get() != ExpectedPointeeClass)
		{
			return nullptr;
		}
		return SoftProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Object);
	}

	/** The `ECardType` entry NAME (the symbol, `SC-§38`) for the per-type census — never a raw integer in a message a human reads. */
	static FString CardTypeSymbol(ECardType CardType)
	{
		const UEnum* const CardTypeEnum = StaticEnum<ECardType>();
		if (!CardTypeEnum)
		{
			return FString::Printf(TEXT("<ECardType reflection unavailable; raw %d>"), static_cast<int32>(CardType));
		}

		FString Symbol = CardTypeEnum->GetNameStringByValue(static_cast<int64>(CardType));
		if (Symbol.IsEmpty())
		{
			Symbol = FString::Printf(TEXT("<not an ECardType enumerator; raw %d>"), static_cast<int32>(CardType));
		}
		return Symbol;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardArtRosterTest,
	"Siegebound.CardRoster.EveryCardRowResolvesItsCardArtTexture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardArtRosterTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardArtRosterTestFixture;

	// ══ SELF-CHECKS ═════════════════════════════════════════════════════════
	// ⛔ EVERY ONE OF THESE RETURNS FALSE RATHER THAN CONTINUING. A gate that
	//    cannot reach its subject must FAIL, never report SAFE — an unreadable
	//    instrument and a healthy roster otherwise produce the identical green bar.

	const UObject* const WidgetDefaults = UCardHandWidget::StaticClass()->GetDefaultObject();
	if (!WidgetDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: UCardHandWidget's CDO is null — the card table this gate walks is derived from it, so nothing below would mean anything."));
		return false;
	}

	// ── (a) THE CARD TABLE, OFF THE CONSUMER'S OWN CDO ───────────────────────
	const FSoftObjectPtr* const WidgetTableValue =
		FindSoftObjectField(WidgetDefaults, TEXT("CardTableAsset"), UDataTable::StaticClass());
	if (!WidgetTableValue)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: UCardHandWidget::CardTableAsset (TSoftObjectPtr<UDataTable>) was not reachable on the CDO — it was RENAMED or RETYPED. The card-art roster cannot be derived, and a gate that walked zero rows would pass VACUOUSLY."));
		return false;
	}

	const FSoftObjectPath WidgetTablePath = WidgetTableValue->ToSoftObjectPath();
	if (WidgetTablePath.IsNull())
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: UCardHandWidget::CardTableAsset is UNSET on the CDO — the shipped constructor assigns it, so an empty path means that assignment was removed and every card face would silently fall back to text-only."));
		return false;
	}

	const UDataTable* const CardTable = TSoftObjectPtr<UDataTable>(WidgetTablePath).LoadSynchronous();
	if (!CardTable)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: the card table '%s' — the exact asset the shipped card-hand widget loads — did not load as a UDataTable."), *WidgetTablePath.ToString()));
		return false;
	}

	if (CardTable->GetRowStruct() != FCardRow::StaticStruct())
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: card table '%s' has row struct '%s', not FCardRow — every FindRow<FCardRow> below would return null and the walk would silently check nothing."),
			*WidgetTablePath.ToString(), *GetNameSafe(CardTable->GetRowStruct())));
		return false;
	}

	// ── (b) THE SECOND SHIPPED COPY OF THE TABLE PATH ────────────────────────
	// ⛔ NOT a self-check — an ASSERTION, and it pins a drift surface nothing else
	//    watches. `/Game/Data/DT_Cards` is written TWICE in shipped C++: once in
	//    UCardHandWidget's constructor and once in ASiegePlayerController's. If one
	//    moved, the card FACE would read art from a different table than the SPAWN
	//    path resolves against, and every existing gate would stay green.
	//    ⭐ Pinned as a RELATIONSHIP between two CDOs, never as a typed path.
	const UObject* const ControllerDefaults = ASiegePlayerController::StaticClass()->GetDefaultObject();
	if (!TestNotNull(TEXT("ASiegePlayerController's CDO is reachable (it carries the second shipped copy of the card-table path)"), ControllerDefaults))
	{
		return false;
	}

	const FSoftObjectPtr* const ControllerTableValue =
		FindSoftObjectField(ControllerDefaults, TEXT("CardTableAsset"), UDataTable::StaticClass());
	if (!TestNotNull(TEXT("ASiegePlayerController::CardTableAsset (TSoftObjectPtr<UDataTable>) is reachable on the CDO"), ControllerTableValue))
	{
		return false;
	}

	const FSoftObjectPath ControllerTablePath = ControllerTableValue->ToSoftObjectPath();
	TestEqual(TEXT("TABLE PARITY — the card-hand widget and the player controller point at the SAME card table (two independently-written path literals in shipped C++; if one moves, the card FACE and the SPAWN path diverge and nothing else in the tree notices)"),
		WidgetTablePath.ToString(), ControllerTablePath.ToString());

	AddInfo(FString::Printf(TEXT("card table derived from the card-art consumer's own CDO (never typed here): '%s', row struct FCardRow."), *WidgetTablePath.ToString()));

	// ══ THE WALK — EVERY ROW, EVERY CARD TYPE, NO FILTER ════════════════════
	const TArray<FName> RowNames = CardTable->GetRowNames();

	int32 TotalRows          = 0;   // rows the walk actually READ (not what GetRowNames promised)
	int32 ProbesExecuted     = 0;   // predicate evaluations actually RUN — must equal TotalRows
	int32 ResolvedRows       = 0;
	int32 UnsetCells         = 0;   // FINDING (a) — the CardArt column is empty
	int32 UnresolvablePaths  = 0;   // FINDING (b) — the column is set and the texture will not load

	TMap<FString, int32> RowsByTypeSymbol;
	TSet<FString> DistinctArtPaths;
	TArray<FString> UnsetCardIDs;
	TArray<FString> UnresolvableCardIDs;

	bool bPositiveControlWalked   = false;
	bool bPositiveControlSet      = false;
	bool bPositiveControlResolved = false;
	FSoftObjectPath PositiveControlArtPath;

	for (const FName& RowName : RowNames)
	{
		const FCardRow* const Row = CardTable->FindRow<FCardRow>(
			RowName, TEXT("Siegebound.CardRoster.EveryCardRowResolvesItsCardArtTexture"), /*bWarnIfRowMissing=*/ false);

		if (!Row)
		{
			AddError(FString::Printf(TEXT("card table row '%s' is named by GetRowNames() but FindRow<FCardRow> returned null — the row exists and cannot be read, which is NOT the same as a row that is absent. This is the shipped resolver's FIRST degrade branch (`!Row`) and it would leave that card's face text-only."), *RowName.ToString()));
			continue;
		}

		++TotalRows;

		const FString TypeSymbol = CardTypeSymbol(Row->CardType);
		RowsByTypeSymbol.FindOrAdd(TypeSymbol)++;

		const bool bIsPositiveControl = RowName.ToString().Equals(PositiveControlCardID, ESearchCase::IgnoreCase);
		if (bIsPositiveControl)
		{
			bPositiveControlWalked = true;
		}

		// ── THE ASSERTIONS THIS WHOLE FILE EXISTS FOR ────────────────────────
		// ⛔ ONE predicate call per row, and the outcome is split into the TWO
		//    findings the spec requires be reported separately.
		++ProbesExecuted;
		UTexture2D* ArtTexture = nullptr;
		const ECardArtOutcome Outcome = ResolveCardArt(*Row, ArtTexture);

		// FINDING (a) — the cell itself.
		const bool bCellIsSet = (Outcome != ECardArtOutcome::UnsetCell);
		TestTrue(FString::Printf(TEXT("card '%s' (%s) — its DT_Cards CardArt cell is SET"),
			*RowName.ToString(), *TypeSymbol), bCellIsSet);

		if (bIsPositiveControl)
		{
			bPositiveControlSet = bCellIsSet;
			PositiveControlArtPath = Row->CardArt.ToSoftObjectPath();
		}

		if (!bCellIsSet)
		{
			++UnsetCells;
			UnsetCardIDs.Add(RowName.ToString());
			AddError(FString::Printf(TEXT("⛔ CARD WITH NO CARD ART — UNSET CELL: '%s' (CardType %s) has an EMPTY CardArt column. UCardHandWidget::ResolveCardArtTexture returns nullptr and the card face renders as TEXT ONLY — it does not error, it does not crash, and nobody finds out until someone looks at that one card. ⛔ The repair is the DATA CELL, never a weakened assertion here (CONVENTIONS SC-§50 cl. 4)."),
				*RowName.ToString(), *TypeSymbol));
			continue;
		}

		const FString ArtPathString = Row->CardArt.ToSoftObjectPath().ToString();
		DistinctArtPaths.Add(ArtPathString);

		// FINDING (b) — the path the cell points at. ⛔ A SEPARATE FINDING with a
		// SEPARATE repair: (a) is a data-entry gap, (b) is a missing, renamed or
		// unimported texture. Collapsing them is how (b) gets "fixed" by editing a
		// cell that was already correct.
		const bool bTextureLoaded = (Outcome == ECardArtOutcome::Resolved);
		TestTrue(FString::Printf(TEXT("card '%s' (%s) — its CardArt '%s' LOADS as a UTexture2D"),
			*RowName.ToString(), *TypeSymbol, *ArtPathString), bTextureLoaded);

		if (bIsPositiveControl)
		{
			bPositiveControlResolved = bTextureLoaded;
		}

		if (!bTextureLoaded)
		{
			++UnresolvablePaths;
			UnresolvableCardIDs.Add(RowName.ToString());
			AddError(FString::Printf(TEXT("⛔ CARD WITH BROKEN CARD ART — UNRESOLVABLE PATH: '%s' (CardType %s) points at '%s', which does not load as a UTexture2D. The cell is SET, so this is NOT a data-entry gap — the texture is missing, renamed or never imported. The card face degrades to TEXT ONLY with a once-per-CardID warning and never a crash. ⛔ The repair is the ASSET, never a weakened assertion here (CONVENTIONS SC-§50 cl. 4)."),
				*RowName.ToString(), *TypeSymbol, *ArtPathString));
			continue;
		}

		++ResolvedRows;
	}

	// ══ THE COUNTS, ASSERTED — ⛔ NOT MERELY LOGGED ═════════════════════════
	//
	// ⭐ RELATIONSHIPS, NOT LITERALS (`SC-§40` cl. 10). ⛔ NO EXPECTED ROW COUNT
	//    APPEARS IN ANY ASSERTION OR ANY RUNTIME MESSAGE IN THIS FILE — the only
	//    roster sizes written down anywhere here are the DATED, HASH-PINNED
	//    measurements in the header (`SC-§53` cl. 3), which cannot rot. A
	//    `TestEqual(TotalRows, 32)` would turn the gate red on card #33 while the
	//    roster is perfectly healthy, and the number would be bumped by the person
	//    least likely to re-read the roster — `TASK-874`'s trap rebuilt inside its
	//    own cure, which `qa/TASK-948.md` §2 already ruled on for the sibling.

	// ⛔ LEDGER (a), SUBSUMED — kept, NOT counted (`SC-§51` cl. 2). Neither of the
	//    next two can fire while the POSITIVE CONTROL WALKED assertion passes.
	TestTrue(TEXT("COUNT (subsumed tell, kept for its message) — the card table yielded rows to walk"),
		TotalRows > 0);

	TestTrue(TEXT("COUNT (subsumed tell, kept for its message) — at least one card-art predicate was EXECUTED"),
		ProbesExecuted > 0);

	// ⛔ NOT VACUITY TELLS — all three PASS AT `0 == 0` (`SC-§51` cl. 1). They are
	//    tells for OTHER properties, and each is named with the property it covers.
	TestEqual(TEXT("PARTITION — every row read was PROBED (a probe count below the row count means rows were skipped, not passed; the cheap-looking `continue` between 'count it' and 'probe it' is exactly how a roster gate quietly stops probing)"),
		ProbesExecuted, TotalRows);

	TestEqual(TEXT("PARTITION — the outcomes are TOTAL: resolved + unset + unresolvable == probes executed (a row that landed in no bucket was silently dropped from the gate)"),
		ResolvedRows + UnsetCells + UnresolvablePaths, ProbesExecuted);

	TestEqual(TEXT("COVERAGE — every row NAME the table published was READ (a shortfall means FindRow failed on a row that exists, which is a different defect from a row that is absent)"),
		TotalRows, RowNames.Num());

	// ⛔ THE TWO FINDINGS, ASSERTED SEPARATELY AND BY COUNT, as the spec requires.
	//    ⚠️ Each is SUBSUMED by its own per-row assertion above and is not counted
	//    as an independent tell; it is here because the two findings have DIFFERENT
	//    REPAIRS and a reader of the summary must be able to tell them apart.
	TestEqual(TEXT("FINDING (a) — ZERO card rows have an UNSET CardArt cell"),
		UnsetCells, 0);

	TestEqual(TEXT("FINDING (b) — ZERO card rows have a CardArt path that will not load"),
		UnresolvablePaths, 0);

	TestEqual(TEXT("DISTINCTNESS — every set CardArt cell names a DISTINCT texture (two cards sharing one path means one card is silently wearing another's face)"),
		DistinctArtPaths.Num(), ResolvedRows + UnresolvablePaths);

	// ══ LEDGER (a) + (b) — THE POSITIVE-CONTROL CHAIN ═══════════════════════
	//
	// ⛔ THESE THREE ARE **ONE TELL**, NOT THREE (`SC-§51` cl. 2), and so are the
	//    two count assertions above them: WALKED false ⟹ SET false ⟹ RESOLVED
	//    false. ⭐ THE LIVE MEMBER IS `RESOLVED` — the only one of the five with a
	//    world in which it ALONE fires (control walked, cell set, texture will not
	//    load). The other four are KEPT for their messages and NOT COUNTED.
	// ⭐ This one tell serves BOTH ledgers: it is the vacuity tell (a walk that read
	//    nothing did not read the control either) AND `SC-§39`'s positive
	//    instrument control (the predicate can still answer RESOLVED).

	TestTrue(FString::Printf(TEXT("VACUITY (subsumed member, kept for its message) — the known-present card '%s' was REACHED by the walk"), PositiveControlCardID),
		bPositiveControlWalked);

	TestTrue(FString::Printf(TEXT("POSITIVE CONTROL (subsumed member, kept for its message) — '%s' has a SET CardArt cell, so the IsNull() half of the predicate can answer 'set'"), PositiveControlCardID),
		bPositiveControlSet);

	TestTrue(FString::Printf(TEXT("⭐ POSITIVE CONTROL — '%s' RESOLVED to a UTexture2D. ⛔ THE LIVE MEMBER OF THE CHAIN: it is what separates 'no card art is missing' from 'the walk never ran' AND from 'the loader cannot return PRESENT'"), PositiveControlCardID),
		bPositiveControlResolved);

	// ══ ⛔⛔⭐⭐ THE SYNTHESIS — `SC-§51` cl. 6 + `SC-§54` cl. 3 ═══════════════
	//
	// ⛔ THIS GATE WAS BORN GREEN — every card-art asset was present when it was
	//    written (the dated, hash-pinned census is in the file header) — SO IT HAS
	//    NO FREE RED. `SHIP-§9` IS NOT WAIVED BY THAT. Below, BOTH failure modes are
	//    driven for real, through the SAME `ResolveCardArt` the walk just used, on
	//    LOCAL COPIES of a real row.
	//
	// ⛔⛔ NOT ONE BYTE OF `DT_Cards.uasset`, `cards.csv` OR `Content/UI/CardArt/`
	//    IS TOUCHED. `FCardRow` is copied BY VALUE off the table onto this stack;
	//    the table's own row memory is never written and no asset is ever opened for
	//    write, moved or renamed. That is the method `SC-§54` cl. 3 fixes, and it is
	//    fixed because the alternative — moving a real deliverable aside — was
	//    REFUSED by the permission system on the sibling gate and correctly not
	//    routed around.
	//
	// ⚠️ THE ONE THING THAT IS SCOPED RATHER THAN ABSOLUTE, DECLARED SO NOBODY HAS
	//    TO DISCOVER IT: `LoadSynchronous()` warms `FSoftObjectPtr`'s internal weak
	//    cache, which is `mutable`, so the walk above does mutate that cache on the
	//    loaded table's rows. ⛔ It does NOT change any serialized value, does NOT
	//    mark the package dirty and does NOT reach disk — and it is EXACTLY what the
	//    shipped card-hand widget does on every hand refresh. The claim this file
	//    makes is about the ASSETS AND THE INDEX, and that claim is absolute.
	//
	// ⭐ THE DONOR IS THE POSITIVE CONTROL, deliberately: it is the one row this
	//    test has ALREADY PROVEN resolves, so the three states below are a
	//    single-variable A/B/C on one known-good input. ⛔ The assertions are
	//    INVERTED — they assert the MISS — so this block is GREEN **BECAUSE** the
	//    predicate answered MISS, and it never calls AddError on an expected miss.

	if (!TestTrue(TEXT("SYNTHESIS PREMISE — the positive control resolved, so it can serve as the known-good donor row for both synthesised failures"),
		bPositiveControlResolved && !PositiveControlArtPath.IsNull()))
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: there is no known-good donor row, so neither failure mode can be demonstrated. This is a defect in the control, not in the roster."));
		return false;
	}

	const FCardRow* const DonorRow = CardTable->FindRow<FCardRow>(
		FName(PositiveControlCardID), TEXT("Siegebound.CardRoster.EveryCardRowResolvesItsCardArtTexture"), /*bWarnIfRowMissing=*/ false);
	if (!TestNotNull(FString::Printf(TEXT("SYNTHESIS PREMISE — the donor row '%s' is re-readable from the shipped table"), PositiveControlCardID), DonorRow))
	{
		return false;
	}

	// ⛔ Declared to the harness by MY OWN derived suffix, never by the engine's
	//    message prose (`SC-§38`), and with Occurrences = -1, which the framework
	//    documents as "silently ignored" ⇒ this line CANNOT turn the suite red if
	//    the engine ever stops emitting the warning or rewords it.
	AddExpectedMessagePlain(SyntheticAbsentSuffix, ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	// ── STATE C (the control): the donor UNTOUCHED ───────────────────────────
	UTexture2D* DonorTexture = nullptr;
	const ECardArtOutcome DonorOutcome = ResolveCardArt(*DonorRow, DonorTexture);

	TestTrue(FString::Printf(TEXT("SYNTHESIS CONTROL — the UNTOUCHED donor row '%s' resolves (outcome %s). Without this, the two synthesised misses below would be satisfied by a predicate that returns a miss for EVERYTHING"),
		PositiveControlCardID, CardArtOutcomeSymbol(DonorOutcome)),
		DonorOutcome == ECardArtOutcome::Resolved);
	TestNotNull(FString::Printf(TEXT("SYNTHESIS CONTROL — the untouched donor '%s' yields a non-null UTexture2D"), PositiveControlCardID), DonorTexture);

	// ── STATE A: SYNTHESISED **UNSET CELL** ──────────────────────────────────
	// ⭐ THE COMPILED, PERMANENT FORM OF RED (a). This is the exact value that
	//    takes the walk's `card '<X>' — its CardArt cell is SET` assertion red.
	FCardRow SyntheticUnsetRow = *DonorRow;                              // stack copy
	SyntheticUnsetRow.CardArt = TSoftObjectPtr<UTexture2D>();            // cleared IN MEMORY only

	UTexture2D* UnsetTexture = nullptr;
	const ECardArtOutcome UnsetOutcome = ResolveCardArt(SyntheticUnsetRow, UnsetTexture);

	TestTrue(FString::Printf(TEXT("⭐ SYNTHESISED RED (a) — an UNSET CardArt cell on card '%s' is classified %s by the SAME predicate the roster walk above uses (got %s). This is the value that turns this gate RED, observed on every pass instead of only when something is broken"),
		PositiveControlCardID, CardArtOutcomeSymbol(ECardArtOutcome::UnsetCell), CardArtOutcomeSymbol(UnsetOutcome)),
		UnsetOutcome == ECardArtOutcome::UnsetCell);
	TestNull(FString::Printf(TEXT("⭐ SYNTHESISED RED (a) — the unset cell on '%s' yields NO texture, which is precisely what leaves a shipped card face text-only"), PositiveControlCardID), UnsetTexture);

	// ── STATE B: SYNTHESISED **BOGUS BUT WELL-FORMED PATH** ──────────────────
	// ⛔ DERIVED, NEVER COMPOSED: the donor's own authored package and asset names
	//    plus a measured-absent suffix. ⇒ same directory, same prefix, same shape —
	//    and no `/Game/...` string literal enters executable code, so this file adds
	//    no sixth copy of any path contract for `TASK-959` to have to find. If the
	//    card-art folder ever moves, this control moves with it for free.
	const FString BogusPackageName = PositiveControlArtPath.GetLongPackageName() + SyntheticAbsentSuffix;
	const FString BogusAssetName   = PositiveControlArtPath.GetAssetName()       + SyntheticAbsentSuffix;
	const FSoftObjectPath BogusArtPath(BogusPackageName + TEXT(".") + BogusAssetName);

	FCardRow SyntheticBogusRow = *DonorRow;         // stack copy
	SyntheticBogusRow.CardArt = BogusArtPath;       // re-pointed IN MEMORY only

	TestTrue(FString::Printf(TEXT("SYNTHESIS PREMISE — the bogus path '%s' is WELL-FORMED and DIFFERENT from the donor's authored '%s' (identical strings would make the discrimination assertion below vacuous)"),
		*BogusArtPath.ToString(), *PositiveControlArtPath.ToString()),
		!BogusArtPath.IsNull() && BogusArtPath != PositiveControlArtPath);

	UTexture2D* BogusTexture = nullptr;
	const ECardArtOutcome BogusOutcome = ResolveCardArt(SyntheticBogusRow, BogusTexture);

	TestTrue(FString::Printf(TEXT("⭐⭐ SYNTHESISED RED (b) / NEGATIVE CONTROL — card '%s' pointed at the MEASURED-ABSENT path '%s' is classified %s (got %s). ⛔ THIS IS THE LOAD-BEARING ASSERTION OF THIS FILE: it is the ONLY one that catches a LoadSynchronous gone BLIND and answering non-null for everything — a mode in which all rows above pass, every count is healthy and every partition balances"),
		PositiveControlCardID, *BogusArtPath.ToString(), CardArtOutcomeSymbol(ECardArtOutcome::UnresolvablePath), CardArtOutcomeSymbol(BogusOutcome)),
		BogusOutcome == ECardArtOutcome::UnresolvablePath);
	TestNull(FString::Printf(TEXT("⭐⭐ SYNTHESISED RED (b) — the bogus path on '%s' yields NO texture"), PositiveControlCardID), BogusTexture);

	// ── DISCRIMINATION: one symbol, three answers, one run ───────────────────
	// ⚠️ ⛔ NOT COUNTED AS A TELL (`SC-§51` cl. 2): it cannot fire while the three
	//    outcome assertions above pass, so it is SUBSUMED by them. It is kept
	//    because it states the whole property in one readable line — and because
	//    deleting a subsumed assertion is how the live ones get deleted next.
	TestTrue(FString::Printf(TEXT("DISCRIMINATION (subsumed, kept for its message) — ONE predicate (ResolveCardArt) returned THREE DIFFERENT outcomes for the SAME card '%s' under three inputs in the SAME run: %s / %s / %s. A predicate stuck on ANY single answer fails here, and it fails at the three assertions above first"),
		PositiveControlCardID, CardArtOutcomeSymbol(DonorOutcome), CardArtOutcomeSymbol(UnsetOutcome), CardArtOutcomeSymbol(BogusOutcome)),
		DonorOutcome != UnsetOutcome && DonorOutcome != BogusOutcome && UnsetOutcome != BogusOutcome);

	TestTrue(TEXT("⛔ NON-DESTRUCTIVENESS — the synthesis wrote only STACK COPIES: the donor row still resolves in the live table AFTER both syntheses (if this ever goes red, the synthesis has started mutating the loaded DataTable and must be fixed, never suppressed)"),
		DonorRow->CardArt.ToSoftObjectPath() == PositiveControlArtPath && !DonorRow->CardArt.IsNull());

	// ══ THE REPORT ══════════════════════════════════════════════════════════
	// Published under the SAME predicate it was measured under (`SC-§49`), with the
	// two findings NAMED and SEPARATE — never summarised as "no art".
	AddInfo(FString::Printf(TEXT("CARD ART — %d row(s) read; %d predicate(s) executed; %d RESOLVED; %d UNSET cell(s); %d UNRESOLVABLE path(s); %d distinct texture(s)."),
		TotalRows, ProbesExecuted, ResolvedRows, UnsetCells, UnresolvablePaths, DistinctArtPaths.Num()));

	for (const TPair<FString, int32>& Entry : RowsByTypeSymbol)
	{
		AddInfo(FString::Printf(TEXT("  CardType %-12s %3d row(s)   [card art is asserted for EVERY type, not only the spawnable ones]"), *Entry.Key, Entry.Value));
	}

	if (UnsetCells > 0)
	{
		AddInfo(FString::Printf(TEXT("⛔ FINDING (a) UNSET CELL — %d card(s) whose CardArt column is empty: %s   [repair: the DATA CELL]"),
			UnsetCells, *FString::Join(UnsetCardIDs, TEXT(", "))));
	}

	if (UnresolvablePaths > 0)
	{
		AddInfo(FString::Printf(TEXT("⛔ FINDING (b) UNRESOLVABLE PATH — %d card(s) whose CardArt will not load: %s   [repair: the ASSET, not the cell]"),
			UnresolvablePaths, *FString::Join(UnresolvableCardIDs, TEXT(", "))));
	}

	AddInfo(FString::Printf(TEXT("SYNTHESISED RED (a) — ResolveCardArt('%s' with a CLEARED cell) = %s   [in-memory stack copy; no asset touched]"),
		PositiveControlCardID, CardArtOutcomeSymbol(UnsetOutcome)));
	AddInfo(FString::Printf(TEXT("SYNTHESISED RED (b) — ResolveCardArt('%s' -> '%s') = %s   [in-memory stack copy; no asset touched]"),
		PositiveControlCardID, *BogusArtPath.ToString(), CardArtOutcomeSymbol(BogusOutcome)));
	AddInfo(FString::Printf(TEXT("SYNTHESIS CONTROL      — ResolveCardArt('%s' UNTOUCHED) = %s"),
		PositiveControlCardID, CardArtOutcomeSymbol(DonorOutcome)));
	AddInfo(TEXT("⭐ ⇒ the card-art predicate answered RESOLVED for a real cell, UNSET CELL for a cleared one and UNRESOLVABLE PATH for a measured-absent one — through ONE symbol, in ONE run. The walk above asserts that same predicate returns RESOLVED for every row; this block demonstrates it is still capable of returning both misses."));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
