// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Math/NumericLimits.h"
#include "Siegebound/SiegeAssistantCommand.h"
#include "Siegebound/SiegeAssistantComponent.h"
#include "Siegebound/SiegeAssistantGrammar.h"
#include "Siegebound/SiegeAssistantRegionStatics.h"
#include "Siegebound/SiegeAssistantSnapshot.h"
#include "Siegebound/SiegeAssistantVocabulary.h"
#include "Siegebound/SiegeKeyboardLayoutStatics.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ────────────────────────────────────────────────────────────────────────────
 *  THE EXCLUSION + ROSTER-VISIBILITY MEASUREMENT (batch ASSISTANT-EXCLUDE,
 *  TASK-523 — CONVENTIONS AS-§20, especially AS-§20.2, AS-§20.4, AS-§20.6 and
 *  the AS-§20.7 naming law that pins this file's PATH, its NAMESPACE
 *  (`Siegebound.Assistant.Selection.<Name>`) and its QA gate (TASK-525)).
 *  ────────────────────────────────────────────────────────────────────────────
 *
 *  M8 DECLARATION DUTY (stated verbatim as required):
 *  "adds no replicated property, no new replicated class, no new relevancy tier."
 *
 *  ── ⭐ WHY THIS FILE IS THE ONLY INSTRUMENT FOR JONATHAN'S SORCERER DEFECT ──
 *
 *  He reported: "whenever I say all units, it doesn't seem to include sorcerers
 *  even when they were spawned." AS-§20.2 found the cause in the PROMPT: the
 *  roster is elastic, `Sorcerer` is the LAST DT_Cards commandable row, so it is
 *  ALWAYS the first kind the trimmer collapses — and the collapse line used to
 *  print COUNTS WITH NO SYMBOLS (`other_kinds: 5 kinds, 9 units`). The token
 *  `sorcerer` therefore never reached the model, and Zone A's "if the unit named
 *  is not a kind in [FORCES], answer unsupported" rule then refused a unit that
 *  was standing on the board.
 *
 *  ⛔ THE EVALUATION CORPUS STRUCTURALLY CANNOT ASSERT THIS, AND TASK-524 HIT
 *  THAT WALL AND REPORTED IT (its finding F-1). The correct model answer to an
 *  "all units" order is `who:"all"`, which parses to an EMPTY kinds array; an
 *  empty expectation cell means "NOT ASSERTED" (AS-§11's known limitation), and
 *  filling the cell with 13 kinds would make the CORRECT answer fail. TASK-524's
 *  finding F-5 goes further: the eval lane hardcodes fixture `t0`, whose roster
 *  prints all 13 kinds UNCONDITIONALLY, so THE COLLAPSE THAT HID THE SORCERER
 *  HAS NEVER HAPPENED ON THAT LANE. A green corpus row is therefore not evidence
 *  about the collapse leg at all.
 *
 *  ⇒ AS-§20.6 names the three instruments this batch actually owes, and they are
 *    all here: (a) a >8-kind board PRINTS `sorcerer`; (b) the COLLAPSE path
 *    prints the collapsed kinds BY NAME; (c) parser tests for every exclusion
 *    accept/refuse case. ⛔ "A repair with no failing test to close is not a
 *    repair" — before this file, both of Jonathan's reported defects were
 *    unmeasurable.
 *
 *  ── ⛔ THE ASSERTIONS RUN AGAINST THE SHIPPED BUILDER, NOT AGAINST A COPY ──
 *
 *  Every roster claim below is made against `USiegeAssistantSnapshot::BuildZoneC`
 *  and, through it, the private `AppendRosterBlock` — the real elastic trimmer,
 *  the real fixed-key order, the real collapse line. ⛔ NOTHING here transcribes
 *  the builder's output into a fixture and compares the builder to it: that is
 *  the "guardrail that reports SAFE" shape AS-§12g rules worse than no test, and
 *  it is the exact trap the sibling ZoneA file's own comments rail against.
 *
 *  ⚠️ HOW THE ROSTER IS POPULATED WITHOUT A WORLD, AND WHY IT IS HONEST.
 *  `UnitKinds` / `KindTotals` / `KindOrderable` / `KindFollowable` / `PlaceNames`
 *  are PRIVATE members filled only by `Capture(UWorld*, ETeamId)`, and TASK-523's
 *  spec forbids building a world fixture ("that is a different, larger task and
 *  it is not authorised here"). They are, however, every one of them
 *  `UPROPERTY(Transient)`, so this file writes them THROUGH THE REFLECTION
 *  SYSTEM — the same values `Capture()` would have written, into the same fields,
 *  after which the SHIPPED builder runs unmodified.
 *
 *  ⛔ AND THE FAILURE MODE OF THAT TECHNIQUE IS GUARDED RATHER THAN ASSUMED: a
 *  renamed or retyped field makes `FindFProperty` return null, which would leave
 *  the snapshot EMPTY and let a collapse assertion pass VACUOUSLY. So every
 *  reflection write is checked, the inner property TYPE is checked too, and a
 *  miss is an explicit AddError naming the field. A test that silently stops
 *  testing is the thing this whole batch exists to prevent.
 *
 *  ⚠️ WHAT THIS FILE CANNOT REACH, STATED UP FRONT RATHER THAN LEFT TO BE
 *  DISCOVERED (TASK-523 spec item 7, and TASK-522's handoff says the same thing
 *  from the other side):
 *   - THE EXECUTOR'S EXCLUSION FILTER. `SelectUnitsForOrder` walks a `UWorld`
 *     for `ASummonedUnit` actors. These are EditorContext SIMPLE tests: no world,
 *     no actors. Executor exclusion is covered by TASK-522's three log lines
 *     (subtracted / subtracted-nobody / emptied-and-refused) and by Jonathan at
 *     TASK-527 — ⛔ NOT by a test, and this file does not pretend otherwise.
 *   - `NativeOnPreviewKeyDown`. It is `protected`, its whole gate
 *     (`bConsoleOpen` / `bConsoleEnabled` / `bConfirmPromptVisible`) is
 *     `private`, and reaching those states needs a player controller, Slate
 *     focus and a live FSM. What IS reachable is the ACCEPT-KEY RESOLUTION the
 *     handler compares against, and that is asserted below.
 *   - THE `Cancelled` TRANSCRIPT LINE (TASK-520). `NotifyConsoleClosed` only
 *     prints it from `AwaitConfirm`, and `AwaitConfirm` is entered only through
 *     the private `EnterAwaitConfirm()` at the end of a full model turn. There is
 *     no headless path into that state.
 *
 *  ⛔⛔ `TestEqualSensitive`, NEVER `TestEqual`, FOR EVERY FString CLAIM.
 *  `FAutomationTestBase::TestEqual(const FString&, const FString&)` forwards to
 *  the TCHAR* overload, which is CASE-INSENSITIVE (SC-§13). A shipped, QA-passed
 *  test in this project once asserted nothing for exactly that reason. Symbol
 *  claims here are about BYTES the model reads, so case is load-bearing:
 *  `sorcerer` and `Sorcerer` tokenize differently.
 */

namespace SiegeAssistantSelectionTestFixture
{
	// ═══════════════════════════════════════════════════════════════════════════
	//  THE BOARD
	// ═══════════════════════════════════════════════════════════════════════════

	/**
	 *  THE 13 COMMANDABLE KINDS, IN DT_Cards ROW ORDER — read off Docs/Data/cards.csv
	 *  and lower-cased exactly as `USiegeAssistantSnapshot`'s `CanonicalKind` does
	 *  (`FName(*CardID.ToString().ToLower())`).
	 *
	 *  ⛔ THE ORDER IS THE TEST, NOT DECORATION. `Sorcerer` is the LAST commandable
	 *  row in the table, which is the entire mechanism of Jonathan's defect: the
	 *  shrink loop always drops from the TAIL, so the Sorcerer is the first kind
	 *  hidden on every board, every time. Sorting this array alphabetically (or
	 *  "tidying" it) would move `sorcerer` to the middle and quietly convert the
	 *  collapse tests into tests of a case that cannot happen.
	 *
	 *  ⚠️ The buildings and spells between these rows (ArrowTower, Wall, BombTower,
	 *  Barracks, the spell cards…) are deliberately absent: `Capture()` only ever
	 *  tallies live `ASummonedUnit`s, so a roster can hold exactly these 13.
	 */
	static TArray<FName> ThirteenKindsInCardRowOrder()
	{
		return TArray<FName>{
			TEXT("footman"), TEXT("archer"), TEXT("knight"), TEXT("miner"),
			TEXT("militiamob"), TEXT("pikeman"), TEXT("sapper"), TEXT("cavalry"),
			TEXT("longbowman"), TEXT("cleric"), TEXT("ogre"), TEXT("wizard"),
			TEXT("sorcerer")
		};
	}

	/** The canonical symbol the whole defect is about. Spelled once so a typo cannot make a test pass by testing the wrong noun. */
	static const TCHAR* const SorcererSymbol = TEXT("sorcerer");

	/**
	 *  The seven place symbols of AS-§9a's pinned v1 vocabulary. Present so
	 *  `BuildZoneC`'s HEAD measures its shipped 108 characters rather than the
	 *  22 an empty place list would give — the roster budget is
	 *  `1085 - 192 - Head - Tail`, so a short head would hand the trimmer ~86
	 *  characters it does not have in play and the collapse tests would be
	 *  measuring a board this game never produces.
	 */
	static TArray<FName> SevenPlaces()
	{
		return TArray<FName>{
			TEXT("own_castle"), TEXT("enemy_castle"), TEXT("mid"),
			TEXT("ancient_ground_near"), TEXT("ancient_ground_far"),
			TEXT("nearest_mine"), TEXT("hero")
		};
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  REFLECTION WRITES INTO THE SNAPSHOT'S PRIVATE, Capture()-OWNED STATE
	// ═══════════════════════════════════════════════════════════════════════════

	/**
	 *  ⚠️ EVERY LOOKUP CHECKS THE INNER PROPERTY TYPE, NOT JUST THE NAME. A field
	 *  renamed OR retyped (say `TArray<FName>` -> `TArray<FString>`) must fail
	 *  LOUDLY here, because the alternative is a snapshot that stays empty and a
	 *  collapse assertion that passes because there was nothing to collapse.
	 */
	static TArray<FName>* FindNameArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner || !ArrayProperty->Inner->IsA<FNameProperty>())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<FName>>(Object);
	}

	static TArray<int32>* FindIntArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner || !ArrayProperty->Inner->IsA<FIntProperty>())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<int32>>(Object);
	}

	static int32* FindIntField(UObject* Object, const TCHAR* FieldName)
	{
		FIntProperty* const IntProperty = FindFProperty<FIntProperty>(Object->GetClass(), FieldName);
		return IntProperty ? IntProperty->ContainerPtrToValuePtr<int32>(Object) : nullptr;
	}

	/** A snapshot plus whichever field name (if any) could not be reached — reported by name so a rename is actionable in one read. */
	struct FScratchSnapshot
	{
		TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot;
		FString MissingField;

		bool IsUsable() const { return Snapshot.IsValid() && MissingField.IsEmpty(); }
	};

	/**
	 *  Builds a snapshot in exactly the state `Capture()` leaves for a board where
	 *  every named kind is alive and fully orderable.
	 *
	 *  @param Kinds  the canonical symbols, IN THE ORDER Capture() would have
	 *                sorted them (DT_Cards row order). The tail of this array is
	 *                what the trimmer collapses.
	 *  @param PerKindTotal  live units of each kind.
	 *
	 *  ⚠️⚠️ THE MAGNITUDE OF THIS NUMBER IS PART OF THE MEASUREMENT, AND IT IS THE
	 *  ONE THING A READER WILL GET WRONG. Every roster row prints the count THREE
	 *  TIMES (total / orderable / followable), so a board with TWO-DIGIT counts is
	 *  3 chars wider PER ROW — 39 chars across 13 kinds — than the same board with
	 *  single-digit counts. AS-§20.3's famous "~6 chars of headroom" (887 of 893)
	 *  is the SINGLE-DIGIT figure: head 108 / roster 621 / tail 158. At
	 *  PerKindTotal >= 10 the same 13-kind block measures 660, and 660 does NOT fit
	 *  the 627-char budget left by the shipped 61-char `order:` line.
	 *
	 *  ⇒ 9 IS USED WHEREVER A TEST NEEDS THE UNCOLLAPSED CASE, so those tests
	 *  reproduce AS-§20.3's own operating point rather than a wider one, and their
	 *  slack is reported by AddInfo rather than asserted. ⛔ The consequence — that
	 *  an ordinary two-digit board is ALREADY collapsing at the default sentence
	 *  length — is deliberately NOT asserted anywhere: it is a live measurement
	 *  reported to TASK-525, and pinning it as a test would make TASK-528's
	 *  ZoneBCharReserve repair fail this file for succeeding.
	 */
	static FScratchSnapshot MakeSnapshotWithRoster(const TArray<FName>& Kinds, int32 PerKindTotal)
	{
		FScratchSnapshot Scratch;
		Scratch.Snapshot.Reset(NewObject<USiegeAssistantSnapshot>());
		if (!Scratch.Snapshot.IsValid())
		{
			Scratch.MissingField = TEXT("<the snapshot object itself>");
			return Scratch;
		}

		USiegeAssistantSnapshot* const Object = Scratch.Snapshot.Get();

		TArray<FName>* const UnitKinds = FindNameArrayField(Object, TEXT("UnitKinds"));
		TArray<int32>* const Totals = FindIntArrayField(Object, TEXT("KindTotals"));
		TArray<int32>* const Orderable = FindIntArrayField(Object, TEXT("KindOrderable"));
		TArray<int32>* const Followable = FindIntArrayField(Object, TEXT("KindFollowable"));
		TArray<FName>* const Places = FindNameArrayField(Object, TEXT("PlaceNames"));
		int32* const StanceFree = FindIntField(Object, TEXT("StanceFree"));

		// Reported as ONE name so the message says which field moved, rather than
		// "something failed".
		if (!UnitKinds)        { Scratch.MissingField = TEXT("UnitKinds (TArray<FName>)"); return Scratch; }
		if (!Totals)           { Scratch.MissingField = TEXT("KindTotals (TArray<int32>)"); return Scratch; }
		if (!Orderable)        { Scratch.MissingField = TEXT("KindOrderable (TArray<int32>)"); return Scratch; }
		if (!Followable)       { Scratch.MissingField = TEXT("KindFollowable (TArray<int32>)"); return Scratch; }
		if (!Places)           { Scratch.MissingField = TEXT("PlaceNames (TArray<FName>)"); return Scratch; }
		if (!StanceFree)       { Scratch.MissingField = TEXT("StanceFree (int32)"); return Scratch; }

		*UnitKinds = Kinds;

		Totals->Reset();
		Orderable->Reset();
		Followable->Reset();
		for (int32 Index = 0; Index < Kinds.Num(); ++Index)
		{
			// Parallel arrays, exactly as Capture() fills them in one pass. Every
			// kind fully orderable and followable keeps the ROW WIDTH at its
			// widest realistic value, which is the conservative direction for a
			// budget test: a narrower row would give the trimmer slack the shipped
			// board does not have.
			Totals->Add(PerKindTotal);
			Orderable->Add(PerKindTotal);
			Followable->Add(PerKindTotal);
		}

		*Places = SevenPlaces();
		*StanceFree = PerKindTotal * Kinds.Num();

		return Scratch;
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  READING ZONE C BACK
	// ═══════════════════════════════════════════════════════════════════════════

	/** The value of the first line beginning `<Key>: `, or an empty string when the key is absent. ⛔ Case-sensitive: these are prompt bytes. */
	static FString ValueOfKey(const FString& ZoneC, const TCHAR* Key)
	{
		TArray<FString> Lines;
		ZoneC.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		const FString Prefix = FString(Key) + TEXT(": ");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return Line.RightChop(Prefix.Len());
			}
		}
		return FString();
	}

	/** True when a `<Key>:` line exists at all — the fixed-key law's actual claim, which is weaker than "it has a value". */
	static bool HasKeyLine(const FString& ZoneC, const TCHAR* Key)
	{
		TArray<FString> Lines;
		ZoneC.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		const FString Prefix = FString(Key) + TEXT(":");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 *  The symbols PRINTED AS FULL ROSTER ROWS, in printed order. A row is
	 *  `- <symbol>: <n> total, <n> orderable, <n> followable`.
	 *
	 *  ⚠️ `- none` is deliberately NOT collected: it is the empty-roster sentinel,
	 *  not a kind, and counting it would let a zero-row block masquerade as a
	 *  one-kind block.
	 */
	static TArray<FString> PrintedRosterSymbols(const FString& ZoneC)
	{
		TArray<FString> Symbols;

		TArray<FString> Lines;
		ZoneC.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		for (const FString& Line : Lines)
		{
			if (!Line.StartsWith(TEXT("- "), ESearchCase::CaseSensitive))
			{
				continue;
			}

			int32 ColonIndex = INDEX_NONE;
			if (!Line.FindChar(TEXT(':'), ColonIndex))
			{
				continue;
			}

			Symbols.Add(Line.Mid(2, ColonIndex - 2));
		}

		return Symbols;
	}

	/**
	 *  The symbols NAMED on the collapse line. `other_kinds: none` yields an empty
	 *  array; `other_kinds: wizard, sorcerer (3 units)` yields [wizard, sorcerer].
	 */
	static TArray<FString> CollapsedSymbols(const FString& ZoneC)
	{
		TArray<FString> Symbols;

		FString Value = ValueOfKey(ZoneC, TEXT("other_kinds"));
		if (Value.IsEmpty() || Value.Equals(TEXT("none"), ESearchCase::CaseSensitive))
		{
			return Symbols;
		}

		// Drop the trailing " (N units)" aggregate; everything before it is the
		// comma-separated symbol list.
		int32 ParenIndex = INDEX_NONE;
		if (Value.FindChar(TEXT('('), ParenIndex))
		{
			Value = Value.Left(ParenIndex).TrimEnd();
		}

		Value.ParseIntoArray(Symbols, TEXT(", "), /*InCullEmpty*/ true);
		return Symbols;
	}

	/**
	 *  A player sentence of exactly N characters — the lever that drives the
	 *  trimmer, because `order:` lives in BuildZoneC's untrimmable Tail.
	 *
	 *  ⚠️ THE SHAPE OF THE STRING IS LOAD-BEARING, NOT COSMETIC. `SanitizeForPrompt`
	 *  drops leading whitespace, COLLAPSES runs of it, and caps on UTF-8 BYTES:
	 *   - ASCII only, so the requested length and the spent budget cannot differ;
	 *   - short words separated by SINGLE spaces, so nothing collapses;
	 *   - ⛔ never a leading or trailing space, or the sanitiser would return
	 *     N-1 characters and every budget figure in these tests would be off by
	 *     one in a direction nothing reports.
	 */
	static FString OrderLineOfLength(int32 Characters)
	{
		FString Out;
		Out.Reserve(Characters);
		while (Out.Len() < Characters)
		{
			Out += (Out.Len() % 5 == 4) ? TEXT(" ") : TEXT("x");
		}

		Out.LeftInline(Characters);

		// The modulo can land a space on the final character; swap it so the
		// sanitiser has nothing to trim.
		if (Out.Len() > 0 && Out[Out.Len() - 1] == TEXT(' '))
		{
			Out[Out.Len() - 1] = TEXT('x');
		}

		return Out;
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  PARSER HELPERS
	// ═══════════════════════════════════════════════════════════════════════════

	/** One well-formed command JSON with an arbitrary `who` payload spliced in, so every test varies exactly one thing. */
	static FString CommandJson(const TCHAR* Intent, const TCHAR* WhoPayload)
	{
		return FString::Printf(
			TEXT("{\"intent\":\"%s\",\"who\":%s,\"where\":\"mid\",\"when\":\"now\"}"),
			Intent, WhoPayload);
	}

	/** The bare reason CODE, with any ':detail' payload stripped — what the FSM switches on. */
	static FString CodeOf(const FString& Reason)
	{
		return SiegeAssistantReasonCode(Reason);
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-549 ADDITIONS — THE AI-COMMANDER ROBUSTNESS BATCH (CONVENTIONS
	//      AS-§21). Everything below serves the FIFTH `who` shape, {"in":ZONE}.
	// ═══════════════════════════════════════════════════════════════════════════
	//
	//  ⛔ THE ORDER THESE TESTS ARE WRITTEN IN IS THE ORDER THEY MATTER IN, AND
	//  ADDITIVITY IS FIRST. AS-§21.5's central claim is that the new shape is
	//  STRICTLY ADDITIVE AT THE WIRE: "every JSON that parses today parses
	//  BYTE-IDENTICALLY after". If that is false the batch is not a feature, it is
	//  a regression with a feature attached — so it is asserted before anything
	//  about the feature itself.
	//
	//  ⚠️ WHAT THIS FILE STILL CANNOT REACH, RESTATED FOR THE NEW SURFACE (the
	//  header's list, extended rather than replaced):
	//   - THE EXECUTOR'S REGION FILTER. `SelectUnitsForOrder` walks a `UWorld` for
	//     `ASummonedUnit` actors; these are EditorContext SIMPLE tests. The
	//     PREDICATE it calls is pure and IS tested here (RegionMembershipContract),
	//     and so is the GEOMETRY LOOKUP it feeds from
	//     (RegionResolvesFromSnapshotGeometry) — but the loop that joins them is
	//     TASK-548's five log lines plus Jonathan at TASK-552. ⛔ Not a test, and
	//     this file does not pretend otherwise.
	//   - `USiegeAssistantComponent::ComposeTurnGrammar`. It is PRIVATE and reads
	//     the component's `Snapshot` member, which only `CaptureTurnSnapshot()`
	//     fills, and that needs a world and a team. See
	//     RegionGrammarCompositionCarriesRegions for exactly what is measured
	//     INSTEAD and for the residual that measurement leaves open.

	/** The three region-bearing places of AS-§21.4, in `PlaceVocabulary` table order. ⛔ The ORDER is the caller's contract: `USiegeAssistantGrammar::Build` preserves it and never sorts. */
	static TArray<FName> ThreeRegionPlaces()
	{
		return TArray<FName>{ TEXT("mid"), TEXT("ancient_ground_near"), TEXT("ancient_ground_far") };
	}

	/** One well-formed command JSON carrying the fifth `who` shape. */
	static FString RegionJson(const TCHAR* Intent, const TCHAR* RegionPayload)
	{
		return FString::Printf(
			TEXT("{\"intent\":\"%s\",\"who\":{\"in\":%s},\"where\":\"mid\",\"when\":\"now\"}"),
			Intent, RegionPayload);
	}

	/** The ':detail' payload of a reason string, or empty when there is none. */
	static FString PayloadOf(const FString& Reason)
	{
		int32 SeparatorIndex = INDEX_NONE;
		return Reason.FindChar(TEXT(':'), SeparatorIndex) ? Reason.RightChop(SeparatorIndex + 1) : FString();
	}

	/**
	 *  ⭐ EVERY FIELD OF A PARSED COMMAND, RENDERED INTO ONE COMPARABLE STRING.
	 *
	 *  ⛔ THIS IS WHAT MAKES "PARSES BYTE-IDENTICALLY" AN ASSERTION RATHER THAN A
	 *  PHRASE. Checking `bParsed` and one or two fields is how an additivity claim
	 *  passes while a SEVENTH FIELD quietly acquires a non-default value on an input
	 *  that never mentioned it — and `RegionPlace` is exactly that seventh field.
	 *  Every member of `FSiegeAssistantCommand` appears here, so a new default, a
	 *  new required field or a changed one fails with the whole struct printed.
	 *
	 *  ⚠️ IF A FUTURE BATCH ADDS AN EIGHTH FIELD THIS FUNCTION MUST GROW A TERM.
	 *  It will not fail on its own if it does not — which is the one weakness of a
	 *  digest, and it is written down rather than left to be discovered.
	 */
	static FString CommandDigest(const FSiegeAssistantCommand& Command)
	{
		FString Out;
		Out += FString::Printf(TEXT("intent=%s|"), *SiegeAssistantIntentToSymbol(Command.Intent));

		Out += TEXT("kinds=[");
		for (int32 Index = 0; Index < Command.Kinds.Num(); ++Index)
		{
			Out += (Index > 0 ? TEXT(",") : TEXT("")) + Command.Kinds[Index].ToString();
		}
		Out += TEXT("]|counts=[");
		for (int32 Index = 0; Index < Command.Counts.Num(); ++Index)
		{
			Out += FString::Printf(TEXT("%s%d"), Index > 0 ? TEXT(",") : TEXT(""), Command.Counts[Index]);
		}
		Out += TEXT("]|except=[");
		for (int32 Index = 0; Index < Command.ExcludeKinds.Num(); ++Index)
		{
			Out += (Index > 0 ? TEXT(",") : TEXT("")) + Command.ExcludeKinds[Index].ToString();
		}
		Out += FString::Printf(TEXT("]|region=%s|where=%s|trigger=%s/%d"),
			*Command.RegionPlace.ToString(), *Command.Where.ToString(),
			*Command.TriggerKind.ToString(), Command.TriggerAtLeast);

		return Out;
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  GRAMMAR READERS — file-scope so the new region tests share ONE reader with
	//  each other. ⚠️ TEST 12's identical lambdas are left EXACTLY where they are:
	//  hoisting them would be a diff in a shipped, passing test for no behaviour,
	//  and this batch's safety argument is that its diff is additive.
	// ═══════════════════════════════════════════════════════════════════════════

	/** The right-hand side of `<RuleName> ::= …`, or empty when the rule is not emitted at all. ⛔ Case-sensitive: rule names are bytes llama.cpp parses. */
	static FString GrammarRuleRhs(const FString& Grammar, const TCHAR* RuleName)
	{
		TArray<FString> Lines;
		Grammar.ParseIntoArrayLines(Lines, /*bCullEmpty*/ true);

		const FString Prefix = FString(RuleName) + TEXT(" ::= ");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return Line.RightChop(Prefix.Len());
			}
		}
		return FString();
	}

	/** The ` | `-separated alternatives of a right-hand side, in emitted order. */
	static TArray<FString> GrammarAlternatives(const FString& Rhs)
	{
		TArray<FString> Out;
		Rhs.ParseIntoArray(Out, TEXT(" | "), /*InCullEmpty*/ true);
		return Out;
	}

	/**
	 *  llama.cpp's own rule-name charset, reproduced: `[a-zA-Z0-9-]`, and NOTHING
	 *  else. ⛔ THIS IS LAW ZERO AND IT IS REPRODUCED HERE RATHER THAN IMPORTED
	 *  because `SiegeAssistantGrammarTest.cpp`'s copy is file-static in another
	 *  translation unit. `at_least` shipped once and cost TASK-413 two of its six
	 *  bars: llama.cpp read the name `at`, demanded `::=`, found `_least`, and
	 *  REJECTED THE WHOLE GRAMMAR — after which generation runs UNCONSTRAINED and
	 *  every other property anyone asserts is true of a string the sampler never
	 *  loaded.
	 */
	static bool IsLegalGbnfRuleNameHere(const FString& Identifier)
	{
		if (Identifier.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Char : Identifier)
		{
			const bool bLegal =
				(Char >= TEXT('a') && Char <= TEXT('z')) ||
				(Char >= TEXT('A') && Char <= TEXT('Z')) ||
				(Char >= TEXT('0') && Char <= TEXT('9')) ||
				Char == TEXT('-');

			if (!bLegal)
			{
				return false;
			}
		}
		return true;
	}

	// ═══════════════════════════════════════════════════════════════════════════
	//  REFLECTION WRITES FOR THE SNAPSHOT'S *GEOMETRY* ARRAYS (TASK-547's members)
	// ═══════════════════════════════════════════════════════════════════════════

	/** A `TArray<FVector>` member. The inner STRUCT is checked, not merely that it is a struct — a retype to FVector3f would otherwise reinterpret memory. */
	static TArray<FVector>* FindVectorArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner)
		{
			return nullptr;
		}

		FStructProperty* const Inner = CastField<FStructProperty>(ArrayProperty->Inner);
		if (!Inner || Inner->Struct != TBaseStructure<FVector>::Get())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<FVector>>(Object);
	}

	/** A `TArray<FVector2D>` member, checked the same way. */
	static TArray<FVector2D>* FindVector2DArrayField(UObject* Object, const TCHAR* FieldName)
	{
		FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner)
		{
			return nullptr;
		}

		FStructProperty* const Inner = CastField<FStructProperty>(ArrayProperty->Inner);
		if (!Inner || Inner->Struct != TBaseStructure<FVector2D>::Get())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<FVector2D>>(Object);
	}

	/**
	 *  ⭐ THE GEOMETRY `Capture()` WOULD HAVE PUBLISHED, WRITTEN STRAIGHT INTO THE
	 *  SAME PRIVATE FIELDS — the file header's technique, extended to TASK-547's
	 *  three new members.
	 *
	 *  ⚠️ THE NUMBERS ARE DELIBERATELY NOT THE SHIPPED ONES, AND THAT IS THE POINT
	 *  OF THE WHOLE FIXTURE. Both shipped regions are SQUARE `(840, 840)`
	 *  (`AncientGround.h`, `CaptureZone.h`), so a transposed-axis implementation —
	 *  `Point.X` compared against `HalfExtent.Y` — answers IDENTICALLY on every
	 *  square box and THE SHIPPED DATA STRUCTURALLY CANNOT CATCH IT (TASK-544's own
	 *  handoff, §8 item 2, asks for exactly this). Every extent here is NON-SQUARE
	 *  and every centre is off-origin, so an axis swap and a dropped centre are both
	 *  observable.
	 *
	 *  ⚠️ THE CENTRES AND EXTENTS ARE ALSO CHOSEN TO BE UNMISTAKABLE IN A PROMPT
	 *  DUMP (91234 / 75319 / 1337 / 8642). Zone C is asserted not to contain any of
	 *  them, which is the coordinate airlock (AS-§3) checked as a byte property
	 *  rather than restated as a promise.
	 *
	 *  @param bPublishRegions  when false, the three region members are left EMPTY —
	 *                          the legal "map with no capture zone and no ancient
	 *                          ground" state, and the control for the Zone C
	 *                          byte-equality comparison.
	 */
	static FScratchSnapshot MakeSnapshotWithRegions(const TArray<FName>& Kinds, int32 PerKindTotal, bool bPublishRegions)
	{
		FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, PerKindTotal);
		if (!Scratch.IsUsable())
		{
			return Scratch;
		}

		USiegeAssistantSnapshot* const Object = Scratch.Snapshot.Get();

		TArray<FVector>* const Locations = FindVectorArrayField(Object, TEXT("PlaceLocations"));
		TArray<FVector2D>* const HalfExtents = FindVector2DArrayField(Object, TEXT("PlaceHalfExtents"));
		TArray<FName>* const Regions = FindNameArrayField(Object, TEXT("RegionPlaceNames"));

		if (!Locations)   { Scratch.MissingField = TEXT("PlaceLocations (TArray<FVector>)"); return Scratch; }
		if (!HalfExtents) { Scratch.MissingField = TEXT("PlaceHalfExtents (TArray<FVector2D>)"); return Scratch; }
		if (!Regions)     { Scratch.MissingField = TEXT("RegionPlaceNames (TArray<FName>)"); return Scratch; }

		Locations->Reset();
		HalfExtents->Reset();
		Regions->Reset();

		if (!bPublishRegions)
		{
			// ⛔ NOT A SHORTCUT — this is the CONTROL, and it is a legal shipped
			// state. `Capture()` publishes nothing here either: PlaceNames stays
			// populated (the vocabulary is fixed) while the geometry arrays stay
			// empty, which is exactly what a map with no region actors produces.
			return Scratch;
		}

		// Parallel to SevenPlaces(), filled at ONE site in ONE pass — the invariant
		// ResolvePlaceRegion's IsValidIndex pair defends.
		const TArray<FName> Places = SevenPlaces();
		for (int32 Index = 0; Index < Places.Num(); ++Index)
		{
			Locations->Add(FVector(91234.0 + Index, -75319.0 - Index, 4242.0));
			HalfExtents->Add(FVector2D::ZeroVector);
		}

		// The three region-bearing rows get REAL, NON-SQUARE boxes; the other four
		// keep the zero extent Capture() leaves them at, which is why
		// RegionPlaceNames — and not "is this extent non-zero" — is what
		// ResolvePlaceRegion asks first.
		const TArray<FName> RegionNames = ThreeRegionPlaces();
		for (const FName& Region : RegionNames)
		{
			const int32 Index = Places.IndexOfByKey(Region);
			if (Index != INDEX_NONE)
			{
				(*HalfExtents)[Index] = FVector2D(1337.0, 8642.0);
				Regions->Add(Region);
			}
		}

		return Scratch;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 — ⭐ Siegebound.Assistant.Selection.RosterShowsAllThirteenKinds
//
//  THE TEST JONATHAN'S FIRST COMPLAINT MAPS ONTO, AND THE ONE THE CORPUS CANNOT
//  WRITE. Before it, nothing anywhere asserted that a board holding a Sorcerer
//  shows the model the token `sorcerer`.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRosterShowsAllThirteenKindsTest,
	"Siegebound.Assistant.Selection.RosterShowsAllThirteenKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRosterShowsAllThirteenKindsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ⚠️ THE COLLAPSE WARNING IS EXPECTED TRAFFIC ON THE HEADROOM SUB-CASE BELOW,
	// NOT A FAILURE. `BuildZoneC` reports every degradation at Warning by design
	// (the TASK-419 WARN-5 visibility latch — a >MaxRosterKinds board used to
	// degrade the prompt silently forever). ⛔ Occurrences < 0 means "silently
	// ignore", chosen over 0 ("must be seen") because THIS test's main path
	// deliberately does NOT collapse. ⛔ It is scoped to this one message: a
	// blanket warning suppression would hide the two player-text truncation
	// latches as well.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();

	// The cap is HIS ruling (AS-§20, ruling 1) and half the fix. Asserted here
	// because a silent revert to 8 would make every other assertion in this file
	// pass while the defect came back.
	TestEqual(TEXT("MaxRosterKinds is 13 — Jonathan's ruling 1, the D2 escalation he spent by name"),
		USiegeAssistantSnapshot::MaxRosterKinds, 13);
	TestEqual(TEXT("The fixture board holds all 13 commandable DT_Cards kinds"), Kinds.Num(), 13);
	TestEqualSensitive(TEXT("`sorcerer` is the LAST kind in DT_Cards row order — the mechanism of the defect"),
		Kinds.Last().ToString(), FString(SorcererSymbol));

	// PerKindTotal 9 — AS-§20.3's own single-digit operating point. See
	// MakeSnapshotWithRoster's comment for why the magnitude is load-bearing.
	FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 9);
	if (!TestTrue(*FString::Printf(
			TEXT("The snapshot's Capture()-owned roster fields were reachable by reflection (missing: %s). ⛔ If this fails the field was RENAMED or RETYPED and every collapse assertion in this file would otherwise pass VACUOUSLY on an empty roster."),
			*Scratch.MissingField),
		Scratch.IsUsable()))
	{
		return false;
	}

	// A short, ordinary sentence: the operating point a player actually types.
	const FString ZoneC = Scratch.Snapshot->BuildZoneC(TEXT("send all units to the middle"), FString());

	AddInfo(FString::Printf(TEXT("Zone C on a 13-kind board with a 28-char order: %d chars (SnapshotTrimBudgetChars %d, ZoneBCharReserve %d ⇒ Zone C budget %d)."),
		ZoneC.Len(), USiegeAssistantSnapshot::SnapshotTrimBudgetChars, USiegeAssistantSnapshot::ZoneBCharReserve,
		USiegeAssistantSnapshot::SnapshotTrimBudgetChars - USiegeAssistantSnapshot::ZoneBCharReserve));

	// ⚠️ THE HEADROOM, MEASURED AND REPORTED RATHER THAN ASSERTED. AS-§20.3
	// records ~6 chars at the shipped 61-char `order:` line, and the number moves
	// with the board. Asserting it would make TASK-528's ZoneBCharReserve repair
	// FAIL this file for succeeding, so it is instrumentation: the figure lands in
	// the run log where the next reader of the risk table can compare it.
	{
		FScratchSnapshot Shipped = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 9);
		FScratchSnapshot Realistic = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 12);
		if (Shipped.IsUsable() && Realistic.IsUsable())
		{
			// 61 chars: the DEFAULT `order:` line the shipped 887-of-893 figure was
			// derived on.
			const FString DefaultOrder = OrderLineOfLength(61);
			const FString ShippedZoneC = Shipped.Snapshot->BuildZoneC(DefaultOrder, FString());
			const FString RealisticZoneC = Realistic.Snapshot->BuildZoneC(DefaultOrder, FString());

			AddInfo(FString::Printf(
				TEXT("HEADROOM READING at the shipped 61-char `order:` line — single-digit counts: Zone C %d chars, %d row(s) printed, %d collapsed. TWO-digit counts (an ordinary mid-match board): Zone C %d chars, %d row(s) printed, %d collapsed. ⚠️ Every roster row carries the count THREE times, so two-digit counts widen the 13-kind block by ~39 chars against a ~6-char headroom."),
				ShippedZoneC.Len(), PrintedRosterSymbols(ShippedZoneC).Num(), CollapsedSymbols(ShippedZoneC).Num(),
				RealisticZoneC.Len(), PrintedRosterSymbols(RealisticZoneC).Num(), CollapsedSymbols(RealisticZoneC).Num()));

			// ⭐ AND THE INVARIANT HOLDS EITHER WAY — which is the whole point of
			// TASK-517's names fix, and the reason the reading above is a note and
			// not a failure.
			TestTrue(TEXT("⭐ `sorcerer` is visible on a two-digit board too, collapsed or not — the names fix is what makes the headroom survivable"),
				RealisticZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));
		}
	}

	// ⭐⭐ THE CLAIM. Case-sensitive because it is a claim about the BYTES the
	// tokenizer sees: `Sorcerer` and `sorcerer` are different tokens, and Zone A
	// teaches the model lower-case symbols.
	TestTrue(TEXT("⭐ Zone C CONTAINS the literal `sorcerer` on a board where a Sorcerer is alive — the token that never reached the model before TASK-517"),
		ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));

	const TArray<FString> Printed = PrintedRosterSymbols(ZoneC);
	TestEqual(TEXT("All 13 kinds print as FULL roster rows at this operating point"), Printed.Num(), 13);

	// Row-by-row, in order — a set comparison would pass on a roster that printed
	// the right symbols in the wrong order, and the ORDER is what makes the
	// collapse deterministic.
	for (int32 Index = 0; Index < Kinds.Num() && Index < Printed.Num(); ++Index)
	{
		TestEqualSensitive(*FString::Printf(TEXT("Roster row %d is the DT_Cards row-order symbol"), Index),
			Printed[Index], Kinds[Index].ToString());
	}

	TestEqualSensitive(TEXT("Nothing collapsed, so `other_kinds:` reads exactly `none`"),
		ValueOfKey(ZoneC, TEXT("other_kinds")), FString(TEXT("none")));

	// The whole row, not just the symbol — this is what proves the model is given
	// the Sorcerer's NUMBERS as well as its name at the uncollapsed operating
	// point, which is the difference between an executable order and a
	// clarification.
	TestTrue(TEXT("The Sorcerer's full roster row is present with its counts"),
		ZoneC.Contains(TEXT("- sorcerer: 9 total, 9 orderable, 9 followable"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 — ⭐⭐ Siegebound.Assistant.Selection.CollapseNamesTheHiddenKinds
//
//  THE HIGHEST-VALUE TEST IN THE BATCH, AND THE ONE NO OTHER INSTRUMENT REACHES.
//  TASK-524's finding F-5: the eval lane's fixture prints all 13 kinds
//  unconditionally, so the collapse has NEVER happened there and a green eval row
//  says nothing about this leg. AS-§20.2: at 13 kinds the roster measures ~887 of
//  ~893, so roughly SEVEN extra typed characters re-collapse the tail — and the
//  tail is the Sorcerer.
//
//  ⛔ THE ASSERTION IS ON THE NAME, NOT THE COUNT. The shipped defect had the
//  right count (`other_kinds: 5 kinds, 9 units`) and no name.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionCollapseNamesTheHiddenKindsTest,
	"Siegebound.Assistant.Selection.CollapseNamesTheHiddenKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionCollapseNamesTheHiddenKindsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ⭐ Occurrences == 0 means "one or more, no upper limit" — so this line is not
	// merely a suppression, it is an ASSERTION that the shipped visibility latch
	// FIRED. A collapse that degrades the prompt without saying so is the TASK-419
	// WARN-5 defect (a >MaxRosterKinds board degrading silently forever), and it
	// would otherwise be invisible to this test: `other_kinds:` would still read
	// correctly while the developer-facing half of the contract had gone missing.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 0);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();

	FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, /*PerKindTotal*/ 9);
	if (!TestTrue(*FString::Printf(TEXT("The roster fields were reachable by reflection (missing: %s)"), *Scratch.MissingField),
		Scratch.IsUsable()))
	{
		return false;
	}

	// ⚠️ THE LEVER IS THE PLAYER'S OWN SENTENCE, WHICH IS THE POINT. `order:` and
	// `pending:` live in BuildZoneC's Tail, are subtracted from the roster budget
	// BEFORE the trimmer runs, and are never themselves trimmed by it (the
	// CONVENTIONS §8 rule that the utterance is never truncated by the snapshot
	// budget). So a long sentence is exactly how a real player forces this path —
	// which is why "it only breaks when you type a lot" was so hard to see.
	const FString LongOrder = OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes);
	const FString LongPending = OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes);

	const FString ZoneC = Scratch.Snapshot->BuildZoneC(LongOrder, LongPending);

	const TArray<FString> Printed = PrintedRosterSymbols(ZoneC);
	const TArray<FString> Collapsed = CollapsedSymbols(ZoneC);

	AddInfo(FString::Printf(TEXT("Forced collapse: %d kind(s) printed in full, %d named on `other_kinds:`. Zone C is %d chars. Collapse line: `other_kinds: %s`."),
		Printed.Num(), Collapsed.Num(), ZoneC.Len(), *ValueOfKey(ZoneC, TEXT("other_kinds"))));

	// ⛔ THE PRE-CONDITION IS ASSERTED, NOT ASSUMED. If the budget arithmetic ever
	// moves so far that a 240-byte order no longer collapses anything, this test
	// would pass on the uncollapsed path while claiming to have measured the
	// collapse — a green bar for a case that never ran. That is the failure shape
	// AS-§12g names, so it is refused here explicitly.
	if (!TestTrue(TEXT("⛔ PRE-CONDITION: a 240-byte order + a 240-byte pending line DOES force the trimmer to collapse at least one kind. If this fails the test below proves NOTHING and must not be read as a pass."),
		Collapsed.Num() > 0))
	{
		return false;
	}

	TestTrue(TEXT("The collapse is genuinely partial — some kinds still print in full"), Printed.Num() > 0);

	// ⭐⭐ THE DURABLE HALF OF THE FIX (AS-§20.2): a collapse may hide a kind's
	// NUMBERS; it may never hide its NAME. This closes the class at ANY cap and
	// ANY board size, which is why it — and not MaxRosterKinds = 13 — is the
	// root-cause repair.
	TestTrue(TEXT("⭐ `sorcerer` appears SOMEWHERE in Zone C even though it was collapsed — the name survives what the numbers do not"),
		ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));

	TestTrue(TEXT("⭐⭐ `sorcerer` is named BY NAME on the `other_kinds:` line, not merely counted"),
		Collapsed.Contains(FString(SorcererSymbol)));

	// The Sorcerer is last in card-row order and the trimmer drops from the tail,
	// so it must be the LAST symbol on the collapse line: the line reproduces the
	// roster's own order rather than an arbitrary one.
	TestEqualSensitive(TEXT("The collapse line preserves DT_Cards row order — `sorcerer`, the last row, is last"),
		Collapsed.Last(), FString(SorcererSymbol));

	// ⛔ EVERY collapsed kind, not just the one the bug was reported on. A fix that
	// named only the newest kind would pass the assertion above and still hide
	// `wizard`, `ogre`, `cleric`… on the next board.
	for (int32 Index = Printed.Num(); Index < Kinds.Num(); ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Collapsed kind `%s` is NAMED on the `other_kinds:` line"), *Kinds[Index].ToString()),
			Collapsed.Contains(Kinds[Index].ToString()));
	}

	TestEqual(TEXT("Printed rows + named collapsed kinds account for the WHOLE roster — nothing vanished"),
		Printed.Num() + Collapsed.Num(), Kinds.Num());

	// ⛔ THE REGRESSION GUARD ON THE OLD FORMAT. `other_kinds: 5 kinds, 9 units`
	// is the string that shipped, and it is the defect. Asserting its ABSENCE means
	// a revert to counts-only fails here even if some future format still happens
	// to contain the substring `sorcerer` elsewhere in the block.
	TestFalse(TEXT("⛔ The retired counts-only format (`N kinds, M units`) is GONE — that string IS the defect"),
		ZoneC.Contains(TEXT(" kinds, "), ESearchCase::CaseSensitive));

	// The aggregate is still carried, in the pinned `(<N> units)` shape. `(1 units)`
	// on a single collapsed kind is deliberate (AS-§20.2) — a pluralisation branch
	// would spend characters out of a ~6-char headroom.
	TestTrue(TEXT("The collapse line still carries the aggregate unit count in the pinned `(<N> units)` shape"),
		ValueOfKey(ZoneC, TEXT("other_kinds")).EndsWith(TEXT(" units)"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 — Siegebound.Assistant.Selection.FixedKeyOrderSurvives
//
//  The fixed-key law: the KEYS never vary, because the executor and the FSM parse
//  nothing in Zone C and a missing key would teach the model that a key is
//  OPTIONAL. TASK-517 changed the VALUE of `other_kinds:`; this asserts it did
//  not change the key set or their order.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionFixedKeyOrderSurvivesTest,
	"Siegebound.Assistant.Selection.FixedKeyOrderSurvives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionFixedKeyOrderSurvivesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// Two of the three boards below collapse on purpose; the shipped visibility
	// latch reports that at Warning. Ignored here rather than asserted, because
	// this test's subject is the KEY SET, not the collapse (TASK-523 spec item 2.3).
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	// THREE boards that exercise all three shapes of the block: uncollapsed,
	// collapsed, and empty. The key order must be identical in all three.
	struct FCase
	{
		const TCHAR* Label;
		TArray<FName> Kinds;
		FString Order;
		FString Pending;
	};

	TArray<FCase> Cases;
	Cases.Add({ TEXT("13 kinds, short order (nothing collapses)"), ThirteenKindsInCardRowOrder(), TEXT("send all units to the middle"), FString() });
	Cases.Add({ TEXT("13 kinds, maximal order (the trimmer bites)"), ThirteenKindsInCardRowOrder(),
		OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes), OrderLineOfLength(USiegeAssistantSnapshot::MaxUtteranceBytes) });
	Cases.Add({ TEXT("empty roster (match start / null-world capture)"), TArray<FName>(), TEXT("send everyone"), FString() });

	for (const FCase& Case : Cases)
	{
		FScratchSnapshot Scratch = MakeSnapshotWithRoster(Case.Kinds, /*PerKindTotal*/ 9);
		if (!TestTrue(*FString::Printf(TEXT("[%s] roster fields reachable (missing: %s)"), Case.Label, *Scratch.MissingField), Scratch.IsUsable()))
		{
			continue;
		}

		const FString ZoneC = Scratch.Snapshot->BuildZoneC(Case.Order, Case.Pending);

		// ⛔ ALWAYS EMITTED, INCLUDING ON THE EMPTY ROSTER AND INCLUDING WHEN
		// NOTHING COLLAPSED. This is the assertion TASK-523's spec item (2.3) asks
		// for by name, and it is the one that stops a future "only print
		// other_kinds when something collapsed" tidy-up.
		TestTrue(*FString::Printf(TEXT("[%s] the `other_kinds:` key is emitted"), Case.Label),
			HasKeyLine(ZoneC, TEXT("other_kinds")));

		for (const TCHAR* Key : { TEXT("places"), TEXT("roster"), TEXT("other_kinds"), TEXT("stances"), TEXT("hero"), TEXT("pending"), TEXT("order") })
		{
			TestTrue(*FString::Printf(TEXT("[%s] the `%s:` key is emitted"), Case.Label, Key), HasKeyLine(ZoneC, Key));
		}

		// ORDER, not merely presence — a set check would pass on a Zone C whose
		// keys had been shuffled, and the prompt's shape is what the model learns.
		int32 Cursor = 0;
		bool bOrdered = true;
		FString FirstOutOfOrderKey;
		for (const TCHAR* Key : { TEXT("[FORCES]\n"), TEXT("places:"), TEXT("roster:"), TEXT("other_kinds:"), TEXT("stances:"), TEXT("hero:"), TEXT("pending:"), TEXT("[ORDER]\n"), TEXT("order:") })
		{
			const int32 Found = ZoneC.Find(Key, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
			if (Found == INDEX_NONE)
			{
				bOrdered = false;
				FirstOutOfOrderKey = Key;
				break;
			}
			Cursor = Found + 1;
		}
		TestTrue(*FString::Printf(TEXT("[%s] the fixed key order holds: [FORCES] places roster other_kinds stances hero pending [ORDER] order (first key out of place: `%s`)"),
			Case.Label, *FirstOutOfOrderKey), bOrdered);

		// The empty roster prints the `- none` sentinel and STILL prints the
		// collapse key — the state a null-world or match-start Capture() leaves.
		if (Case.Kinds.Num() == 0)
		{
			TestTrue(*FString::Printf(TEXT("[%s] an empty roster prints the `- none` sentinel"), Case.Label),
				ZoneC.Contains(TEXT("roster:\n- none\n"), ESearchCase::CaseSensitive));
			TestEqualSensitive(*FString::Printf(TEXT("[%s] an empty roster still prints `other_kinds: none`"), Case.Label),
				ValueOfKey(ZoneC, TEXT("other_kinds")), FString(TEXT("none")));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 — Siegebound.Assistant.Selection.ShrinkLoopNeverHidesASymbol
//
//  ⭐ THE INVARIANT AS-§20.2 ACTUALLY CLAIMS, MEASURED ACROSS THE WHOLE SHRINK
//  RANGE RATHER THAN AT ONE POINT: for EVERY sentence length, every kind on the
//  board is visible to the model — as a full row, or by name on the collapse line.
//
//  Test 2 measures one collapse. This measures ALL of them, and it is what would
//  catch a format change that names the collapsed kinds correctly at depth 1 and
//  drops one at depth 9.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionShrinkLoopNeverHidesASymbolTest,
	"Siegebound.Assistant.Selection.ShrinkLoopNeverHidesASymbol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionShrinkLoopNeverHidesASymbolTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// The sweep drives ~124 collapses on purpose (a FRESH snapshot per rung, so
	// each one's Warning latch starts clean). Ignored rather than asserted: Test 2
	// is where the latch itself is the claim.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();

	// BOTH board magnitudes. A roster row prints its count THREE times, so a
	// two-digit board is 39 chars wider across 13 kinds and reaches every rung of
	// the ladder at a SHORTER sentence. Sweeping both means the invariant is
	// measured on the operating point AS-§20.3 recorded AND on the one an
	// ordinary mid-match board actually produces.
	const int32 BoardMagnitudes[] = { 9, 12 };

	int32 DeepestCollapseOverall = 0;

	for (const int32 PerKindTotal : BoardMagnitudes)
	{
		int32 PreviousPrinted = MAX_int32;
		int32 DeepestCollapse = 0;
		int32 FirstCollapseLength = INDEX_NONE;

		// Sweeps the ONE variable a player controls, from a short sentence to the
		// MaxUtteranceBytes cap, in steps small enough to catch every rung of the
		// ladder (AS-§20.2 measures ~7 characters between the first two rungs).
		for (int32 OrderLength = 0; OrderLength <= USiegeAssistantSnapshot::MaxUtteranceBytes; OrderLength += 4)
		{
			FScratchSnapshot Scratch = MakeSnapshotWithRoster(Kinds, PerKindTotal);
			if (!Scratch.IsUsable())
			{
				AddError(FString::Printf(TEXT("The roster fields were not reachable by reflection (missing: %s)."), *Scratch.MissingField));
				return false;
			}

			const FString ZoneC = Scratch.Snapshot->BuildZoneC(OrderLineOfLength(OrderLength), FString());

			const TArray<FString> Printed = PrintedRosterSymbols(ZoneC);
			const TArray<FString> Collapsed = CollapsedSymbols(ZoneC);

			// ⭐ THE INVARIANT. Union == the whole board, at every rung.
			if (Printed.Num() + Collapsed.Num() != Kinds.Num())
			{
				AddError(FString::Printf(
					TEXT("⛔ A SYMBOL VANISHED at order length %d (board of %d per kind): %d printed + %d collapsed != %d kinds. A collapse may hide a kind's NUMBERS; it may NEVER hide its NAME (AS-§20.2). Collapse line: `other_kinds: %s`."),
					OrderLength, PerKindTotal, Printed.Num(), Collapsed.Num(), Kinds.Num(), *ValueOfKey(ZoneC, TEXT("other_kinds"))));
				return false;
			}

			for (const FName& Kind : Kinds)
			{
				const FString Symbol = Kind.ToString();
				if (!Printed.Contains(Symbol) && !Collapsed.Contains(Symbol))
				{
					AddError(FString::Printf(TEXT("⛔ `%s` is INVISIBLE to the model at order length %d (board of %d per kind) — neither a roster row nor a named collapse."),
						*Symbol, OrderLength, PerKindTotal));
					return false;
				}
			}

			// MONOTONIC. A longer sentence can only ever shrink the roster, never
			// grow it: the budget is `TrimBudget - ZoneBReserve - Head - Tail` and
			// the sentence is in the Tail. A non-monotonic step would mean the
			// collapse line's own bytes had begun to fight the loop that produces
			// it — the ONE property TASK-517's new format could have broken, and
			// the reason its handoff argues the symbol "cancels" between the two.
			if (Printed.Num() > PreviousPrinted)
			{
				AddError(FString::Printf(
					TEXT("⛔ THE SHRINK LOOP IS NOT MONOTONIC (board of %d per kind): order length %d prints %d kinds where the shorter sentence printed %d. A longer utterance must never widen the roster."),
					PerKindTotal, OrderLength, Printed.Num(), PreviousPrinted));
				return false;
			}
			PreviousPrinted = Printed.Num();

			if (Collapsed.Num() > 0 && FirstCollapseLength == INDEX_NONE)
			{
				FirstCollapseLength = OrderLength;
			}
			DeepestCollapse = FMath::Max(DeepestCollapse, Collapsed.Num());

			// The Sorcerer, called out by name at every single rung, because it is
			// the kind Jonathan reported and the one the tail always drops first.
			if (!ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive))
			{
				AddError(FString::Printf(TEXT("⛔ `sorcerer` disappeared from Zone C at order length %d (board of %d per kind) — this is Jonathan's reported defect, reproduced."),
					OrderLength, PerKindTotal));
				return false;
			}
		}

		AddInfo(FString::Printf(
			TEXT("Board of %d per kind: swept order lengths 0..%d. First collapse at order length %d. Deepest collapse: %d kind(s) named on `other_kinds:`. Roster narrowed monotonically throughout and every symbol stayed visible."),
			PerKindTotal, USiegeAssistantSnapshot::MaxUtteranceBytes, FirstCollapseLength, DeepestCollapse));

		DeepestCollapseOverall = FMath::Max(DeepestCollapseOverall, DeepestCollapse);
	}

	// ⛔ THE SWEEP MUST ACTUALLY HAVE EXERCISED THE COLLAPSE. Without this the
	// whole test would pass on a build where the trimmer never fires, reporting
	// SAFE about a path that never ran.
	TestTrue(TEXT("⛔ The sweep genuinely reached the collapse path (at least one kind was collapsed at some length) — otherwise nothing above was measured"),
		DeepestCollapseOverall > 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 — Siegebound.Assistant.Selection.ExclusionParses
//
//  The accept side of Jonathan's ruling 3: "all except X (and Y)" is a GENUINE
//  EXECUTABLE ORDER, not a clarify-only fix.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionParsesTest,
	"Siegebound.Assistant.Selection.ExclusionParses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionParsesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── ONE KIND — Jonathan's own example, "(All units except miners)" ──────────
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\"]}")), Command, Error);

		TestTrue(*FString::Printf(TEXT("⭐ `send all units except the miners` parses (error: %s)"), *Error), bParsed);
		TestEqual(TEXT("ExcludeKinds holds exactly the one named kind"), Command.ExcludeKinds.Num(), 1);
		if (Command.ExcludeKinds.Num() == 1)
		{
			TestEqualSensitive(TEXT("The excluded symbol is `miner`"), Command.ExcludeKinds[0].ToString(), FString(TEXT("miner")));
		}

		// ⭐ THE SEAM THE EXECUTOR KEYS ON, AND TASK-524's DEV-26 DEPENDS ON IT:
		// an exclusion is a MODIFIED "all", so Kinds stays EMPTY and the command
		// flows through the executor's existing `Kinds.Num() == 0` branch. ⛔ If
		// the parser ever synthesised an enumeration here instead, the exclusion
		// would silently become a positive selection capped at 3 kinds.
		TestEqual(TEXT("⭐ Kinds stays EMPTY — an exclusion is a modified `all`, never a synthesised enumeration"), Command.Kinds.Num(), 0);
		TestEqual(TEXT("Counts stays empty alongside it"), Command.Counts.Num(), 0);
		TestTrue(TEXT("The intent survives the exclusion shape"), Command.Intent == ESiegeAssistantIntent::Send);
	}

	// ── 1, 2 AND 3 KINDS ALL ACCEPTED, ON EVERY SELECTION-BEARING VERB ─────────
	const TCHAR* const SelectionBearingIntents[] = { TEXT("send"), TEXT("guard"), TEXT("ambush"), TEXT("follow") };
	const TCHAR* const Payloads[] = {
		TEXT("{\"all_except\":[\"miner\"]}"),
		TEXT("{\"all_except\":[\"miner\",\"cleric\"]}"),
		TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\"]}")
	};

	for (const TCHAR* Intent : SelectionBearingIntents)
	{
		for (int32 Arity = 0; Arity < UE_ARRAY_COUNT(Payloads); ++Arity)
		{
			FSiegeAssistantCommand Command;
			FString Error;
			const bool bParsed = ParseSiegeAssistantCommand(CommandJson(Intent, Payloads[Arity]), Command, Error);

			TestTrue(*FString::Printf(TEXT("`%s` accepts a %d-kind exclusion (error: %s)"), Intent, Arity + 1, *Error), bParsed);
			TestEqual(*FString::Printf(TEXT("`%s` keeps all %d excluded kinds"), Intent, Arity + 1), Command.ExcludeKinds.Num(), Arity + 1);
			TestEqual(*FString::Printf(TEXT("`%s` leaves Kinds empty at arity %d"), Intent, Arity + 1), Command.Kinds.Num(), 0);
		}
	}

	// ── THE CAP IS THE CONSTANT, NOT THE LITERAL 3 ─────────────────────────────
	// Widening SiegeAssistantMaxExclusionKinds is a MANAGER RULING (AS-§20.1's
	// ruling-15 precedent). Pinned here so a tuner's edit fails a test instead of
	// slipping through, and pinned against the SELECTION cap it deliberately
	// mirrors.
	TestEqual(TEXT("SiegeAssistantMaxExclusionKinds is 3 — widening it is a manager ruling, not a tuner's edit"),
		SiegeAssistantMaxExclusionKinds, 3);
	TestEqual(TEXT("It mirrors SiegeAssistantMaxSelectionKinds deliberately"),
		SiegeAssistantMaxExclusionKinds, SiegeAssistantMaxSelectionKinds);

	// ── ⛔ ADDITIVITY AT THE WIRE, ASSERTED AS A BEHAVIOUR (SC-§18) ────────────
	// AS-§20.1's central claim is that the new `who` shape is STRICTLY ADDITIVE:
	// "every JSON the model emits today stays valid, byte-for-byte, and nothing
	// that already passes starts failing." A command carrying no exclusion must
	// therefore leave ExcludeKinds empty rather than defaulting to anything.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The pre-existing `who`:\"all\" shape still parses unchanged"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), Command, Error));
		TestEqual(TEXT("…and leaves ExcludeKinds EMPTY (the normal state of every command that excludes nothing)"),
			Command.ExcludeKinds.Num(), 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 — Siegebound.Assistant.Selection.ExclusionArityRefused
//
//  ⚠️ THE EMPTY LIST IS THE INTERESTING HALF. `{"all_except":[]}` is the model
//  saying "everyone except —" and stopping. Quietly PROMOTING that to a plain
//  "all" is how an exception gets dropped without anybody noticing, so it is
//  REFUSED. That is a deliberate ruling, and it is the one a "helpful"
//  simplification would undo first.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionArityRefusedTest,
	"Siegebound.Assistant.Selection.ExclusionArityRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionArityRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	struct FRefusal
	{
		const TCHAR* Label;
		const TCHAR* Payload;
	};

	const FRefusal Refusals[] = {
		{ TEXT("⛔ ZERO kinds — REFUSED, never promoted to a plain `all`"), TEXT("{\"all_except\":[]}") },
		{ TEXT("FOUR kinds — one past the cap"), TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\",\"ogre\"]}") },
		{ TEXT("FIVE kinds"), TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\",\"ogre\",\"wizard\"]}") }
	};

	for (const FRefusal& Refusal : Refusals)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), Refusal.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is refused"), Refusal.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s reports `exclude_arity`"), Refusal.Label),
			CodeOf(Error), FString(SiegeAssistantReason::ExcludeArity));

		// ⛔ NEVER PARTIALLY FILLED. A caller that ignores the return value must be
		// left holding an inert None command, not half an order.
		TestTrue(*FString::Printf(TEXT("%s leaves the command fully reset"), Refusal.Label),
			Command.Intent == ESiegeAssistantIntent::None && Command.ExcludeKinds.Num() == 0 && Command.Kinds.Num() == 0);
	}

	// ⚠️ THE EMPTY-LIST REFUSAL, RESTATED AS THE THING IT ACTUALLY PREVENTS. This
	// assertion is the whole reason the case above is not "harmless": if
	// `{"all_except":[]}` parsed, it would parse to a command IDENTICAL to
	// `who:"all"` — an order that moves the units the player just excluded.
	{
		FSiegeAssistantCommand Empty;
		FString EmptyError;
		const bool bEmptyParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[]}")), Empty, EmptyError);

		FSiegeAssistantCommand All;
		FString AllError;
		const bool bAllParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), All, AllError);

		TestTrue(TEXT("⛔ `{\"all_except\":[]}` must NOT parse to the same command as `who`:\"all\" — promoting it would execute the order the exception was refusing"),
			bAllParsed && !bEmptyParsed);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 — Siegebound.Assistant.Selection.ExclusionConflictRefused
//
//  ⛔ NEVER A MERGE. "Send 10 footmen except the miners" is a confused sentence,
//  and silently picking one half of it to honour is the valid-shaped-wrong-command
//  class this whole design exists to stop.
//
//  ⚠️ AND THE REACHABILITY IS STATED HONESTLY: from JSON the two `who` shapes are
//  DISJOINT (a selection is an Array, an exclusion is an Object), so this state
//  CANNOT be produced by any model output. It is reachable only over the M8 P2
//  wire, which is exactly why `SiegeAssistantValidateSelection` — the receive-side
//  gate — is where this is tested rather than through the parser.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionConflictRefusedTest,
	"Siegebound.Assistant.Selection.ExclusionConflictRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionConflictRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── CASE 1 — A SELECTION AND AN EXCLUSION AT ONCE ──────────────────────────
	{
		const TArray<FName> Kinds{ TEXT("footman") };
		const TArray<int32> Counts{ 10 };
		const TArray<FName> Excludes{ TEXT("miner") };

		FString Error;
		const bool bValid = SiegeAssistantValidateSelection(Kinds, Counts, Error, Excludes);

		TestFalse(TEXT("⛔ A positive selection PLUS an exclusion is refused — never merged"), bValid);
		TestEqualSensitive(TEXT("…and it reports `exclude_conflict`"), CodeOf(Error), FString(SiegeAssistantReason::ExcludeConflict));
	}

	// ── CASE 2 — `who`:"none" WITH AN EXCLUSION ────────────────────────────────
	// ⚠️ STRUCTURALLY UNREACHABLE FROM JSON AND SAID SO RATHER THAN FAKED: the
	// parser sets `bWhoIsNone` only in its String branch and fills ExcludeKinds
	// only in its Object branch, so no single `who` value can produce both. The
	// shipped guard is honestly labelled DEFENSIVE in the source. What IS
	// reachable — and is asserted here — is the pre-existing behaviour it sits
	// beside, unchanged by this batch.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"none\"")), Command, Error);

		TestFalse(TEXT("`who`:\"none\" on a selection-bearing verb is still refused (pre-existing, unchanged)"), bParsed);
		TestEqualSensitive(TEXT("…still with `who_required`, NOT the new `exclude_conflict` — the change is additive"),
			CodeOf(Error), FString(SiegeAssistantReason::WhoRequired));
	}
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("`who`:\"none\" on an ARMY-WIDE verb is still accepted (pre-existing, unchanged)"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("charge"), TEXT("\"none\"")), Command, Error));
	}

	// ── CASE 3 — A REPEATED EXCLUDED SYMBOL ────────────────────────────────────
	// A repeat is not merely redundant: it spends one of only three exclusion
	// slots saying nothing, which means the player's sentence and the order we
	// built have already diverged.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\",\"miner\"]}")), Command, Error);

		TestFalse(TEXT("A repeated excluded kind is refused"), bParsed);
		TestEqualSensitive(TEXT("…reusing the existing `duplicate_kind` code, not a second one"),
			CodeOf(Error), FString(SiegeAssistantReason::DuplicateKind));
	}

	// ⚠️ CASE-INSENSITIVE REPEAT. Symbol VALUES are compared case-insensitively
	// (FName is), so `["miner","MINER"]` is the SAME kind twice. Asserted because
	// a naive string-set implementation would accept it and hand the executor two
	// slots for one kind.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("A repeat that differs only in CASE is still a repeat"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\",\"MINER\"]}")), Command, Error));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 — ⭐ Siegebound.Assistant.Selection.ExclusionRefusedOnArmyWideIntents
//
//  ⛔⛔ THE RULING-3 ASSERTION, AND THE ONE THAT STOPS THE SILENT-DROP FAILURE.
//  charge / fallback / rally execute through
//  `ASiegePlayerController::ApplyArmyWideStance` and `AHeroCharacter::Rally()` —
//  the same shipped APIs the T / E / R keys call — and NONE of them walks the
//  candidate list. An ExcludeKinds handed to them has no code path that could
//  subtract anything, so it would be parsed and then DROPPED IN SILENCE:
//  "fall back except the miners" executing as "fall back INCLUDING the miners".
//  A valid-shaped wrong command that LOOKS obeyed is strictly worse than a
//  refusal, because nothing in the game or the log would contradict it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionRefusedOnArmyWideIntentsTest,
	"Siegebound.Assistant.Selection.ExclusionRefusedOnArmyWideIntents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionRefusedOnArmyWideIntentsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	const TCHAR* const ArmyWideIntents[] = { TEXT("charge"), TEXT("fallback"), TEXT("rally") };

	for (const TCHAR* Intent : ArmyWideIntents)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(Intent, TEXT("{\"all_except\":[\"miner\"]}")), Command, Error);

		TestFalse(*FString::Printf(TEXT("⛔ `%s ... except the miners` is REFUSED — it can never be honoured, so it is never accepted"), Intent), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("`%s` reports `exclude_conflict`"), Intent),
			CodeOf(Error), FString(SiegeAssistantReason::ExcludeConflict));

		// The payload NAMES THE OFFENDING VERB, which is what lets the FSM and the
		// log say which half of the sentence could not be kept.
		TestTrue(*FString::Printf(TEXT("The refusal's payload names `%s`, so the log says WHICH verb could not carry the exception"), Intent),
			Error.Contains(Intent, ESearchCase::CaseSensitive));

		// ⛔ AND THE ORDER IS NOT SILENTLY DEGRADED TO THE UNEXCEPTED ONE. This is
		// the assertion that distinguishes "refused" from "accepted with the
		// exception dropped" — the exact failure this whole feature exists to stop.
		TestTrue(*FString::Printf(TEXT("⛔ `%s` is NOT accepted with the exception quietly discarded — the command is fully reset"), Intent),
			Command.Intent == ESiegeAssistantIntent::None && Command.ExcludeKinds.Num() == 0);
	}

	// ── THE GATE IS THE SHIPPED PREDICATE, NOT A SECOND LIST OF VERBS ──────────
	// Reusing `SiegeAssistantIntentTakesSelection` means there is no parallel
	// army-wide list to drift out of step with the executor seam it describes.
	// Pinned here so a future verb lands on the right side automatically.
	TestTrue(TEXT("Send / Guard / Ambush / Follow take a selection"),
		SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Send)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Guard)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Ambush)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Follow));
	TestFalse(TEXT("Charge / Fallback / Rally do NOT — they never reach the selector"),
		SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Charge)
		|| SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Fallback)
		|| SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Rally));

	// ✅ HIS OWN EXAMPLE IS FULLY SERVED, and this is the control that proves the
	// refusal above is about the VERB and not about exclusions in general.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("✅ The same exception on `send` — Jonathan's own sentence — is ACCEPTED"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"miner\"]}")), Command, Error));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 9 — Siegebound.Assistant.Selection.ExclusionSymbolsRefused
//
//  The malformed-payload paths, by reason code and by name. Symbol validation
//  REUSES `ParseKindSymbol` — ⛔ no second kind-validation path — so these also
//  assert that the exclusion list did not grow one.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionExclusionSymbolsRefusedTest,
	"Siegebound.Assistant.Selection.ExclusionSymbolsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionExclusionSymbolsRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	struct FCase
	{
		const TCHAR* Label;
		const TCHAR* Payload;
		const TCHAR* ExpectedCode;
	};

	const FCase Cases[] = {
		// The list is not an array at all.
		{ TEXT("`all_except` holding a bare string"), TEXT("{\"all_except\":\"miner\"}"), SiegeAssistantReason::BadType },
		{ TEXT("`all_except` holding an object"),     TEXT("{\"all_except\":{\"kind\":\"miner\"}}"), SiegeAssistantReason::BadType },

		// ⛔ THE DECLINED FEATURE, REFUSED AT THE PARSER TOO. Jonathan declined
		// "all except 5 archers"; the grammar makes it unsayable and this makes it
		// unparseable, so neither a prompt tweak nor a hand-written JSON can
		// re-introduce it.
		{ TEXT("⛔ a count-controlled item `{\"kind\":…,\"n\":…}` — the DECLINED variant"),
		  TEXT("{\"all_except\":[{\"kind\":\"archer\",\"n\":5}]}"), SiegeAssistantReason::BadType },

		// Reserved sentinels and empties are not kinds.
		{ TEXT("the reserved sentinel `none` as an excluded kind"), TEXT("{\"all_except\":[\"none\"]}"), SiegeAssistantReason::BadKind },
		{ TEXT("an empty excluded symbol"),                        TEXT("{\"all_except\":[\"\"]}"),     SiegeAssistantReason::BadKind },
		{ TEXT("a whitespace-only excluded symbol"),               TEXT("{\"all_except\":[\"   \"]}"),  SiegeAssistantReason::BadKind },

		// An unknown key inside the exclusion object — the exact-key-set rule.
		{ TEXT("an unknown key beside `all_except`"), TEXT("{\"all_except\":[\"miner\"],\"n\":5}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("a misspelled `all_except`"),          TEXT("{\"allexcept\":[\"miner\"]}"),          SiegeAssistantReason::UnknownKey }
	};

	for (const FCase& Case : Cases)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), Case.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is refused"), Case.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s reports `%s`"), Case.Label, Case.ExpectedCode),
			CodeOf(Error), FString(Case.ExpectedCode));
	}

	// ── SYMBOL CASE IS NORMALISED, NOT REJECTED ────────────────────────────────
	// The grammar only ever emits lower case, but FName is case-insensitive and
	// the parser lower-cases every symbol. Pinned so a future "strict" pass does
	// not start refusing a model that capitalised one letter.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("An excluded symbol in mixed case still parses"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"Miner\"]}")), Command, Error));
		if (Command.ExcludeKinds.Num() == 1)
		{
			TestEqualSensitive(TEXT("…and is stored LOWER-CASE, the canonical wire form"),
				Command.ExcludeKinds[0].ToString(), FString(TEXT("miner")));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 10 — Siegebound.Assistant.Selection.PreExistingWhoShapesUnchanged
//
//  ⭐ THE ADDITIVITY CLAIM, MEASURED. AS-§20.1: "every JSON the model emits today
//  stays valid, byte-for-byte, and nothing that already passes starts failing."
//  SC-§18: "additive" is a claim about BEHAVIOUR, not about diff arithmetic — so
//  it is asserted as behaviour, on the three PRE-EXISTING `who` shapes and the
//  three PRE-EXISTING refusal codes the new branch sits beside.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionPreExistingWhoShapesUnchangedTest,
	"Siegebound.Assistant.Selection.PreExistingWhoShapesUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionPreExistingWhoShapesUnchangedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── THE THREE PRE-EXISTING SHAPES STILL PARSE, IDENTICALLY ─────────────────
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The 1-item selection array still parses"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("[{\"kind\":\"footman\",\"n\":10}]")), Command, Error));
		TestEqual(TEXT("…with its kind"), Command.Kinds.Num(), 1);
		TestEqual(TEXT("…and its count"), Command.Counts.Num() == 1 ? Command.Counts[0] : -1, 10);
		TestEqual(TEXT("…and NO exclusion appears from nowhere"), Command.ExcludeKinds.Num(), 0);
	}
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("The 3-item selection array — the cap — still parses"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"),
				TEXT("[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1},{\"kind\":\"archer\",\"n\":\"all\"}]")), Command, Error));
		TestEqual(TEXT("…with all three kinds"), Command.Kinds.Num(), 3);
		TestEqual(TEXT("…and `all` still maps to count 0"), Command.Counts.Num() == 3 ? Command.Counts[2] : -1, 0);
	}
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("`who`:\"all\" still parses to an EMPTY selection"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), Command, Error));
		TestEqual(TEXT("…Kinds empty"), Command.Kinds.Num(), 0);
		TestEqual(TEXT("…ExcludeKinds empty"), Command.ExcludeKinds.Num(), 0);
	}

	// ── THE THREE PRE-EXISTING REFUSALS STILL REPORT THEIR OWN CODES ───────────
	// ⛔ NOT `exclude_arity` OR `exclude_conflict`. If the new branch had widened
	// one of these, an FSM switching on the code would route a familiar failure to
	// a new, wrong clarification — a regression with no compiler diagnostic.
	struct FCase
	{
		const TCHAR* Label;
		const TCHAR* Intent;
		const TCHAR* Payload;
		const TCHAR* ExpectedCode;
	};

	const FCase Cases[] = {
		{ TEXT("`who` = a string that is neither `all` nor `none`"), TEXT("send"), TEXT("\"everyone\""),
		  SiegeAssistantReason::BadWho },
		{ TEXT("`who` = an EMPTY selection array"), TEXT("send"), TEXT("[]"),
		  SiegeAssistantReason::WhoArity },
		{ TEXT("`who` = a 4-item selection array (one past the cap)"), TEXT("send"),
		  TEXT("[{\"kind\":\"footman\",\"n\":1},{\"kind\":\"archer\",\"n\":1},{\"kind\":\"cleric\",\"n\":1},{\"kind\":\"ogre\",\"n\":1}]"),
		  SiegeAssistantReason::WhoArity },
		{ TEXT("`who` = \"none\" on a selection-bearing verb"), TEXT("send"), TEXT("\"none\""),
		  SiegeAssistantReason::WhoRequired },
		{ TEXT("a repeated kind inside a positive selection"), TEXT("send"),
		  TEXT("[{\"kind\":\"footman\",\"n\":1},{\"kind\":\"footman\",\"n\":2}]"),
		  SiegeAssistantReason::DuplicateKind },
		{ TEXT("`who` = a number"), TEXT("send"), TEXT("7"), SiegeAssistantReason::BadType }
	};

	for (const FCase& Case : Cases)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(Case.Intent, Case.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is still refused"), Case.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s still reports `%s` — NOT one of the new exclusion codes"), Case.Label, Case.ExpectedCode),
			CodeOf(Error), FString(Case.ExpectedCode));
	}

	// ── THE TOP-LEVEL KEY SET DID NOT CHANGE ───────────────────────────────────
	// ⛔ AS-§20.1 rejected a fourth top-level key (`"except":…`) precisely because
	// `ValidateExactKeySet` demands an exact set and the prompt law emits every key
	// always. Asserted so nobody "improves" it back later.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			TEXT("{\"intent\":\"send\",\"who\":\"all\",\"except\":[\"miner\"],\"where\":\"mid\",\"when\":\"now\"}"), Command, Error);

		TestFalse(TEXT("⛔ `except` as a FOURTH TOP-LEVEL KEY is refused — the exclusion is a `who` SHAPE, never a new key"), bParsed);
		TestEqualSensitive(TEXT("…reported as `unknown_key`"), CodeOf(Error), FString(SiegeAssistantReason::UnknownKey));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 11 — ⭐ Siegebound.Assistant.Selection.ValidatorExclusionArgumentIsLoadBearing
//
//  ⛔⛔ THE HIGHEST-RISK LINE IN THE BATCH, PINNED.
//
//  `SiegeAssistantValidateSelection` gained `ExcludeKinds` as a TRAILING DEFAULTED
//  parameter — a shape FORCED by file ownership (a required 4th argument would
//  have broken the compile in two files TASK-518 could not edit), not chosen. The
//  cost: A CALLER THAT OMITS THE ARGUMENT COMPILES CLEANLY AND VALIDATES NOTHING
//  ABOUT EXCLUSION. Removing `Command.ExcludeKinds` from the call in
//  `USiegeAssistantComponent::HandleModelCompletion` would raise NO compiler
//  diagnostic anywhere.
//
//  ⚠️ WHAT THIS TEST HONESTLY IS, AND IS NOT. It CANNOT observe that call site:
//  `HandleModelCompletion` needs a live component, a world and the Thinking state.
//  What it does is MEASURE THE DELTA the argument buys, so the hazard is a red bar
//  the moment anyone changes what the default means, and so the size of what is
//  lost by dropping it is written down as an executable fact rather than as a
//  comment. ⛔ A test whose name is a stronger claim than its comparator is
//  documentation, not a test — so the name says "load-bearing", which is exactly
//  what is measured.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionValidatorExclusionArgumentIsLoadBearingTest,
	"Siegebound.Assistant.Selection.ValidatorExclusionArgumentIsLoadBearing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionValidatorExclusionArgumentIsLoadBearingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// Three commands that are INVALID because of their exclusion alone. Each is a
	// state the M8 P2 wire path can deliver, since a peer passed through nobody's
	// grammar and nobody's parser.
	struct FCase
	{
		const TCHAR* Label;
		TArray<FName> Kinds;
		TArray<int32> Counts;
		TArray<FName> Excludes;
		const TCHAR* ExpectedCode;
	};

	TArray<FCase> Cases;
	Cases.Add({ TEXT("four excluded kinds (one past the cap)"),
		TArray<FName>(), TArray<int32>(),
		TArray<FName>{ TEXT("miner"), TEXT("cleric"), TEXT("sorcerer"), TEXT("ogre") },
		SiegeAssistantReason::ExcludeArity });
	Cases.Add({ TEXT("a repeated excluded kind"),
		TArray<FName>(), TArray<int32>(),
		TArray<FName>{ TEXT("miner"), TEXT("miner") },
		SiegeAssistantReason::DuplicateKind });
	Cases.Add({ TEXT("a selection AND an exclusion at once"),
		TArray<FName>{ TEXT("footman") }, TArray<int32>{ 10 },
		TArray<FName>{ TEXT("miner") },
		SiegeAssistantReason::ExcludeConflict });

	for (const FCase& Case : Cases)
	{
		// (a) THE FOUR-ARGUMENT CALL — the one every caller holding a whole
		//     FSiegeAssistantCommand must make.
		FString CheckedError;
		const bool bCheckedValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, CheckedError, Case.Excludes);

		TestFalse(*FString::Printf(TEXT("[%s] the 4-argument call REFUSES it"), Case.Label), bCheckedValid);
		TestEqualSensitive(*FString::Printf(TEXT("[%s] …reporting `%s`"), Case.Label, Case.ExpectedCode),
			CodeOf(CheckedError), FString(Case.ExpectedCode));

		// (b) THE DEFAULTED THREE-ARGUMENT CALL — what a dropped argument becomes.
		FString UncheckedError;
		const bool bUncheckedValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, UncheckedError);

		// ⛔ THIS IS THE MEASUREMENT. The two calls DISAGREE about the same command,
		// and the disagreement is precisely what the fourth argument buys. If a
		// future edit makes them agree — by making the parameter required, or by
		// giving the default a non-empty value — this assertion fires and the next
		// reader is sent to the call site rather than discovering it in a playtest.
		if (!TestTrue(*FString::Printf(
				TEXT("⛔ [%s] THE DEFAULTED 3-ARGUMENT CALL VALIDATES NOTHING ABOUT EXCLUSION and reports this command VALID. That is why USiegeAssistantComponent::HandleModelCompletion MUST pass Command.ExcludeKinds — dropping it compiles clean and silently turns this refusal into an acceptance."),
				Case.Label),
			bUncheckedValid && !bCheckedValid))
		{
			AddError(FString::Printf(
				TEXT("[%s] The trailing-default hazard has CHANGED SHAPE: 4-arg=%s, 3-arg=%s. Re-read SiegeAssistantValidateSelection's signature and every caller of it before touching anything else."),
				Case.Label, bCheckedValid ? TEXT("valid") : TEXT("refused"), bUncheckedValid ? TEXT("valid") : TEXT("refused")));
		}
	}

	// ── AND THE CONVERSE: THE 4-ARG CALL DID NOT BREAK THE ARRAYS-ONLY CALLERS ──
	// The default exists FOR the arrays-only call sites. A valid selection with no
	// exclusion must validate identically through both forms, or the "strictly
	// additive" claim is false for the callers that never asked for the feature.
	{
		const TArray<FName> Kinds{ TEXT("footman"), TEXT("archer") };
		const TArray<int32> Counts{ 10, 0 };

		FString ThreeArgError;
		FString FourArgError;
		const bool bThree = SiegeAssistantValidateSelection(Kinds, Counts, ThreeArgError);
		const bool bFour = SiegeAssistantValidateSelection(Kinds, Counts, FourArgError, TArray<FName>());

		TestTrue(TEXT("A valid exclusion-free selection passes BOTH call forms identically"), bThree && bFour);
		TestEqualSensitive(TEXT("…with the same (empty) error on both"), ThreeArgError, FourArgError);
	}

	// ── A LEGAL EXCLUSION IS NOT REFUSED BY THE NEW INVARIANTS ─────────────────
	{
		FString Error;
		TestTrue(TEXT("Three excluded kinds against an empty selection is VALID — the cap is inclusive"),
			SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), Error,
				TArray<FName>{ TEXT("miner"), TEXT("cleric"), TEXT("sorcerer") }));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 12 — Siegebound.Assistant.Selection.GrammarAdmitsExceptOnlyWithKinds
//
//  ⚠️ VERIFIED AGAINST THE REAL EMITTER, NOT AGAINST A HANDOFF. TASK-518's
//  handoff carried a HAND-TRANSCRIBED GBNF block, and a transcription is evidence
//  about the transcriber. Every claim below reads `USiegeAssistantGrammar::Build`.
//
//  ⛔ THE RULE NAME IS `exceptlist`, ONE WORD — a DECLARED DEPARTURE (SC-§15)
//  from AS-§20.1's `except_list`, because llama.cpp reads a rule name as
//  [a-zA-Z0-9-] and STOPS at the underscore, which is the defect `at_least`
//  shipped with and which cost TASK-413 two of its six bars. AS-§20.1 named
//  `exceptlist` as the sanctioned substitute, so this is pinned, not invented.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionGrammarAdmitsExceptOnlyWithKindsTest,
	"Siegebound.Assistant.Selection.GrammarAdmitsExceptOnlyWithKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionGrammarAdmitsExceptOnlyWithKindsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	auto RuleRhs = [](const FString& Grammar, const TCHAR* RuleName) -> FString
	{
		TArray<FString> Lines;
		Grammar.ParseIntoArrayLines(Lines, /*bCullEmpty*/ true);

		const FString Prefix = FString(RuleName) + TEXT(" ::= ");
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return Line.RightChop(Prefix.Len());
			}
		}
		return FString();
	};

	auto Alternatives = [](const FString& Rhs) -> TArray<FString>
	{
		TArray<FString> Out;
		Rhs.ParseIntoArray(Out, TEXT(" | "), /*InCullEmpty*/ true);
		return Out;
	};

	auto ReferenceCount = [](const FString& Alternative, const TCHAR* RuleName) -> int32
	{
		TArray<FString> Tokens;
		Alternative.ParseIntoArrayWS(Tokens);

		int32 Total = 0;
		for (const FString& Token : Tokens)
		{
			if (Token.Equals(RuleName, ESearchCase::CaseSensitive))
			{
				++Total;
			}
		}
		return Total;
	};

	const TArray<FName> Places = SevenPlaces();

	// ── A POPULATED ROSTER — the `except` alternative is emitted ───────────────
	{
		const FString Grammar = USiegeAssistantGrammar::Build(ThirteenKindsInCardRowOrder(), Places);

		TestTrue(TEXT("A non-empty roster defines the `except` rule"), !RuleRhs(Grammar, TEXT("except")).IsEmpty());
		TestTrue(TEXT("A non-empty roster defines the `exceptlist` rule"), !RuleRhs(Grammar, TEXT("exceptlist")).IsEmpty());

		// ⛔ THE RULE NAME'S CHARSET IS LAW ZERO. `except_list` would parse as the
		// name `except`, after which llama.cpp rejects the WHOLE grammar and
		// generation runs UNCONSTRAINED — every other property in this file would
		// still be true of a string the sampler never loads.
		TestFalse(TEXT("⛔ The underscore spelling `except_list` is NOT a rule name — it would make the whole grammar unparseable"),
			Grammar.Contains(TEXT("except_list ::="), ESearchCase::CaseSensitive));

		// The JSON KEY keeps its underscore: it is wire format, and it is safe
		// because it lives inside a TERMINAL rather than in identifier position.
		TestTrue(TEXT("The `all_except` JSON key appears (inside a terminal, which is why its underscore is safe)"),
			Grammar.Contains(TEXT("all_except"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("`except` references `exceptlist`"), ReferenceCount(RuleRhs(Grammar, TEXT("except")), TEXT("exceptlist")) == 1);

		// ── `who` NOW OFFERS FOUR ALTERNATIVES ────────────────────────────────
		const FString WhoRule = RuleRhs(Grammar, TEXT("who"));
		TestEqual(TEXT("`who` offers FOUR alternatives: selection | except | \"all\" | \"none\""),
			Alternatives(WhoRule).Num(), 4);
		TestTrue(TEXT("`who` references `selection`"), ReferenceCount(WhoRule, TEXT("selection")) == 1);
		TestTrue(TEXT("`who` references `except`"), ReferenceCount(WhoRule, TEXT("except")) == 1);

		// Order matters: Zone A's `WHO =` line enumerates the shapes in the
		// grammar's own order (selection | except | "all" | "none"), and the two
		// are one contract (AS-§9c). A silent re-ordering here would leave the
		// prompt describing a different grammar than the sampler enforces.
		const TArray<FString> WhoAlternatives = Alternatives(WhoRule);
		if (WhoAlternatives.Num() == 4)
		{
			TestEqualSensitive(TEXT("`who` alternative 1 is `selection`"), WhoAlternatives[0], FString(TEXT("selection")));
			TestEqualSensitive(TEXT("`who` alternative 2 is `except`"), WhoAlternatives[1], FString(TEXT("except")));
		}

		// ── ⛔ THE DECLINED FEATURE, ENFORCED BY SHAPE ────────────────────────
		// `exceptlist` is BARE KIND STRINGS, never `item`. Referencing `item` here
		// would cost nothing to write and would silently re-introduce
		// "all except 5 archers" — the variant Jonathan DECLINED — because
		// {"kind":…,"n":…} would become a shape the sampler could reach.
		const FString ExceptListRule = RuleRhs(Grammar, TEXT("exceptlist"));
		const TArray<FString> ExceptAlternatives = Alternatives(ExceptListRule);

		TestEqual(TEXT("`exceptlist` offers exactly one alternative per permitted arity (1, 2, 3)"),
			ExceptAlternatives.Num(), SiegeAssistantMaxExclusionKinds);

		for (int32 Index = 0; Index < ExceptAlternatives.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("`exceptlist` alternative %d holds %d bare `kind` references"), Index + 1, Index + 1),
				ReferenceCount(ExceptAlternatives[Index], TEXT("kind")), Index + 1);
			TestEqual(*FString::Printf(TEXT("⛔ `exceptlist` alternative %d references `item` ZERO times — no count-controlled shape exists"), Index + 1),
				ReferenceCount(ExceptAlternatives[Index], TEXT("item")), 0);
		}

		TestFalse(TEXT("⛔ There is NO `n` key anywhere in the exclusion rules — the declined feature is INEXPRESSIBLE, not merely unimplemented"),
			ExceptListRule.Contains(TEXT("\\\"n\\\""), ESearchCase::CaseSensitive) || RuleRhs(Grammar, TEXT("except")).Contains(TEXT("\\\"n\\\""), ESearchCase::CaseSensitive));

		// ⛔ BOUNDED ALTERNATION, NEVER A REPETITION OPERATOR. An unbounded
		// repetition is exactly what a small model rambles into.
		TestFalse(TEXT("The grammar contains no '*' repetition operator"), Grammar.Contains(TEXT("*")));
		TestFalse(TEXT("The grammar contains no '+' repetition operator"), Grammar.Contains(TEXT("+")));
		TestFalse(TEXT("The grammar contains no '?' optional operator"), Grammar.Contains(TEXT("?")));

		// Determinism survives the new rules: same state in, byte-identical
		// grammar out. This is what keeps the sampler's constraint stable across
		// turns, and *Sensitive is required because it is a byte claim.
		TestEqualSensitive(TEXT("The grammar is still byte-deterministic with the exclusion rules present"),
			USiegeAssistantGrammar::Build(ThirteenKindsInCardRowOrder(), Places), Grammar);
	}

	// ── A ONE-KIND ROSTER — degradation to REFUSABLE, not to UNREACHABLE ───────
	// At one kind, `exceptlist`'s 2- and 3-kind alternatives can only produce the
	// SAME symbol twice, which the parser refuses with DuplicateKind. That is
	// deliberate and it is `selection`'s own long-standing property; pinned here
	// so nobody "fixes" it by bounding the rule on the live kind count and making
	// the two rules disagree about their own construction.
	{
		const FString Grammar = USiegeAssistantGrammar::Build(TArray<FName>{ TEXT("footman") }, Places);
		TestEqual(TEXT("A one-kind roster still emits all three `exceptlist` arities (bounded by the CAP, not by the board)"),
			Alternatives(RuleRhs(Grammar, TEXT("exceptlist"))).Num(), SiegeAssistantMaxExclusionKinds);

		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("…and the parser refuses the degenerate `[\"footman\",\"footman\"]` it admits"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"all_except\":[\"footman\",\"footman\"]}")), Command, Error));
	}

	// ── AN EMPTY ROSTER — the rule is OMITTED and `who` falls back to two ──────
	// An empty roster has nothing to EXCLUDE for exactly the reason it has nothing
	// to select, and emitting the alternative anyway would leave `exceptlist`
	// referencing an UNDEFINED `kind` rule — which breaks the whole grammar.
	{
		const FString Grammar = USiegeAssistantGrammar::Build(TArray<FName>(), Places);

		TestFalse(TEXT("An empty roster OMITS `except`"), Grammar.Contains(TEXT("except ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("An empty roster OMITS `exceptlist`"), Grammar.Contains(TEXT("exceptlist ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("An empty roster OMITS `kind` (which `exceptlist` would otherwise dangle on)"),
			Grammar.Contains(TEXT("kind ::="), ESearchCase::CaseSensitive));

		TestEqual(TEXT("An empty roster leaves `who` with the two whole-army selectors — UNCHANGED by this batch"),
			Alternatives(RuleRhs(Grammar, TEXT("who"))).Num(), 2);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 13 — Siegebound.Assistant.Selection.PositionalAcceptKeyResolves
//
//  THE CONFIRM UX's ONE HEADLESSLY-REACHABLE SEAM (TASK-516 / TASK-519,
//  AS-§20.5, KBD-§8). `USiegeAssistantConsoleWidget::GetAcceptKey()` is
//  `LayoutSubsystem->GetPositionalKey(EKeys::Z)` with an `EKeys::Z` fallback;
//  the widget's own handler is unreachable from an EditorContext test (see the
//  file header), but the ACCESSOR IT COMPARES AGAINST is not.
//
//  ⛔ THE `EKeys::Invalid` PIN IS THE POINT. TASK-516 flagged that the "obvious
//  simplification" `TranslationMap.FindRef(QwertyKey)` returns a
//  default-constructed FKey on a miss, whose KeyName is NAME_None — and
//  `EKeys::Invalid` IS `FKey(NAME_None)`, compared by KeyName alone. So FindRef
//  returns EXACTLY the one value this function is forbidden to return, ON THE
//  SINGLE MOST COMMON PATH: a QWERTY host, where the map is EMPTY and every
//  lookup misses. A console whose accept key is Invalid accepts nothing, and the
//  player cannot confirm an order.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionPositionalAcceptKeyResolvesTest,
	"Siegebound.Assistant.Selection.PositionalAcceptKeyResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionPositionalAcceptKeyResolvesTest::RunTest(const FString& Parameters)
{
	// ⛔ The test seam announces itself at Warning ON PURPOSE — its own comment says
	// "this line must never appear in a shipped session" — so it is expected
	// traffic here and nowhere else.
	AddExpectedMessagePlain(TEXT("AUTOMATION OVERRIDE"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	// UGameInstanceSubsystem is UCLASS(Within = GameInstance), so the outer is not
	// optional — a bare NewObject trips the ClassWithin check. Initialize() is
	// never called, which is deliberate: no OS probe and no 1 Hz timer run here, so
	// the only translation in play is the one injected below.
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GetTransientPackageAsObject()));
	if (!TestTrue(TEXT("A throwaway UGameInstance was created"), GameInstance.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<USiegeKeyboardLayoutSubsystem> Layout(NewObject<USiegeKeyboardLayoutSubsystem>(GameInstance.Get()));
	if (!TestTrue(TEXT("A keyboard-layout subsystem was created inside it"), Layout.IsValid()))
	{
		return false;
	}

	// ── (a) THE QWERTY HOST — AN EMPTY MAP, WHICH IS THE MISS PATH ─────────────
	Layout->SetTranslationMapForAutomationTests(TMap<FKey, FKey>());
	{
		const FKey Resolved = Layout->GetPositionalKey(EKeys::Z);

		TestTrue(TEXT("⭐ On a QWERTY host GetPositionalKey(Z) is IDENTITY — it returns Z"), Resolved == EKeys::Z);

		// ⛔ THE FORBIDDEN VALUE, ASSERTED BY NAME. This is the assertion that
		// fails the day someone "simplifies" the Find-then-fallback into FindRef.
		TestFalse(TEXT("⛔ It is NEVER EKeys::Invalid — the exact value TMap::FindRef would return here (TASK-516's flagged simplification)"),
			Resolved == EKeys::Invalid);
		TestTrue(TEXT("⛔ The resolved accept key is a VALID FKey"), Resolved.IsValid());

		// ⛔ AND IT IS NEVER `Escape`. AS-§6 A-2 is CLOSED on "leave Escape alone,
		// permanently" and names NativeOnPreviewKeyDown as a way to break it. The
		// handler consumes exactly one key — the one resolved here — so an accept
		// key that resolved to Escape would absorb it and overturn the ruling.
		TestFalse(TEXT("⛔ The accept key is NEVER EKeys::Escape — AS-§6 A-2 is CLOSED and Escape must stay unabsorbed"),
			Resolved == EKeys::Escape);
	}

	// ── (b) A NON-QWERTY HOST — THE HIT PATH, AND THE DIRECTION ───────────────
	// US-Dvorak prints `;` at the physical position QWERTY prints `Z` on, so the
	// map is SOURCE (QWERTY) -> what the ACTIVE layout yields THERE.
	// ⛔ Getting the direction backwards compiles and silently binds the wrong key.
	{
		TMap<FKey, FKey> Dvorak;
		Dvorak.Add(EKeys::Z, EKeys::Semicolon);
		Dvorak.Add(EKeys::W, EKeys::Comma);
		Layout->SetTranslationMapForAutomationTests(Dvorak);

		const FKey Resolved = Layout->GetPositionalKey(EKeys::Z);

		TestTrue(TEXT("⭐ On US-Dvorak GetPositionalKey(Z) TRANSLATES to Semicolon — the key at the physical Z position"), Resolved == EKeys::Semicolon);
		TestFalse(TEXT("⛔ Still never EKeys::Invalid on the hit path"), Resolved == EKeys::Invalid);
		TestFalse(TEXT("⛔ Still never EKeys::Escape on the hit path"), Resolved == EKeys::Escape);

		// A key ABSENT from a NON-EMPTY map still falls back to identity — the
		// second miss path, and the one a FindRef "simplification" would also break.
		TestTrue(TEXT("A letter absent from a populated map still resolves to ITSELF, not to Invalid"),
			Layout->GetPositionalKey(EKeys::Q) == EKeys::Q);
	}

	// ── (c) EVERY LETTER POSITION, ON BOTH MAPS ───────────────────────────────
	// ⛔ KBD-§4 scopes the feature to LETTERS ONLY, all 26. Sweeping the shipped
	// scan-code table means the two forbidden values are excluded for every key the
	// accept resolver could ever be pointed at, not just for `Z`.
	{
		const TArray<FSiegePositionalKeyProbe> Probes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();
		TestEqual(TEXT("The shipped table still covers all 26 letter positions"), Probes.Num(), 26);

		Layout->SetTranslationMapForAutomationTests(TMap<FKey, FKey>());

		int32 InvalidCount = 0;
		int32 EscapeCount = 0;
		for (const FSiegePositionalKeyProbe& Probe : Probes)
		{
			const FKey Resolved = Layout->GetPositionalKey(Probe.QwertyKey);
			if (!Resolved.IsValid() || Resolved == EKeys::Invalid)
			{
				++InvalidCount;
			}
			if (Resolved == EKeys::Escape)
			{
				++EscapeCount;
			}
		}

		TestEqual(TEXT("⛔ NOT ONE of the 26 letter positions resolves to EKeys::Invalid on a QWERTY host"), InvalidCount, 0);
		TestEqual(TEXT("⛔ NOT ONE of the 26 letter positions resolves to EKeys::Escape — Escape cannot become an absorbed key by this route"), EscapeCount, 0);
	}

	// ── (d) THE PLAYER-FACING PROMPT STILL SAYS `Z` ───────────────────────────
	// AS-§20.5: the key is resolved POSITIONALLY but the prompt says `Z` on every
	// layout, because the player is looking at a keycap. Restated here as the
	// reason the two must NOT be derived from one another; the on-screen string
	// itself is TASK-527's pixel check, not this file's.
	AddInfo(TEXT("The accept key is resolved positionally (GetPositionalKey(EKeys::Z)); the player-facing prompt says `Z` on every layout by design (AS-§20.5). The on-screen string is a human pixel check at TASK-527 and is deliberately NOT asserted here."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 14 — ⭐⭐ Siegebound.Assistant.Selection.RegionAdditivityAtTheWire
//
//  ⛔⛔ THE FIRST TEST OF THE AI-COMMANDER BATCH, AND IT IS FIRST ON PURPOSE.
//  AS-§21.5's central claim is "STRICTLY ADDITIVE AT THE WIRE: every JSON that
//  parses today parses BYTE-IDENTICALLY after", and TASK-550's criterion (1) says
//  to verify it AT THE CODE rather than at the claim. If it is false, this batch
//  is not a feature — it is a regression with a feature attached.
//
//  ⭐ "BYTE-IDENTICALLY" IS ASSERTED AS THE WHOLE STRUCT, NOT AS `bParsed`. The
//  failure this shape exists to catch is a SEVENTH FIELD quietly acquiring a
//  non-default value on an input that never mentioned a region — which no
//  `TestTrue(bParsed)` anywhere would see. See CommandDigest.
//
//  ⚠️ ONE INPUT'S BEHAVIOUR DID CHANGE AND IT IS PINNED AS THE SOLE EXCEPTION.
//  TASK-545 declared it: `{"in":…,"all_except":…}` moved from `unknown_key:in` to
//  `region_conflict:all_except/in`. ⛔ IT WAS REFUSED BEFORE AND IS REFUSED NOW —
//  only the reason code moved, and the new one is the true one ("two filters
//  stacked" rather than "I do not know that key"). A changed reason on a REFUSED
//  input is not a wire change; a changed outcome on an ACCEPTED one would be, and
//  there is none.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionAdditivityAtTheWireTest,
	"Siegebound.Assistant.Selection.RegionAdditivityAtTheWire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionAdditivityAtTheWireTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── (a) EVERY SHIPPED `who` SHAPE, ON EVERY VERB THAT TAKES IT, DIGESTED ───
	// ⛔ The expected digests are written from the SHIPPED SEMANTICS, not captured
	// from a run. A digest harvested from the parser and pasted back in would be a
	// guardrail that reports SAFE (AS-§12g) — the same defect the sibling ZoneA
	// file's frozen fixture exists to refuse.
	struct FAccepted
	{
		const TCHAR* Label;
		const TCHAR* Intent;
		const TCHAR* WhoPayload;
		const TCHAR* ExpectedDigest;
	};

	const FAccepted Accepted[] = {
		{ TEXT("shape 1 — a 1-item selection array on `send`"), TEXT("send"), TEXT("[{\"kind\":\"footman\",\"n\":10}]"),
		  TEXT("intent=send|kinds=[footman]|counts=[10]|except=[]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 1 — a 3-item selection array (the cap) on `guard`"), TEXT("guard"),
		  TEXT("[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1},{\"kind\":\"archer\",\"n\":\"all\"}]"),
		  TEXT("intent=guard|kinds=[footman,sorcerer,archer]|counts=[10,1,0]|except=[]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 2 — a 1-kind exclusion on `ambush`"), TEXT("ambush"), TEXT("{\"all_except\":[\"miner\"]}"),
		  TEXT("intent=ambush|kinds=[]|counts=[]|except=[miner]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 2 — a 3-kind exclusion on `follow`"), TEXT("follow"),
		  TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\"]}"),
		  TEXT("intent=follow|kinds=[]|counts=[]|except=[miner,cleric,sorcerer]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 3 — `who`:\"all\" on `send`"), TEXT("send"), TEXT("\"all\""),
		  TEXT("intent=send|kinds=[]|counts=[]|except=[]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 4 — `who`:\"none\" on the army-wide `charge`"), TEXT("charge"), TEXT("\"none\""),
		  TEXT("intent=charge|kinds=[]|counts=[]|except=[]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 4 — `who`:\"none\" on the army-wide `fallback`"), TEXT("fallback"), TEXT("\"none\""),
		  TEXT("intent=fallback|kinds=[]|counts=[]|except=[]|region=None|where=mid|trigger=None/0") },
		{ TEXT("shape 4 — `who`:\"none\" on the army-wide `rally`"), TEXT("rally"), TEXT("\"none\""),
		  TEXT("intent=rally|kinds=[]|counts=[]|except=[]|region=None|where=mid|trigger=None/0") }
	};

	for (const FAccepted& Case : Accepted)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(Case.Intent, Case.WhoPayload), Command, Error);

		if (!TestTrue(*FString::Printf(TEXT("%s still PARSES (error: %s)"), Case.Label, *Error), bParsed))
		{
			continue;
		}

		// ⭐ THE WHOLE STRUCT, CASE-SENSITIVELY. `RegionPlace` is inside the digest,
		// so an input that never said `in` acquiring a region fails HERE.
		TestEqualSensitive(*FString::Printf(TEXT("⭐ %s parses to a BYTE-IDENTICAL command — every field, including the new seventh"), Case.Label),
			CommandDigest(Command), FString(Case.ExpectedDigest));
	}

	// ── (b) THE DEFERRED-TRIGGER `when` SHAPE IS UNTOUCHED TOO ────────────────
	// It is the only other object-valued key in the schema, and the region branch
	// is an object branch — so it is the nearest neighbour to the edit and the
	// cheapest place for a stray `ValidateExactKeySet` change to land.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\",\"when\":{\"kind\":\"footman\",\"at_least\":3}}"), Command, Error);

		TestTrue(*FString::Printf(TEXT("The deferred `when` trigger still parses (error: %s)"), *Error), bParsed);
		TestEqualSensitive(TEXT("…to a byte-identical command, with the trigger intact and no region"),
			CommandDigest(Command),
			FString(TEXT("intent=send|kinds=[]|counts=[]|except=[]|region=None|where=mid|trigger=footman/3")));
	}

	// ── (c) EVERY PRE-EXISTING REFUSAL STILL REPORTS ITS OWN CODE ─────────────
	// ⛔ NOT `region_conflict` AND NOT `bad_region`. An FSM switching on the code
	// would route a familiar failure to a new, wrong clarification — a regression
	// with no compiler diagnostic behind it.
	struct FRefusal
	{
		const TCHAR* Label;
		const TCHAR* Intent;
		const TCHAR* WhoPayload;
		const TCHAR* ExpectedCode;
	};

	const FRefusal Refusals[] = {
		{ TEXT("`who` = an unknown string"), TEXT("send"), TEXT("\"everyone\""), SiegeAssistantReason::BadWho },
		{ TEXT("`who` = an EMPTY selection array"), TEXT("send"), TEXT("[]"), SiegeAssistantReason::WhoArity },
		{ TEXT("`who` = \"none\" on a selection-bearing verb"), TEXT("send"), TEXT("\"none\""), SiegeAssistantReason::WhoRequired },
		{ TEXT("`who` = a number"), TEXT("send"), TEXT("7"), SiegeAssistantReason::BadType },
		{ TEXT("a repeated kind in a positive selection"), TEXT("send"),
		  TEXT("[{\"kind\":\"footman\",\"n\":1},{\"kind\":\"footman\",\"n\":2}]"), SiegeAssistantReason::DuplicateKind },
		{ TEXT("an EMPTY exclusion list"), TEXT("send"), TEXT("{\"all_except\":[]}"), SiegeAssistantReason::ExcludeArity },
		{ TEXT("a 4-kind exclusion"), TEXT("send"),
		  TEXT("{\"all_except\":[\"miner\",\"cleric\",\"sorcerer\",\"ogre\"]}"), SiegeAssistantReason::ExcludeArity },
		{ TEXT("an exclusion on an army-wide verb"), TEXT("fallback"), TEXT("{\"all_except\":[\"miner\"]}"), SiegeAssistantReason::ExcludeConflict },
		{ TEXT("⛔ an EMPTY `who` object — still `missing_key`, which is what the region branch's `else` fall-through preserves"),
		  TEXT("send"), TEXT("{}"), SiegeAssistantReason::MissingKey },
		{ TEXT("an unknown key inside the `who` object"), TEXT("send"), TEXT("{\"foo\":1}"), SiegeAssistantReason::UnknownKey },
		{ TEXT("a misspelled `all_except`"), TEXT("send"), TEXT("{\"allexcept\":[\"miner\"]}"), SiegeAssistantReason::UnknownKey }
	};

	for (const FRefusal& Case : Refusals)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(CommandJson(Case.Intent, Case.WhoPayload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is still refused"), Case.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s still reports `%s` — NOT one of the new region codes"), Case.Label, Case.ExpectedCode),
			CodeOf(Error), FString(Case.ExpectedCode));
	}

	// ── (d) ⛔ THE TOP-LEVEL KEY SET IS STILL FOUR ────────────────────────────
	// AS-§21.5 rejected `"in"` as a FOURTH top-level key on this schema's own
	// rules, exactly as AS-§20.1 rejected `"except"`: `ValidateExactKeySet` demands
	// an exact set and the prompt law emits every key always, so a new top-level
	// key would have had to appear in EVERY emission at once. ⛔ THIS IS THE ONE
	// WAY ADDITIVITY COULD HAVE DIED SILENTLY, and it is asserted as behaviour.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			TEXT("{\"intent\":\"send\",\"who\":\"all\",\"in\":\"mid\",\"where\":\"mid\",\"when\":\"now\"}"), Command, Error);

		TestFalse(TEXT("⛔ `in` as a FOURTH TOP-LEVEL KEY is refused — the region is a `who` SHAPE, never a new key"), bParsed);
		TestEqualSensitive(TEXT("…reported as `unknown_key`, exactly as `except` is"), CodeOf(Error), FString(SiegeAssistantReason::UnknownKey));
	}
	{
		// The converse: a command MISSING a top-level key still reports missing_key,
		// so the exact-key-set rule did not become one-directional.
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("A command missing `when` is still refused"),
			ParseSiegeAssistantCommand(TEXT("{\"intent\":\"send\",\"who\":\"all\",\"where\":\"mid\"}"), Command, Error));
		TestEqualSensitive(TEXT("…as `missing_key`"), CodeOf(Error), FString(SiegeAssistantReason::MissingKey));
	}

	// ── (e) ⚠️ THE ONE DECLARED EXCEPTION, PINNED AS THE ONLY ONE ─────────────
	// TASK-545's handoff §3: `{"in":…,"all_except":…}` moved `unknown_key:in` ->
	// `region_conflict:all_except/in`. It is asserted here rather than merely
	// allowed, so that (a) the new code is the one that ships and (b) the input is
	// on the record as having been REFUSED ON BOTH SIDES of the change.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			CommandJson(TEXT("send"), TEXT("{\"in\":\"mid\",\"all_except\":[\"miner\"]}")), Command, Error);

		TestFalse(TEXT("⛔ A region AND an exclusion in one `who` object is refused — two filters stacked, and nobody ruled on how they compose"), bParsed);
		TestEqualSensitive(TEXT("⚠️ THE BATCH'S SOLE DECLARED BEHAVIOUR CHANGE: it now reports `region_conflict` (was `unknown_key`). Both are REFUSALS — no accepted input changed."),
			CodeOf(Error), FString(SiegeAssistantReason::RegionConflict));
		TestEqualSensitive(TEXT("…and the payload names BOTH filters, so the log says which two stacked"),
			PayloadOf(Error), FString(TEXT("all_except/in")));
		TestTrue(TEXT("…and the command is fully reset, never half-built"),
			Command.Intent == ESiegeAssistantIntent::None && Command.RegionPlace.IsNone() && Command.ExcludeKinds.Num() == 0);
	}

	AddInfo(TEXT("ADDITIVITY: 8 accepted shapes + the deferred trigger were digested field-by-field; 11 pre-existing refusal codes were re-asserted; the 4-key top-level set was re-asserted in both directions. ⚠️ ONE input changed reason code and it is REFUSED on both sides of the change (TASK-545's declared exception). ⛔ No input that SUCCEEDS today behaves differently."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 15 — ⭐ Siegebound.Assistant.Selection.RegionParses
//
//  THE ACCEPT SIDE OF JONATHAN'S SENTENCE B: "send all units currently in an
//  ancient ground to attack a castle". Before this batch that sentence was
//  STRUCTURALLY INEXPRESSIBLE — `who` had four shapes, all type-symbol shapes,
//  and nothing anywhere carried a spatial predicate (AS-§21.1).
//
//  ⭐ THE LOAD-BEARING ASSERTION IS THE LAST ONE: `who` AND `where` POPULATED IN
//  ONE COMMAND, out of the SAME seven-symbol vocabulary. That is the distinction
//  TASK-547's 147-char rule line exists to teach, and it is the one the model has
//  to get right for the feature to do anything at all.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionParsesTest,
	"Siegebound.Assistant.Selection.RegionParses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionParsesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── ALL FOUR SELECTION-BEARING VERBS × ALL THREE REGION SYMBOLS ───────────
	const TCHAR* const SelectionBearingIntents[] = { TEXT("send"), TEXT("guard"), TEXT("ambush"), TEXT("follow") };
	const TArray<FName> Regions = ThreeRegionPlaces();

	for (const TCHAR* Intent : SelectionBearingIntents)
	{
		for (const FName& Region : Regions)
		{
			const FString Payload = FString::Printf(TEXT("\"%s\""), *Region.ToString());

			FSiegeAssistantCommand Command;
			FString Error;
			const bool bParsed = ParseSiegeAssistantCommand(RegionJson(Intent, *Payload), Command, Error);

			if (!TestTrue(*FString::Printf(TEXT("⭐ `%s` accepts {\"in\":\"%s\"} (error: %s)"), Intent, *Region.ToString(), *Error), bParsed))
			{
				continue;
			}

			TestEqualSensitive(*FString::Printf(TEXT("`%s` stores the region symbol verbatim"), Intent),
				Command.RegionPlace.ToString(), Region.ToString());

			// ⭐ THE SEAM THE EXECUTOR KEYS ON, AND IT IS THE SAME ONE THE EXCLUSION
			// USES: a region is a MODIFIED "all", so Kinds stays EMPTY and the
			// command flows through the existing `Kinds.Num() == 0` path rather than
			// through a second selector. ⛔ If the parser ever synthesised an
			// enumeration here, the region would silently become a positive
			// selection capped at three kinds.
			TestEqual(*FString::Printf(TEXT("`%s`: Kinds stays EMPTY — a region is a modified `all`, never a synthesised enumeration"), Intent),
				Command.Kinds.Num(), 0);
			TestEqual(*FString::Printf(TEXT("`%s`: Counts stays empty alongside it"), Intent), Command.Counts.Num(), 0);
			TestEqual(*FString::Printf(TEXT("`%s`: ExcludeKinds stays empty — the two filters never merge"), Intent), Command.ExcludeKinds.Num(), 0);
		}
	}

	// ── SYMBOL CASE IS NORMALISED, NOT REJECTED ──────────────────────────────
	// The grammar only ever emits lower case, but FName is case-insensitive and the
	// parser lower-cases the symbol before constructing it. Pinned so a future
	// "strict" pass does not start refusing a model that capitalised one letter.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("A mixed-case region symbol still parses"),
			ParseSiegeAssistantCommand(RegionJson(TEXT("send"), TEXT("\"Ancient_Ground_Near\"")), Command, Error));
		TestEqualSensitive(TEXT("…and is stored LOWER-CASE, the canonical wire form"),
			Command.RegionPlace.ToString(), FString(TEXT("ancient_ground_near")));
	}

	// ── ⛔ NO EXISTENCE CHECK AT THE PARSER, AND IT IS A LEGAL PARSE ──────────
	// `ParseSiegeAssistantCommand` is PURE — no world, no roster, no snapshot — so
	// "does that region exist?" and "was anybody standing in it?" are questions it
	// STRUCTURALLY CANNOT ANSWER. Both are the executor's, answered through
	// `ResolvePlaceRegion` and the loud empty-after-region refusal. This mirrors the
	// `where` reader, which has never validated its place either, and it is
	// AS-§20.1's EMPTY-AFTER-EXCLUSION clause applied verbatim (AS-§21.5).
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("⭐ An UNKNOWN region symbol is a legal PARSE — existence is the executor's question, and refusing here would need a world the parser must never have"),
			ParseSiegeAssistantCommand(RegionJson(TEXT("send"), TEXT("\"atlantis\"")), Command, Error));
		TestEqualSensitive(TEXT("…and it survives as the symbol the model wrote, for the executor to refuse"),
			Command.RegionPlace.ToString(), FString(TEXT("atlantis")));
	}
	{
		// The same for a region that exists but is EMPTY on the board: from the
		// parser's seat those two inputs are the identical string. Written as its
		// own case because "empty-after-region is not a parse error" is a RULING
		// (AS-§21.5), and a ruling with no test is a preference.
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("⭐ EMPTY-AFTER-REGION IS NOT A PARSE ERROR — the refusal (with arithmetic) belongs to the executor, which is the only thing that can count occupants"),
			ParseSiegeAssistantCommand(RegionJson(TEXT("guard"), TEXT("\"mid\"")), Command, Error));
	}

	// ── ⭐⭐ JONATHAN'S SENTENCE B, END TO END AT THE PARSER ──────────────────
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(
			TEXT("{\"intent\":\"send\",\"who\":{\"in\":\"ancient_ground_near\"},\"where\":\"enemy_castle\",\"when\":\"now\"}"),
			Command, Error);

		TestTrue(*FString::Printf(TEXT("⭐⭐ \"send all units currently in an ancient ground to attack a castle\" is now EXPRESSIBLE (error: %s)"), *Error), bParsed);
		TestEqualSensitive(TEXT("⭐ BOTH place-valued keys are populated, from the SAME vocabulary, in ONE command — `who` is where they ARE"),
			Command.RegionPlace.ToString(), FString(TEXT("ancient_ground_near")));
		TestEqualSensitive(TEXT("…and `where` is where they GO. Getting these two the wrong way round sends the army to the ground it was recruited from."),
			Command.Where.ToString(), FString(TEXT("enemy_castle")));
	}

	// ⛔ NO ACCURACY CLAIM IS MADE OR IMPLIED ANYWHERE IN THIS FILE (AS-§12f).
	// Everything above is about what the PARSER accepts. Whether the model EMITS
	// the shape is unmeasured — AS-§21.11 outcome 5 records that honestly, and
	// TASK-542's raw-output log is what will finally show it.
	AddInfo(TEXT("⛔ This test asserts what the PARSER accepts. It says NOTHING about how often the model emits the shape — that is unmeasured (AS-§21.11 outcome 5) and no figure for it may appear anywhere (AS-§12f)."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 16 — ⭐⭐ Siegebound.Assistant.Selection.RegionRefusedOnArmyWideIntents
//
//  ⛔⛔ CROSS-FIELD CHECK 3, AND IT IS THE SILENT-DROP ASSERTION AGAIN — the same
//  one TEST 8 makes for the exclusion, for word-for-word the same reason.
//  charge / fallback / rally execute through `ASiegePlayerController::
//  ApplyArmyWideStance` and `AHeroCharacter::Rally()`; NONE of the three walks the
//  candidate list, so a `RegionPlace` handed to them has no code path that could
//  filter anything. It would be PARSED AND THEN DROPPED IN SILENCE, and
//  "fall back, but only the ones in the mid" would execute as "FALL BACK,
//  EVERYONE" — a valid-shaped wrong command that LOOKS obeyed.
//
//  ⚖️ AN EXCLUSION AND A REGION ARE BOTH FILTERS AND BOTH FAIL THIS WAY. That is
//  why this is the SECOND INSTANCE OF ONE PRINCIPLE rather than a second
//  principle, and it is why the gate is the shipped
//  `SiegeAssistantIntentTakesSelection` and not a new list of verbs.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionRefusedOnArmyWideIntentsTest,
	"Siegebound.Assistant.Selection.RegionRefusedOnArmyWideIntents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionRefusedOnArmyWideIntentsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	const TCHAR* const ArmyWideIntents[] = { TEXT("charge"), TEXT("fallback"), TEXT("rally") };

	for (const TCHAR* Intent : ArmyWideIntents)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(RegionJson(Intent, TEXT("\"mid\"")), Command, Error);

		TestFalse(*FString::Printf(TEXT("⛔ `%s ... only the ones in the mid` is REFUSED — it can never be honoured, so it is never accepted"), Intent), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("`%s` reports `region_conflict`"), Intent),
			CodeOf(Error), FString(SiegeAssistantReason::RegionConflict));

		// The payload names the OFFENDING VERB, which is what lets the FSM and the
		// log say which half of the sentence could not be kept.
		TestEqualSensitive(*FString::Printf(TEXT("`%s`'s payload is the intent symbol itself, so the log names the verb"), Intent),
			PayloadOf(Error), FString(Intent));

		// ⛔ AND THE ORDER IS NOT SILENTLY DEGRADED TO THE UNFILTERED ONE. This is
		// the assertion that distinguishes "refused" from "accepted with the filter
		// dropped" — the exact failure the whole cross-field check exists to stop.
		TestTrue(*FString::Printf(TEXT("⛔ `%s` is NOT accepted with the region quietly discarded — the command is fully reset"), Intent),
			Command.Intent == ESiegeAssistantIntent::None && Command.RegionPlace.IsNone());
	}

	// ── THE GATE IS THE SHIPPED PREDICATE, NOT A SECOND LIST OF VERBS ─────────
	// Reusing `SiegeAssistantIntentTakesSelection` means there is no parallel
	// army-wide list to drift out of step with the executor seam it describes, and
	// a future eighth verb lands on the right side automatically.
	TestTrue(TEXT("Send / Guard / Ambush / Follow take a selection, so they may carry a region"),
		SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Send)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Guard)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Ambush)
		&& SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Follow));
	TestFalse(TEXT("Charge / Fallback / Rally do NOT — they never reach the selector, so a region there could only be dropped"),
		SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Charge)
		|| SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Fallback)
		|| SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent::Rally));

	// ✅ THE CONTROL — this proves the refusal is about the VERB, not about regions
	// in general.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestTrue(TEXT("✅ The same region on `send` is ACCEPTED"),
			ParseSiegeAssistantCommand(RegionJson(TEXT("send"), TEXT("\"mid\"")), Command, Error));
	}

	// 📌 THE CONSEQUENCE IS DESIGNED, AND IT IS ON JONATHAN'S PLAYTEST SHEET.
	// ⚠️ Recorded as an AddInfo so a reader of a red-or-green run sees it too: this
	// is the one outcome most likely to be reported as a bug when it is a ruling.
	AddInfo(TEXT("📌 AS-§21.11 item 1, DESIGNED, NOT A BUG: \"everyone in the mid, fall back\" is REFUSED (region_conflict:fallback) — the same verdict, for the same reason, as DEV-08. Refusing is the only answer that cannot silently move an army the player never mentioned."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 17 — Siegebound.Assistant.Selection.RegionSymbolsRefused
//
//  The malformed-payload paths, by reason code and by payload.
//
//  ⚠️ `"none"` IS THE LOAD-BEARING ONE AND IT IS WHY THE CHECK LIVES IN THE
//  PARSER RATHER THAN IN THE VALIDATOR. `FName(TEXT("none"))` IS `NAME_None`, so a
//  tolerated "none" would be stored as "no region was named" and
//  `{"who":{"in":"none"}}` would execute as "EVERYONE" — a filter the player typed,
//  dropped in silence. The symbol has to be caught in the STRING, before the FName
//  exists.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionSymbolsRefusedTest,
	"Siegebound.Assistant.Selection.RegionSymbolsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionSymbolsRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	struct FCase
	{
		const TCHAR* Label;
		const TCHAR* Payload;
		const TCHAR* ExpectedCode;
	};

	const FCase Cases[] = {
		// ── NOT A STRING AT ALL ⇒ bad_type:in ────────────────────────────────
		{ TEXT("`in` holding a number"),  TEXT("7"),                    SiegeAssistantReason::BadType },
		{ TEXT("`in` holding an array"),  TEXT("[\"mid\"]"),            SiegeAssistantReason::BadType },
		{ TEXT("`in` holding an object"), TEXT("{\"place\":\"mid\"}"),  SiegeAssistantReason::BadType },
		{ TEXT("`in` holding null"),      TEXT("null"),                 SiegeAssistantReason::BadType },
		{ TEXT("`in` holding a bool"),    TEXT("true"),                 SiegeAssistantReason::BadType },

		// ── A STRING THAT CANNOT NAME A PLACE ⇒ bad_region ───────────────────
		{ TEXT("an EMPTY region symbol"),          TEXT("\"\""),      SiegeAssistantReason::BadRegion },
		{ TEXT("a whitespace-only region symbol"), TEXT("\"   \""),   SiegeAssistantReason::BadRegion },
		{ TEXT("⛔ the reserved sentinel `none` — the silent-drop case"), TEXT("\"none\""), SiegeAssistantReason::BadRegion },
		{ TEXT("the reserved sentinel `all`"),     TEXT("\"all\""),   SiegeAssistantReason::BadRegion },
		{ TEXT("⚠️ the reserved sentinel `now` — see the AddInfo below"), TEXT("\"now\""), SiegeAssistantReason::BadRegion }
	};

	for (const FCase& Case : Cases)
	{
		FSiegeAssistantCommand Command;
		FString Error;
		const bool bParsed = ParseSiegeAssistantCommand(RegionJson(TEXT("send"), Case.Payload), Command, Error);

		TestFalse(*FString::Printf(TEXT("%s is refused"), Case.Label), bParsed);
		TestEqualSensitive(*FString::Printf(TEXT("%s reports `%s`"), Case.Label, Case.ExpectedCode),
			CodeOf(Error), FString(Case.ExpectedCode));
		TestTrue(*FString::Printf(TEXT("%s leaves the command fully reset"), Case.Label),
			Command.Intent == ESiegeAssistantIntent::None && Command.RegionPlace.IsNone());
	}

	// ⛔ THE `none` CASE, RESTATED AS THE THING IT ACTUALLY PREVENTS — the same
	// shape TEST 6 uses for the empty exclusion list. If `{"in":"none"}` parsed, it
	// would parse to a command IDENTICAL to `who`:"all", i.e. an order that moves
	// the whole army the player was trying to narrow.
	{
		FSiegeAssistantCommand Region;
		FString RegionError;
		const bool bRegionParsed = ParseSiegeAssistantCommand(RegionJson(TEXT("send"), TEXT("\"none\"")), Region, RegionError);

		FSiegeAssistantCommand All;
		FString AllError;
		const bool bAllParsed = ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"all\"")), All, AllError);

		TestTrue(TEXT("⛔ `{\"in\":\"none\"}` must NOT parse to the same command as `who`:\"all\" — FName(\"none\") IS NAME_None, so tolerating it would turn a filter into an unfiltered army"),
			bAllParsed && !bRegionParsed);
	}

	// ── THE EXACT-KEY-SET RULE APPLIES INSIDE THE REGION OBJECT TOO ───────────
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("An unknown key beside `in` is refused"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"in\":\"mid\",\"n\":5}")), Command, Error));
		TestEqualSensitive(TEXT("…as `unknown_key`"), CodeOf(Error), FString(SiegeAssistantReason::UnknownKey));
	}

	// ⚠️⚠️ A DECLARED SUPERSET, FLAGGED FOR TASK-550 RATHER THAN QUIETLY ENCODED.
	// AS-§21.5 ENUMERATES the BadRegion triggers as `""` / `"all"` / `"none"` while
	// JUSTIFYING them as "the three reserved wire symbols (SiegeAssistantSymbols)
	// can never name a place" — and the third reserved symbol is `now`, not `""`.
	// The two statements do not describe the same set. TASK-545 resolved it as a
	// SUPERSET (`""` plus all three reserved symbols) and declared it; this test
	// PINS WHAT SHIPS, and it is deliberately written so the disagreement is
	// visible in the run log rather than buried in a handoff.
	//
	// ⛔ IF TASK-550 RULES THE ENUMERATION EXHAUSTIVE, the `"now"` row above is the
	// one to delete, together with one `||` clause in `ParseRegionSymbol` and one in
	// the validator's reserved-symbol test. ⚖️ The test author's own view, recorded
	// because a test that hides its author's disagreement is worth less: the
	// superset is right — `USiegeAssistantGrammar::Build` drops all three reserved
	// symbols before they can become `zone` alternatives, so `now` can never name a
	// live region either, and refusing it costs nothing that was reachable.
	AddInfo(TEXT("⚠️ FLAGGED FOR TASK-550: `{\"in\":\"now\"}` is refused as bad_region. AS-§21.5 ENUMERATES only \"\" / \"all\" / \"none\", but JUSTIFIES the list as \"the three reserved wire symbols\", which are none / all / NOW. TASK-545 shipped the SUPERSET and declared it; this test pins what ships. Rule it explicitly — the enumeration and its own justification contradict each other."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 18 — Siegebound.Assistant.Selection.RegionConflictsRefused
//
//  ⛔ NEVER A MERGE. "Send 10 footmen in the mid" is a FILTERED COUNT — a
//  different feature that was not asked for — and silently honouring one half of
//  it is the valid-shaped-wrong-command class this design exists to stop
//  (AS-§21.5).
//
//  ⚠️ AND THE REACHABILITY IS STATED HONESTLY, exactly as TEST 7 does for the
//  exclusion: from JSON the `who` shapes are DISJOINT (a selection is an Array,
//  an exclusion and a region are Objects with different keys), so most of these
//  states CANNOT be produced by any model output. They are reachable over the
//  M8 P2 wire, from a peer that passed through nobody's grammar and nobody's
//  parser — which is precisely why `SiegeAssistantValidateSelection`, the
//  receive-side gate, is where they are tested.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionConflictsRefusedTest,
	"Siegebound.Assistant.Selection.RegionConflictsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionConflictsRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── CASE 1 — A POSITIVE SELECTION AND A REGION (the wire path) ────────────
	{
		FString Error;
		const bool bValid = SiegeAssistantValidateSelection(
			TArray<FName>{ TEXT("footman") }, TArray<int32>{ 10 }, Error,
			TArray<FName>(), FName(TEXT("mid")));

		TestFalse(TEXT("⛔ A positive selection PLUS a region is refused — \"send 10 footmen in the mid\" is a filtered COUNT, a feature nobody asked for"), bValid);
		TestEqualSensitive(TEXT("…and it reports `region_conflict`"), CodeOf(Error), FString(SiegeAssistantReason::RegionConflict));
		TestEqualSensitive(TEXT("…with a payload naming BOTH halves, so the log says what could not be composed"),
			PayloadOf(Error), FString(TEXT("1/mid")));
	}

	// ── CASE 2 — AN EXCLUSION AND A REGION (the wire path) ────────────────────
	{
		FString Error;
		const bool bValid = SiegeAssistantValidateSelection(
			TArray<FName>(), TArray<int32>(), Error,
			TArray<FName>{ TEXT("miner") }, FName(TEXT("mid")));

		TestFalse(TEXT("⛔ An exclusion PLUS a region is refused — two filters stacked, and nobody ruled on how they compose"), bValid);
		TestEqualSensitive(TEXT("…as `region_conflict`"), CodeOf(Error), FString(SiegeAssistantReason::RegionConflict));
		TestEqualSensitive(TEXT("…payload `all_except/mid`"), PayloadOf(Error), FString(TEXT("all_except/mid")));
	}

	// ── CASE 3 — THE SAME PAIR, THROUGH THE PARSER'S OWN OBJECT BRANCH ────────
	// This one IS reachable from JSON (both keys live in the same object), which is
	// why it earns its own code BEFORE the exact-key-set check rather than being
	// reported as the technically-true-and-useless `unknown_key:in`.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("⛔ `{\"in\":\"mid\",\"all_except\":[\"miner\"]}` is refused at the PARSER"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("{\"in\":\"mid\",\"all_except\":[\"miner\"]}")), Command, Error));
		TestEqualSensitive(TEXT("…as `region_conflict:all_except/in`, not `unknown_key`"),
			Error, FString(TEXT("region_conflict:all_except/in")));
	}

	// ── CASE 4 — `who`:"none" WITH A REGION ──────────────────────────────────
	// ⚠️ STRUCTURALLY UNREACHABLE FROM JSON AND SAID SO RATHER THAN FAKED: the
	// parser sets `bWhoIsNone` only in its String branch and `RegionPlace` only in
	// its Object branch, so no single `who` value can produce both. The shipped
	// guard is honestly labelled DEFENSIVE in the source, and it is written for the
	// wire — the previous instance of this guard held ONLY because it had been
	// written when the fifth shape arrived. What IS reachable, and is asserted
	// here, is the pre-existing behaviour beside it, unchanged by this batch.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("`who`:\"none\" on a selection-bearing verb is still refused (pre-existing, unchanged)"),
			ParseSiegeAssistantCommand(CommandJson(TEXT("send"), TEXT("\"none\"")), Command, Error));
		TestEqualSensitive(TEXT("…still with `who_required`, NOT `region_conflict` — the change is additive"),
			CodeOf(Error), FString(SiegeAssistantReason::WhoRequired));
	}

	// ── CASE 5 — THE RESERVED SYMBOLS, RE-CHECKED ON THE WIRE SIDE ───────────
	// ⚠️ `"none"` NEEDS NO TEST HERE AND CANNOT HAVE ONE: `FName(TEXT("none"))` IS
	// `NAME_None`, so a peer sending "none" is INDISTINGUISHABLE from a peer sending
	// no region at all — and "no region" is the safe reading, because it filters
	// nothing rather than filtering wrongly. That asymmetry with the parser is
	// asserted rather than left as a surprise.
	{
		FString AllError;
		TestFalse(TEXT("A wire peer sending region `all` is refused"),
			SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), AllError, TArray<FName>(), FName(TEXT("all"))));
		TestEqualSensitive(TEXT("…as `bad_region:all`"), AllError, FString(TEXT("bad_region:all")));

		FString NowError;
		TestFalse(TEXT("A wire peer sending region `now` is refused"),
			SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), NowError, TArray<FName>(), FName(TEXT("now"))));
		TestEqualSensitive(TEXT("…as `bad_region:now`"), NowError, FString(TEXT("bad_region:now")));

		FString NoneError;
		TestTrue(TEXT("⚠️ A wire peer sending region \"none\" is INDISTINGUISHABLE from sending none at all — FName(\"none\") IS NAME_None, so it validates as \"no region\", which filters nothing rather than filtering wrongly. The PARSER catches the STRING before the FName exists; this is the honest limit of the wire side."),
			SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), NoneError, TArray<FName>(), FName(TEXT("none"))));
	}

	// ✅ AND A LEGAL REGION IS NOT REFUSED BY THE NEW INVARIANTS ──────────────
	{
		FString Error;
		TestTrue(TEXT("✅ A region against an empty selection and an empty exclusion is VALID — the control that proves the refusals above are about the COMBINATION"),
			SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), Error, TArray<FName>(), FName(TEXT("mid"))));
		TestEqualSensitive(TEXT("…with no error text at all"), Error, FString());
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 19 — ⭐⭐ Siegebound.Assistant.Selection.ValidatorRegionArgumentIsLoadBearing
//
//  ⛔⛔ THE HIGHEST-RISK LINE IN THE BATCH, PINNED — AND THIS IS THE SECOND BATCH
//  RUNNING WHERE A TRAILING DEFAULT IS THAT LINE.
//
//  `SiegeAssistantValidateSelection` now has TWO trailing defaulted parameters:
//  `ExcludeKinds` (4th, TASK-518) and `RegionPlace` (5th, TASK-545). Both shapes
//  were FORCED by file ownership, not chosen. The cost is written into the header
//  in its own words: "A CALLER THAT OMITS THIS ARGUMENT SILENTLY VALIDATES NOTHING
//  ABOUT THE REGION, AND NO COMPILER DIAGNOSTIC STANDS BEHIND IT — the omission
//  compiles, links, runs and reports SUCCESS."
//
//  ⭐ SO THE TEST MEASURES ALL THREE CALL FORMS, NOT TWO. TEST 11 measured what
//  the FOURTH argument buys; this measures what the FIFTH buys AND re-measures the
//  fourth in its presence — because "we already have a test for the exclusion
//  argument" is exactly the reasoning under which a second omission ships.
//
//  ⚠️ WHAT THIS TEST HONESTLY IS, AND IS NOT (TEST 11's caveat, unchanged). It
//  CANNOT observe the two whole-command call sites: `SiegeAssistantCommand.cpp`'s
//  is inside the parser and `SiegeAssistantComponent.cpp`'s needs a live component,
//  a world and the Thinking state. What it does is WRITE DOWN, AS AN EXECUTABLE
//  FACT, exactly how much validation each argument buys — so the hazard is a red
//  bar the moment anyone changes what a default MEANS. ⛔ A dropped argument at a
//  call site would still compile and this suite would still be green; that
//  residual is real, it is TASK-550 criterion (3)'s grep, and it is stated rather
//  than papered over.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionValidatorRegionArgumentIsLoadBearingTest,
	"Siegebound.Assistant.Selection.ValidatorRegionArgumentIsLoadBearing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionValidatorRegionArgumentIsLoadBearingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ── (a) COMMANDS INVALID BECAUSE OF THE REGION ALONE ─────────────────────
	// Each is a state the M8 P2 wire path can deliver, since a peer passed through
	// nobody's grammar and nobody's parser.
	struct FRegionCase
	{
		const TCHAR* Label;
		TArray<FName> Kinds;
		TArray<int32> Counts;
		TArray<FName> Excludes;
		FName Region;
		const TCHAR* ExpectedCode;
	};

	TArray<FRegionCase> RegionCases;
	RegionCases.Add({ TEXT("a selection AND a region at once"),
		TArray<FName>{ TEXT("footman") }, TArray<int32>{ 10 }, TArray<FName>(),
		FName(TEXT("mid")), SiegeAssistantReason::RegionConflict });
	RegionCases.Add({ TEXT("an exclusion AND a region at once"),
		TArray<FName>(), TArray<int32>(), TArray<FName>{ TEXT("miner") },
		FName(TEXT("mid")), SiegeAssistantReason::RegionConflict });
	RegionCases.Add({ TEXT("the reserved symbol `all` as a region"),
		TArray<FName>(), TArray<int32>(), TArray<FName>(),
		FName(TEXT("all")), SiegeAssistantReason::BadRegion });
	RegionCases.Add({ TEXT("the reserved symbol `now` as a region"),
		TArray<FName>(), TArray<int32>(), TArray<FName>(),
		FName(TEXT("now")), SiegeAssistantReason::BadRegion });

	for (const FRegionCase& Case : RegionCases)
	{
		// (i) THE FIVE-ARGUMENT CALL — the one every caller holding a whole
		//     FSiegeAssistantCommand must make.
		FString FiveArgError;
		const bool bFiveArgValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, FiveArgError, Case.Excludes, Case.Region);

		TestFalse(*FString::Printf(TEXT("[%s] the 5-argument call REFUSES it"), Case.Label), bFiveArgValid);
		TestEqualSensitive(*FString::Printf(TEXT("[%s] …reporting `%s`"), Case.Label, Case.ExpectedCode),
			CodeOf(FiveArgError), FString(Case.ExpectedCode));

		// (ii) THE FOUR-ARGUMENT CALL — what dropping ONLY `RegionPlace` becomes.
		//      ⛔ This is the exact edit that would have shipped at
		//      SiegeAssistantComponent.cpp's receive-side gate had TASK-548 not
		//      caught it, and it raises no diagnostic anywhere.
		FString FourArgError;
		const bool bFourArgValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, FourArgError, Case.Excludes);

		// (iii) THE THREE-ARGUMENT CALL — dropping BOTH filters.
		FString ThreeArgError;
		const bool bThreeArgValid = SiegeAssistantValidateSelection(Case.Kinds, Case.Counts, ThreeArgError);

		if (!TestTrue(*FString::Printf(
				TEXT("⛔ [%s] DROPPING THE FIFTH ARGUMENT VALIDATES NOTHING ABOUT THE REGION and reports this command VALID. That is why every caller holding an FSiegeAssistantCommand MUST pass Command.RegionPlace — SiegeAssistantCommand.cpp's parser call and SiegeAssistantComponent.cpp's receive-side gate both do, and dropping either compiles clean and silently turns this refusal into an acceptance."),
				Case.Label),
			bFourArgValid && !bFiveArgValid))
		{
			AddError(FString::Printf(
				TEXT("[%s] The FIFTH-argument hazard has CHANGED SHAPE: 5-arg=%s, 4-arg=%s. Re-read SiegeAssistantValidateSelection's signature and re-run `grep -rn \"SiegeAssistantValidateSelection(\" Source/ Plugins/` before touching anything else."),
				Case.Label, bFiveArgValid ? TEXT("valid") : TEXT("refused"), bFourArgValid ? TEXT("valid") : TEXT("refused")));
		}

		TestTrue(*FString::Printf(TEXT("⛔ [%s] and dropping BOTH filters validates nothing either"), Case.Label),
			bThreeArgValid);
	}

	// ── (b) THE FOURTH ARGUMENT IS RE-MEASURED IN THE FIFTH'S PRESENCE ────────
	// ⛔ NOT REDUNDANT WITH TEST 11. TEST 11 was written when there were four
	// parameters; this asserts that adding a fifth did not change what the fourth
	// buys — which is the additivity claim applied to the VALIDATOR's signature
	// rather than to the wire.
	{
		const TArray<FName> Excludes{ TEXT("miner"), TEXT("cleric"), TEXT("sorcerer"), TEXT("ogre") };

		FString FiveArgError;
		const bool bFiveArgValid = SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), FiveArgError, Excludes, NAME_None);
		FString FourArgError;
		const bool bFourArgValid = SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), FourArgError, Excludes);
		FString ThreeArgError;
		const bool bThreeArgValid = SiegeAssistantValidateSelection(TArray<FName>(), TArray<int32>(), ThreeArgError);

		TestFalse(TEXT("A 4-kind exclusion is still refused by the 5-argument call"), bFiveArgValid);
		TestFalse(TEXT("…and by the 4-argument call — the fourth argument still buys exactly what it bought"), bFourArgValid);
		TestEqualSensitive(TEXT("…with the SAME error text through both forms"), FourArgError, FiveArgError);
		TestTrue(TEXT("⛔ …while the 3-argument call still validates NOTHING about it"), bThreeArgValid);
	}

	// ── (c) THE CONVERSE: A CLEAN COMMAND VALIDATES IDENTICALLY THROUGH ALL ───
	// The defaults exist FOR the arrays-only call sites. A valid selection with
	// neither filter must validate identically through all three forms, or the
	// "strictly additive" claim is false for the callers that never asked for the
	// feature — and there are a dozen of those in this file and its sibling.
	{
		const TArray<FName> Kinds{ TEXT("footman"), TEXT("archer") };
		const TArray<int32> Counts{ 10, 0 };

		FString ThreeArgError;
		FString FourArgError;
		FString FiveArgError;
		const bool bThree = SiegeAssistantValidateSelection(Kinds, Counts, ThreeArgError);
		const bool bFour = SiegeAssistantValidateSelection(Kinds, Counts, FourArgError, TArray<FName>());
		const bool bFive = SiegeAssistantValidateSelection(Kinds, Counts, FiveArgError, TArray<FName>(), NAME_None);

		TestTrue(TEXT("A valid, filter-free selection passes ALL THREE call forms"), bThree && bFour && bFive);
		TestEqualSensitive(TEXT("…with the same (empty) error on the 3- and 4-argument forms"), ThreeArgError, FourArgError);
		TestEqualSensitive(TEXT("…and on the 5-argument form"), FiveArgError, FourArgError);
	}

	// ── (d) ⛔ A SIXTH TRAILING DEFAULT IS FORBIDDEN, AND THE COUNT IS PINNED ─
	// AS-§21.9 accepts the fifth WITH ITS COST NAMED and forbids a sixth: "the NEXT
	// field-shaped addition takes an overload taking `const FSiegeAssistantCommand&`
	// instead of a sixth default." ⚠️ There is no way to assert a parameter COUNT
	// from a test, and pretending otherwise would be worse than saying so — the
	// three call forms above are the closest mechanical statement available, and a
	// sixth default would make a FOURTH form silently meaningful without failing
	// anything here.
	AddInfo(TEXT("⛔ AS-§21.9: a SIXTH trailing default is forbidden — the next field-shaped addition takes a `const FSiegeAssistantCommand&` overload, which cannot be under-called at all. ⚠️ This test measures the 3-/4-/5-argument forms; it CANNOT see a sixth default being added, and no test can. That is a review criterion (TASK-550 item 3), not an assertion."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 20 — ⭐⭐ Siegebound.Assistant.Selection.RegionMembershipContract
//
//  `FSiegeAssistantRegionStatics::IsPointInRegion`, every branch, with no world.
//
//  ⭐⭐ THE EXTENT IS DELIBERATELY NON-SQUARE, AND THAT IS THE WHOLE POINT OF THE
//  TEST. TASK-544's own handoff (§8, item 2) asks for it in terms: both shipped
//  regions are SQUARE `(840, 840)`, so a TRANSPOSED-AXIS implementation —
//  `Point.X` compared against `HalfExtent.Y` — RETURNS THE IDENTICAL ANSWER ON
//  EVERY SQUARE BOX. ⛔ THE SHIPPED DATA STRUCTURALLY CANNOT CATCH THAT MISTAKE,
//  so a test written against shipped geometry would pass a transposed predicate
//  and report SAFE. Two probes below fail in OPPOSITE DIRECTIONS under a
//  transposition, so neither a swapped comparison nor a swapped extent survives.
//
//  ⚖️ AND WHY THE PREDICATE MATTERS THAT MUCH: it exists to give the assistant THE
//  SAME ANSWER THE GAME ALREADY GIVES. A divergence here makes a unit "in the mid"
//  for capture scoring and "not in the mid" for selection — the exact class
//  AS-§21.4 forbids.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionMembershipContractTest,
	"Siegebound.Assistant.Selection.RegionMembershipContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionMembershipContractTest::RunTest(const FString& Parameters)
{
	// ⛔ OFF-ORIGIN CENTRE AND A NON-SQUARE EXTENT. An origin-centred fixture would
	// pass an implementation that ignored `Centre` entirely, and a square one would
	// pass a transposed implementation. Neither survives this geometry.
	const FVector Centre(1000.0, -2000.0, 300.0);
	const FVector2D HalfExtent(400.0, 100.0);

	TestTrue(TEXT("⛔ THE FIXTURE'S EXTENT IS NON-SQUARE — a square extent cannot distinguish a correct predicate from a transposed one, and BOTH shipped regions are square (840,840)"),
		HalfExtent.X != HalfExtent.Y);

	auto InRegion = [&Centre, &HalfExtent](double X, double Y, double Z) -> bool
	{
		return FSiegeAssistantRegionStatics::IsPointInRegion(FVector(X, Y, Z), Centre, HalfExtent);
	};

	// ── (a) PLAINLY INSIDE / PLAINLY OUTSIDE, ON EACH AXIS SEPARATELY ─────────
	TestTrue(TEXT("A point well inside the box is INSIDE"), InRegion(1200.0, -2050.0, 0.0));
	TestTrue(TEXT("The centre itself is INSIDE"), InRegion(1000.0, -2000.0, 0.0));
	TestFalse(TEXT("A point beyond the X half-extent is OUTSIDE"), InRegion(1500.0, -2000.0, 0.0));
	TestFalse(TEXT("A point beyond the Y half-extent is OUTSIDE"), InRegion(1000.0, -1850.0, 0.0));
	TestFalse(TEXT("A point beyond BOTH is OUTSIDE"), InRegion(1500.0, -1850.0, 0.0));
	TestFalse(TEXT("⛔ The WORLD ORIGIN is OUTSIDE this off-origin box — an implementation that ignored `Centre` would say otherwise"),
		InRegion(0.0, 0.0, 0.0));

	// ── (b) ⭐ EXACTLY ON THE BOUNDARY ⇒ INSIDE (`<=`, not `<`) ───────────────
	// ⚖️ NOT A DETAIL. Both shipped instances use `<=`; flipping this to `<` would
	// make a unit standing on the edge "in the mid" for capture scoring and "not in
	// the mid" for selection. All four edges and all four corners, because a single
	// mistyped comparison affects exactly one of them.
	TestTrue(TEXT("⭐ Exactly on the +X edge is INSIDE (`<=`)"), InRegion(1400.0, -2000.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the -X edge is INSIDE"), InRegion(600.0, -2000.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the +Y edge is INSIDE"), InRegion(1000.0, -1900.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the -Y edge is INSIDE"), InRegion(1000.0, -2100.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the (+X,+Y) corner is INSIDE"), InRegion(1400.0, -1900.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the (-X,-Y) corner is INSIDE"), InRegion(600.0, -2100.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the (+X,-Y) corner is INSIDE"), InRegion(1400.0, -2100.0, 0.0));
	TestTrue(TEXT("⭐ Exactly on the (-X,+Y) corner is INSIDE"), InRegion(600.0, -1900.0, 0.0));

	// The other side of the same edge, so "inclusive" is a boundary and not a
	// tolerance.
	TestFalse(TEXT("One unit past the +X edge is OUTSIDE — the boundary is a boundary, not a tolerance"), InRegion(1401.0, -2000.0, 0.0));
	TestFalse(TEXT("One unit past the -Y edge is OUTSIDE"), InRegion(1000.0, -2101.0, 0.0));

	// ── (c) ⭐ Z IS IGNORED ENTIRELY, ABOVE AND BELOW ─────────────────────────
	// ⚖️ COPIED, NOT RE-DECIDED. Units stand on terrain of varying height and the
	// hills are climbable, so a 3D test would quietly omit whoever walked uphill
	// INSIDE the footprint — "everyone in the ancient ground" minus the ones on the
	// rise. Both shipped instances already made this choice and say so.
	TestTrue(TEXT("⭐ A point far ABOVE the centre's Z is STILL INSIDE — Z is never read"), InRegion(1200.0, -2050.0, 1.0e9));
	TestTrue(TEXT("⭐ A point far BELOW the centre's Z is STILL INSIDE"), InRegion(1200.0, -2050.0, -1.0e9));
	TestTrue(TEXT("⭐ …and at the largest representable Z"), InRegion(1200.0, -2050.0, TNumericLimits<double>::Max()));
	TestTrue(TEXT("⭐ …and at the smallest"), InRegion(1200.0, -2050.0, TNumericLimits<double>::Lowest()));
	TestFalse(TEXT("⛔ An extreme Z does NOT rescue a point that is outside in XY — Z is ignored, not forgiving"),
		InRegion(1500.0, -2000.0, 1.0e9));

	// ── (d) ⭐⭐ THE TRANSPOSED-AXIS PROBES — THE PAIR SHIPPED DATA CANNOT WRITE
	// Probe A: offset (0, +250). Correct  ⇒ OUTSIDE (250 > HalfExtent.Y = 100).
	//                            Transposed ⇒ INSIDE  (250 <= HalfExtent.X = 400).
	// Probe B: offset (+250, 0). Correct  ⇒ INSIDE  (250 <= HalfExtent.X = 400).
	//                            Transposed ⇒ OUTSIDE (250 > HalfExtent.Y = 100).
	// ⇒ The two probes fail in OPPOSITE directions, so a transposition cannot be
	//   hidden by either one alone, and neither can a "helpful" symmetrisation.
	{
		const bool bProbeA = InRegion(1000.0, -1750.0, 0.0);
		const bool bProbeB = InRegion(1250.0, -2000.0, 0.0);

		TestFalse(TEXT("⭐⭐ TRANSPOSED-AXIS PROBE A: a point 250 units off in Y is OUTSIDE (the Y half-extent is 100). A predicate comparing Point.Y against HalfExtent.X would call this INSIDE."), bProbeA);
		TestTrue(TEXT("⭐⭐ TRANSPOSED-AXIS PROBE B: a point 250 units off in X is INSIDE (the X half-extent is 400). A predicate comparing Point.X against HalfExtent.Y would call this OUTSIDE."), bProbeB);

		if (bProbeA == bProbeB)
		{
			AddError(TEXT("⛔ THE AXES ARE NOT DISTINGUISHED. Probes A and B are the SAME distance from the centre on DIFFERENT axes against DIFFERENT half-extents, so they must disagree. They agreed - which means IsPointInRegion is comparing X against HalfExtent.Y (or is using one extent for both axes). ⚠️ This defect is INVISIBLE to every test written against the shipped regions, because both are square (840,840). Re-read SiegeAssistantRegionStatics.cpp against AncientGround.cpp:143-151 token by token."));
		}

		// The same two probes against a SQUARE extent, to show — in the run log —
		// exactly why the fixture above is not square.
		const bool bSquareA = FSiegeAssistantRegionStatics::IsPointInRegion(FVector(1000.0, -1750.0, 0.0), Centre, FVector2D(840.0, 840.0));
		const bool bSquareB = FSiegeAssistantRegionStatics::IsPointInRegion(FVector(1250.0, -2000.0, 0.0), Centre, FVector2D(840.0, 840.0));
		TestTrue(TEXT("Against the SHIPPED SQUARE (840,840) extent both probes are INSIDE — which is precisely why the shipped geometry cannot catch a transposition"),
			bSquareA && bSquareB);
	}

	// ── (e) THE DEGENERATE EXTENTS — PINNING WHAT TASK-544 CHOSE ──────────────
	// ⛔ NOT SPECIAL-CASED, DELIBERATELY, AND THIS TEST PINS THE CHOICE RATHER THAN
	// ENDORSING IT. Two reasons are recorded on the header: (a) any guard would be a
	// DIVERGENCE from the two shipped instances, which is exactly what this function
	// exists not to be; (b) the degenerate answer is FAIL-CLOSED — an empty region
	// selects nobody, and an empty selection is a LOUD refusal with arithmetic in
	// the executor, never a silently unfiltered army.
	// ⚠️ If TASK-550 wants a guard, that ruling must ALSO apply to
	// AAncientGround::IsPointInZone and ACaptureZone::IsPointInZone, or it
	// re-creates the divergence this file exists to prevent.
	{
		TestTrue(TEXT("A ZERO extent admits ONLY the exact centre"),
			FSiegeAssistantRegionStatics::IsPointInRegion(Centre, Centre, FVector2D::ZeroVector));
		TestFalse(TEXT("…and nothing one unit away from it"),
			FSiegeAssistantRegionStatics::IsPointInRegion(FVector(1001.0, -2000.0, 0.0), Centre, FVector2D::ZeroVector));

		TestFalse(TEXT("⛔ A NEGATIVE extent admits NOTHING — not even the centre, because FMath::Abs is never negative. FAIL-CLOSED, and stated in the header rather than discovered."),
			FSiegeAssistantRegionStatics::IsPointInRegion(Centre, Centre, FVector2D(-1.0, -1.0)));
		TestFalse(TEXT("…and a single negative axis is enough to empty the region"),
			FSiegeAssistantRegionStatics::IsPointInRegion(Centre, Centre, FVector2D(400.0, -1.0)));

		// A zero extent on ONE axis only: a line, not a point. Asserted because
		// "degenerate" is three different shapes and a guard would collapse them.
		TestTrue(TEXT("A zero extent on ONE axis leaves a LINE, and points on it are inside"),
			FSiegeAssistantRegionStatics::IsPointInRegion(FVector(1200.0, -2000.0, 0.0), Centre, FVector2D(400.0, 0.0)));
		TestFalse(TEXT("…and points off that line are not"),
			FSiegeAssistantRegionStatics::IsPointInRegion(FVector(1200.0, -2001.0, 0.0), Centre, FVector2D(400.0, 0.0)));
	}

	// ⚠️ THE HEADER ALSO DOCUMENTS "a NaN coordinate is OUTSIDE" AND IT IS
	// DELIBERATELY NOT ASSERTED HERE. Constructing a NaN and comparing it is
	// compiler- and flag-dependent under fast floating point, so the assertion could
	// pass, fail or be optimised away for reasons that have nothing to do with this
	// predicate — and a test whose outcome depends on the optimiser is worse than a
	// documented note. ⛔ Recorded rather than silently skipped.
	AddInfo(TEXT("⚠️ NOT COVERED: the header's NaN-is-outside claim. A NaN comparison's behaviour depends on the float model the module is built with, so asserting it would produce a result about the compiler rather than about IsPointInRegion. The claim is fail-closed in the same direction as everything else here and is left as documentation."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 21 — ⭐ Siegebound.Assistant.Selection.RegionResolvesFromSnapshotGeometry
//
//  `USiegeAssistantSnapshot::ResolvePlaceRegion` — the seam between TASK-547's
//  captured geometry and TASK-544's predicate, joined here with no world.
//
//  ⛔ THE REGION LIST IS THE AUTHORITY, AND THE FOUR NON-REGION PLACES MUST FAIL.
//  `own_castle` resolves perfectly well as a DESTINATION and has no box at all;
//  answering it with the castle's location and a zero extent would be a silent
//  wrong answer WEARING A `true`, and the caller would then filter an army against
//  a point-sized box.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionResolvesFromSnapshotGeometryTest,
	"Siegebound.Assistant.Selection.RegionResolvesFromSnapshotGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionResolvesFromSnapshotGeometryTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	FScratchSnapshot Scratch = MakeSnapshotWithRegions(ThirteenKindsInCardRowOrder(), /*PerKindTotal*/ 9, /*bPublishRegions*/ true);
	if (!TestTrue(*FString::Printf(
			TEXT("The snapshot's Capture()-owned geometry fields were reachable by reflection (missing: %s). ⛔ If this fails, a field was RENAMED or RETYPED and every assertion below would otherwise pass VACUOUSLY on an empty snapshot."),
			*Scratch.MissingField),
		Scratch.IsUsable()))
	{
		return false;
	}

	USiegeAssistantSnapshot* const Snapshot = Scratch.Snapshot.Get();

	// ── (a) THE THREE REGION-BEARING PLACES RESOLVE, WITH THEIR OWN GEOMETRY ──
	for (const FName& Region : ThreeRegionPlaces())
	{
		FVector Centre = FVector::ZeroVector;
		FVector2D HalfExtent = FVector2D::ZeroVector;

		const bool bResolved = Snapshot->ResolvePlaceRegion(Region, Centre, HalfExtent);
		if (!TestTrue(*FString::Printf(TEXT("⭐ `%s` resolves to a region"), *Region.ToString()), bResolved))
		{
			continue;
		}

		TestTrue(*FString::Printf(TEXT("`%s` carries the captured NON-SQUARE half-extent (X = 1337)"), *Region.ToString()), HalfExtent.X == 1337.0);
		TestTrue(*FString::Printf(TEXT("`%s` carries the captured NON-SQUARE half-extent (Y = 8642)"), *Region.ToString()), HalfExtent.Y == 8642.0);

		// ⭐ THE TWO HALVES JOINED — the executor's actual expression, minus the
		// world: resolve the box, then ask the predicate. ⛔ THE OFFSET IS THE SAME
		// 5000 ON BOTH AXES AND THE ANSWERS DIFFER, which is only possible because
		// the captured box is NOT SQUARE (1337 x 8642). A transposed predicate, or
		// one that used a single extent for both axes, gives the same answer twice.
		TestTrue(*FString::Printf(TEXT("⭐ A unit standing at `%s`'s captured centre is INSIDE it"), *Region.ToString()),
			FSiegeAssistantRegionStatics::IsPointInRegion(Centre, Centre, HalfExtent));
		TestFalse(*FString::Printf(TEXT("⭐ A unit 5000 units off in X is OUTSIDE `%s` (its X half-extent is 1337)"), *Region.ToString()),
			FSiegeAssistantRegionStatics::IsPointInRegion(Centre + FVector(5000.0, 0.0, 0.0), Centre, HalfExtent));
		TestTrue(*FString::Printf(TEXT("⭐ …while the SAME 5000 units off in Y is INSIDE `%s` (its Y half-extent is 8642) — the box is not square, and that is what makes this pair meaningful"), *Region.ToString()),
			FSiegeAssistantRegionStatics::IsPointInRegion(Centre + FVector(0.0, 5000.0, 0.0), Centre, HalfExtent));
	}

	// ── (b) ⛔ THE FOUR NON-REGION PLACES DO NOT RESOLVE, AND LEAVE THE OUT-PARAMS
	//        UNTOUCHED
	// ⚠️ THE UNTOUCHED-ON-FAILURE CONTRACT IS ASSERTED, NOT ASSUMED. It mirrors
	// `ResolvePlace` rather than `ValidateCommandAgainstSnapshot`, deliberately: a
	// caller that ignores the return value keeps its OWN initialised box, and there
	// is ⛔ NO "whole map" default — inventing one would turn "send everyone in the
	// mid" into "send everyone", the one open-failure mode AS-§21.5 forbids outright.
	const TCHAR* const NonRegionPlaces[] = { TEXT("own_castle"), TEXT("enemy_castle"), TEXT("nearest_mine"), TEXT("hero") };
	for (const TCHAR* Place : NonRegionPlaces)
	{
		const FVector Sentinel(-12345.0, -54321.0, -6789.0);
		const FVector2D SentinelExtent(-11.0, -22.0);

		FVector Centre = Sentinel;
		FVector2D HalfExtent = SentinelExtent;

		TestFalse(*FString::Printf(TEXT("⛔ `%s` is NOT region-bearing and does NOT resolve — it is a destination with no box, and answering it would be a wrong answer wearing a `true`"), Place),
			Snapshot->ResolvePlaceRegion(FName(Place), Centre, HalfExtent));
		TestTrue(*FString::Printf(TEXT("⛔ `%s` leaves BOTH out-params UNTOUCHED on failure (the ResolvePlace contract, and the reason there is no whole-map fallback)"), Place),
			Centre.Equals(Sentinel, 0.0) && HalfExtent.Equals(SentinelExtent, 0.0));
	}

	// ── (c) SYMBOLS THAT ARE NOT PLACES AT ALL ───────────────────────────────
	{
		FVector Centre = FVector::ZeroVector;
		FVector2D HalfExtent = FVector2D::ZeroVector;

		TestFalse(TEXT("An unknown symbol does not resolve"), Snapshot->ResolvePlaceRegion(FName(TEXT("atlantis")), Centre, HalfExtent));
		TestFalse(TEXT("NAME_None does not resolve — \"no region named\" is not a region"), Snapshot->ResolvePlaceRegion(NAME_None, Centre, HalfExtent));
		TestFalse(TEXT("The reserved symbol `all` does not resolve"), Snapshot->ResolvePlaceRegion(FName(TEXT("all")), Centre, HalfExtent));
	}

	// ── (d) ⭐ A SNAPSHOT THAT PUBLISHED NO REGIONS RESOLVES NOTHING ──────────
	// The legal "map with no capture zone and no ancient ground" state. ⛔ It must
	// FAIL CLOSED: every region symbol becomes unresolvable, the executor refuses,
	// and the grammar never offered the shape in the first place.
	{
		FScratchSnapshot Bare = MakeSnapshotWithRegions(ThirteenKindsInCardRowOrder(), /*PerKindTotal*/ 9, /*bPublishRegions*/ false);
		if (TestTrue(*FString::Printf(TEXT("A region-free snapshot was built (missing: %s)"), *Bare.MissingField), Bare.IsUsable()))
		{
			TestEqual(TEXT("A region-free snapshot publishes an EMPTY region list — a legal state, not an error"),
				Bare.Snapshot->GetRegionPlaceNames().Num(), 0);

			for (const FName& Region : ThreeRegionPlaces())
			{
				FVector Centre = FVector::ZeroVector;
				FVector2D HalfExtent = FVector2D::ZeroVector;
				TestFalse(*FString::Printf(TEXT("⛔ `%s` does NOT resolve on a map that published no regions — FAIL CLOSED, never a fall-through to the whole map"), *Region.ToString()),
					Bare.Snapshot->ResolvePlaceRegion(Region, Centre, HalfExtent));
			}
		}
	}

	// ── (e) THE PUBLISHED LIST IS A SUBSET OF THE PLACE VOCABULARY ───────────
	// ⚠️ The two can never disagree about what exists: a symbol reaches
	// `RegionPlaceNames` only if it is ALREADY in `PlaceNames`. Asserted because
	// `ResolvePlaceRegion` indexes one array using membership of the other.
	{
		const TArray<FName>& Published = Snapshot->GetRegionPlaceNames();
		TestEqual(TEXT("The fixture published exactly three regions"), Published.Num(), 3);
		for (const FName& Region : Published)
		{
			TestTrue(*FString::Printf(TEXT("⛔ Published region `%s` is also in PlaceNames — the two lists can never disagree about what exists"), *Region.ToString()),
				Snapshot->GetPlaceNames().Contains(Region));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 22 — ⭐ Siegebound.Assistant.Selection.RegionGrammarShapes
//
//  Three regions / one region / zero regions, against the REAL emitter.
//
//  ⛔ THE ZERO-REGION CASE IS THE ONE THAT CAN KILL THE BATCH, AND IT IS ASSERTED
//  AS BYTE-EQUALITY WITH THE TWO-ARGUMENT BUILD. An empty alternation would leave
//  `zone` DEFINED-AS-NOTHING with `inplace` referencing it, and llama.cpp answers
//  a grammar it cannot parse by generating UNCONSTRAINED — a TOTAL LOSS of the
//  mechanism, not a missing feature. That is the `at_least` disaster that cost
//  TASK-413 two of its six bars.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionGrammarShapesTest,
	"Siegebound.Assistant.Selection.RegionGrammarShapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionGrammarShapesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();
	const TArray<FName> Places = SevenPlaces();
	const TArray<FName> Regions = ThreeRegionPlaces();

	// ── (a) THREE REGIONS — THE FULL BOARD ───────────────────────────────────
	{
		const FString Grammar = USiegeAssistantGrammar::Build(Kinds, Places, Regions);

		TestTrue(TEXT("A region-bearing board defines the `inplace` rule"), !GrammarRuleRhs(Grammar, TEXT("inplace")).IsEmpty());
		TestTrue(TEXT("…and the `zone` rule"), !GrammarRuleRhs(Grammar, TEXT("zone")).IsEmpty());

		// ⛔ THE RULE NAMES CARRY NO UNDERSCORE. `in_place` would parse as the name
		// `in`, after which llama.cpp rejects the WHOLE grammar. This project has
		// been bitten TWICE (`at_least`, then `except_list`), which is why it is
		// asserted rather than reviewed.
		TestFalse(TEXT("⛔ `in_place` is NOT a rule name — it would make the whole grammar unparseable"),
			Grammar.Contains(TEXT("in_place ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("⛔ `zone_list` is NOT a rule name either"),
			Grammar.Contains(TEXT("zone_list ::="), ESearchCase::CaseSensitive));

		// ── `zone`'s ALTERNATIVES ARE EXACTLY THE PASSED SYMBOLS, IN CALLER ORDER
		// ⛔ CALLER ORDER, NEVER SORTED. `FName::operator<` orders by comparison
		// index, which depends on the order names were first registered IN THE
		// PROCESS — sorting by it would silently break the determinism guarantee
		// the whole suite rests on.
		const TArray<FString> ZoneAlternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("zone")));
		if (TestEqual(TEXT("`zone` offers exactly one alternative per live region"), ZoneAlternatives.Num(), Regions.Num()))
		{
			for (int32 Index = 0; Index < Regions.Num(); ++Index)
			{
				TestEqualSensitive(*FString::Printf(TEXT("`zone` alternative %d is `%s` — CALLER ORDER, never sorted"), Index + 1, *Regions[Index].ToString()),
					ZoneAlternatives[Index], FString::Printf(TEXT("\"\\\"%s\\\"\""), *Regions[Index].ToString()));
			}
		}

		// ⛔ `zone` HAS NO "none" ALTERNATIVE, UNLIKE `where` — TASK-546's declared
		// D3. `where` carries "none" because it is a key that is ALWAYS emitted and
		// the army-wide verbs have no destination; `zone` is reachable only from
		// INSIDE `inplace`, which the model chooses to enter, so "no region" is
		// already said by the other four `who` shapes. A "none" here would be a
		// second spelling of the same thing, and `{"in":"none"}` is BadRegion.
		//
		// ⚠️ ASSERTED ON THE ALTERNATIVES, NOT ON A SUBSTRING SEARCH — a place
		// symbol could legitimately CONTAIN "all" or "none" (a `wall_gate` would),
		// and a substring test would then fail for a reason that has nothing to do
		// with a reserved terminal.
		{
			const TArray<FString> Alternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("zone")));
			bool bHasReservedTerminal = false;
			for (const FString& Alternative : Alternatives)
			{
				bHasReservedTerminal = bHasReservedTerminal
					|| Alternative.Equals(TEXT("\"\\\"none\\\"\""), ESearchCase::CaseSensitive)
					|| Alternative.Equals(TEXT("\"\\\"all\\\"\""), ESearchCase::CaseSensitive);
			}
			TestFalse(TEXT("⛔ `zone` offers NO \"none\" and NO \"all\" alternative (unlike `where`, which carries \"none\") — absence is said by not entering `inplace`, and {\"in\":\"none\"} is bad_region"),
				bHasReservedTerminal);
		}
		{
			// The `where` rule DOES carry "none" — asserted so the asymmetry above is
			// a difference between two live rules rather than a claim about one.
			const TArray<FString> WhereAlternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("where")));
			bool bWhereHasNone = false;
			for (const FString& Alternative : WhereAlternatives)
			{
				bWhereHasNone = bWhereHasNone || Alternative.Equals(TEXT("\"\\\"none\\\"\""), ESearchCase::CaseSensitive);
			}
			TestTrue(TEXT("⭐ …while `where` DOES carry \"none\" — the asymmetry is deliberate and is a difference between two LIVE rules, not an assertion about one"),
				bWhereHasNone);
		}

		// ── `who` NOW OFFERS FIVE ALTERNATIVES, IN THE PINNED ORDER ───────────
		const TArray<FString> WhoAlternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("who")));
		if (TestEqual(TEXT("⭐ `who` offers FIVE alternatives with kinds and regions both live"), WhoAlternatives.Num(), 5))
		{
			TestEqualSensitive(TEXT("`who` alternative 1 is `selection`"), WhoAlternatives[0], FString(TEXT("selection")));
			TestEqualSensitive(TEXT("`who` alternative 2 is `except`"), WhoAlternatives[1], FString(TEXT("except")));
			TestEqualSensitive(TEXT("⭐ `who` alternative 3 is `inplace` — THIRD, after `except` and BEFORE the two bare strings, so the three object/array shapes stay grouped (AS-§21.5)"),
				WhoAlternatives[2], FString(TEXT("inplace")));
			TestEqualSensitive(TEXT("`who` alternative 4 is the \"all\" terminal"), WhoAlternatives[3], FString(TEXT("\"\\\"all\\\"\"")));
			TestEqualSensitive(TEXT("`who` alternative 5 is the \"none\" terminal"), WhoAlternatives[4], FString(TEXT("\"\\\"none\\\"\"")));
		}

		// `inplace` references `zone` exactly once, and the JSON key is inside a
		// TERMINAL — which is why `in` needs no kebab-case split the way `at_least`
		// did.
		TestTrue(TEXT("`inplace` references `zone`"), GrammarRuleRhs(Grammar, TEXT("inplace")).Contains(TEXT("zone"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("The JSON key `\"in\"` appears inside a terminal"),
			Grammar.Contains(TEXT("\\\"in\\\""), ESearchCase::CaseSensitive));

		// ⛔ BOUNDED ALTERNATION, NEVER A REPETITION OPERATOR — an unbounded
		// repetition is exactly what a small model rambles into.
		TestFalse(TEXT("The region rules add no '*' repetition operator"), Grammar.Contains(TEXT("*")));
		TestFalse(TEXT("…no '+'"), Grammar.Contains(TEXT("+")));
		TestFalse(TEXT("…and no '?'"), Grammar.Contains(TEXT("?")));

		// Determinism survives the new rules: same state in, byte-identical grammar
		// out. This is what keeps the sampler's constraint stable across turns.
		TestEqualSensitive(TEXT("The grammar is still byte-deterministic with the region rules present"),
			USiegeAssistantGrammar::Build(Kinds, Places, Regions), Grammar);
	}

	// ── (b) ONE REGION AND ZERO KINDS — THE CASE `bHasKinds` WOULD HAVE KILLED ─
	// ⭐ "Everyone in the mid" names NO unit kind, and a board with nothing spawned
	// is exactly where a player points at ground instead of at units. Gating
	// `inplace` on the roster would have made the shape unreachable there.
	{
		const FString Grammar = USiegeAssistantGrammar::Build(TArray<FName>(), Places, TArray<FName>{ TEXT("mid") });

		TestFalse(TEXT("An empty roster still OMITS `kind`"), Grammar.Contains(TEXT("kind ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("…and `except`"), Grammar.Contains(TEXT("except ::="), ESearchCase::CaseSensitive));
		TestTrue(TEXT("⭐ …but STILL defines `inplace` — the gate is REGIONS, not kinds (AS-§21.4, and a stated QA criterion: do not \"fix\" it into symmetry)"),
			!GrammarRuleRhs(Grammar, TEXT("inplace")).IsEmpty());

		const TArray<FString> ZoneAlternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("zone")));
		TestEqual(TEXT("`zone` offers exactly one alternative at one region"), ZoneAlternatives.Num(), 1);

		const TArray<FString> WhoAlternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("who")));
		if (TestEqual(TEXT("⭐ `who` is `inplace | \"all\" | \"none\"` — three alternatives"), WhoAlternatives.Num(), 3))
		{
			TestEqualSensitive(TEXT("…and `inplace` is FIRST once the two kind-gated shapes are gone"),
				WhoAlternatives[0], FString(TEXT("inplace")));
		}
	}

	// ── (c) ⛔⛔ ZERO REGIONS ⇒ BYTE-IDENTICAL TO THE TWO-ARGUMENT BUILD ──────
	{
		const FString TwoArg = USiegeAssistantGrammar::Build(Kinds, Places);
		const FString EmptyRegions = USiegeAssistantGrammar::Build(Kinds, Places, TArray<FName>());

		TestEqualSensitive(TEXT("⛔⛔ An EMPTY region list produces a grammar BYTE-IDENTICAL to the 2-argument build — every one of the ~20 existing call sites is untouched and unchanged"),
			EmptyRegions, TwoArg);

		TestFalse(TEXT("⛔ No `inplace` rule"), EmptyRegions.Contains(TEXT("inplace ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("⛔ No `zone` rule"), EmptyRegions.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive));

		const TArray<FString> WhoAlternatives = GrammarAlternatives(GrammarRuleRhs(EmptyRegions, TEXT("who")));
		TestEqual(TEXT("⛔ `who` is the shipped FOUR-alternative line, byte-identical to today's"), WhoAlternatives.Num(), 4);
		TestEqualSensitive(TEXT("⛔ …and the whole `who` right-hand side matches the 2-argument build exactly"),
			GrammarRuleRhs(EmptyRegions, TEXT("who")), GrammarRuleRhs(TwoArg, TEXT("who")));

		// Zero regions AND zero kinds — the fully degenerate board.
		TestEqualSensitive(TEXT("⛔ A board with neither kinds nor regions is byte-identical to the 2-argument build too"),
			USiegeAssistantGrammar::Build(TArray<FName>(), Places, TArray<FName>()),
			USiegeAssistantGrammar::Build(TArray<FName>(), Places));
	}

	// ── (d) ⭐ TASK-546's DECLARED D2, PINNED: THE GATE IS THE POST-CANONICAL COUNT
	// A caller that hands in ONLY reserved or empty symbols passes a NON-EMPTY array
	// that canonicalises to NOTHING. Gating on the raw parameter count would then
	// emit `zone ::= ` with an EMPTY right-hand side — the whole-grammar rejection
	// the requirement exists to prevent. TASK-546 gated on the POST-filter count and
	// declared it as stricter than the spec's literal wording; this pins it.
	{
		const FString TwoArg = USiegeAssistantGrammar::Build(Kinds, Places);
		const FString ReservedOnly = USiegeAssistantGrammar::Build(Kinds, Places,
			TArray<FName>{ NAME_None, FName(TEXT("all")), FName(TEXT("none")) });

		TestEqualSensitive(TEXT("⭐ A region list of ONLY reserved/empty symbols canonicalises to nothing and produces a BYTE-IDENTICAL grammar — never an empty `zone ::= ` right-hand side (TASK-546 D2)"),
			ReservedOnly, TwoArg);
		TestFalse(TEXT("⛔ …and there is no dangling `zone ::=` anywhere in it"),
			ReservedOnly.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 23 — ⭐⭐ Siegebound.Assistant.Selection.RegionRuleNamesAreCharsetLegal
//
//  ⛔⛔ LAW ZERO, EXTENDED TO THE SHAPES THAT DID NOT EXIST WHEN IT WAS WRITTEN.
//
//  TASK-546 found the hole and could not close it: `Siegebound.Assistant.Grammar
//  .RuleNameCharset` builds four grammars under a comment claiming "every shape the
//  builder can produce, because a rule that is only emitted on one branch is
//  exactly the rule that escapes review" — but ALL FOUR use the two-argument
//  overload, so `inplace` and `zone` were invisible to the ONE test guarding the
//  rule-name charset.
//
//  ⚠️ AND THIS PROJECT HAS PAID FOR THAT CHARSET TWICE. `at_least` shipped as a
//  rule name: llama.cpp read the name `at`, demanded `::=`, found `_least`, and
//  REJECTED THE WHOLE GRAMMAR — `llama_sampler_init_grammar` returned NULL on
//  every generation of all six of TASK-413's bench runs, the model emitted
//  `<think>` prose instead of JSON, and bars #2 and #5 came back NOT MEASURED.
//  `except_list` was the second, caught before it shipped. `in_place` /
//  `zone_list` would have been the third.
//
//  ⚖️ EVERY OTHER TEST PASSES THROUGHOUT SUCH A FAILURE, WHICH IS THE POINT: they
//  are all true of a string the sampler never loads.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionRuleNamesAreCharsetLegalTest,
	"Siegebound.Assistant.Selection.RegionRuleNamesAreCharsetLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionRuleNamesAreCharsetLegalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();
	const TArray<FName> Places = SevenPlaces();
	const TArray<FName> Regions = ThreeRegionPlaces();

	// ⭐ EVERY REGION-BEARING SHAPE THE BUILDER CAN NOW PRODUCE. The three-argument
	// overload is what makes this test see `inplace` and `zone` at all.
	struct FBoard
	{
		const TCHAR* Label;
		FString Grammar;
	};

	const FBoard Boards[] = {
		{ TEXT("kinds + three regions"),        USiegeAssistantGrammar::Build(Kinds, Places, Regions) },
		{ TEXT("NO kinds + three regions"),     USiegeAssistantGrammar::Build(TArray<FName>(), Places, Regions) },
		{ TEXT("kinds + ONE region"),           USiegeAssistantGrammar::Build(Kinds, Places, TArray<FName>{ TEXT("mid") }) },
		{ TEXT("NO kinds + ONE region"),        USiegeAssistantGrammar::Build(TArray<FName>(), Places, TArray<FName>{ TEXT("mid") }) },
		{ TEXT("kinds + NO regions"),           USiegeAssistantGrammar::Build(Kinds, Places, TArray<FName>()) },
		{ TEXT("kinds + regions, NO places"),   USiegeAssistantGrammar::Build(Kinds, TArray<FName>(), Regions) }
	};

	int32 BoardsDefiningInplace = 0;

	for (const FBoard& Board : Boards)
	{
		TArray<FString> Lines;
		Board.Grammar.ParseIntoArrayLines(Lines, /*bCullEmpty*/ true);
		TestTrue(*FString::Printf(TEXT("[%s] the grammar has at least one rule"), Board.Label), Lines.Num() > 0);

		for (const FString& Line : Lines)
		{
			const int32 ArrowIndex = Line.Find(TEXT(" ::= "), ESearchCase::CaseSensitive);
			if (!TestTrue(*FString::Printf(TEXT("[%s] line is a rule: %s"), Board.Label, *Line), ArrowIndex != INDEX_NONE))
			{
				continue;
			}

			const FString RuleName = Line.Left(ArrowIndex);
			TestTrue(
				*FString::Printf(TEXT("[%s] rule name \"%s\" is legal GBNF — llama.cpp accepts [a-zA-Z0-9-] ONLY, and one bad character rejects the WHOLE grammar"), Board.Label, *RuleName),
				IsLegalGbnfRuleNameHere(RuleName));
		}

		if (Board.Grammar.Contains(TEXT("inplace ::="), ESearchCase::CaseSensitive))
		{
			++BoardsDefiningInplace;

			// The REFERENCE half of the same defect: `at_least` appeared once as a
			// definition and once as a reference, and either alone is fatal. `who`
			// references `inplace`, and `inplace` references `zone`.
			TestTrue(*FString::Printf(TEXT("[%s] `who` references `inplace` by its legal one-word name"), Board.Label),
				GrammarRuleRhs(Board.Grammar, TEXT("who")).Contains(TEXT("inplace"), ESearchCase::CaseSensitive));
			TestTrue(*FString::Printf(TEXT("[%s] `inplace` references `zone` by its legal one-word name"), Board.Label),
				GrammarRuleRhs(Board.Grammar, TEXT("inplace")).Contains(TEXT("zone"), ESearchCase::CaseSensitive));
			TestTrue(*FString::Printf(TEXT("[%s] `zone` is DEFINED, so the reference resolves — an undefined `zone` breaks the whole grammar"), Board.Label),
				Board.Grammar.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive));
		}
	}

	// ⛔ THE SWEEP MUST HAVE REACHED THE NEW RULES AT ALL. Without this the whole
	// test could pass on six grammars that never emitted `inplace` — which is
	// EXACTLY the shape of the hole it was written to close, reproduced one level
	// up. (TASK-546's D4: four grammars, all 2-argument, guarding nothing.)
	// ⛔ FIVE, NOT SIX: only the `kinds + NO regions` board omits `inplace`, because
	// the gate is REGIONS ONLY and never `bHasKinds` (AS-§21.4). The `NO places`
	// board still emits it — `zone` is generated from the region list, which is
	// canonicalised independently of the place list.
	TestEqual(TEXT("⛔ FIVE of the six boards actually EMITTED `inplace` — otherwise this test reproduces the very coverage hole it exists to close"),
		BoardsDefiningInplace, 5);

	// ── THE GUARD ITSELF IS WORTH ONE ROW: A TEST THAT ALWAYS PASSES IS NOT A GATE
	TestTrue(TEXT("`inplace` is a legal rule name"), IsLegalGbnfRuleNameHere(TEXT("inplace")));
	TestTrue(TEXT("`zone` is a legal rule name"), IsLegalGbnfRuleNameHere(TEXT("zone")));
	TestFalse(TEXT("⛔ `in_place` is REJECTED — this is the shape of the shipped `at_least` defect"), IsLegalGbnfRuleNameHere(TEXT("in_place")));
	TestFalse(TEXT("⛔ `zone_list` is REJECTED"), IsLegalGbnfRuleNameHere(TEXT("zone_list")));
	TestFalse(TEXT("⛔ `at_least` is REJECTED — the original"), IsLegalGbnfRuleNameHere(TEXT("at_least")));
	TestTrue(TEXT("kebab-case is accepted, which is why the RULE is `at-least`"), IsLegalGbnfRuleNameHere(TEXT("at-least")));

	// ⚠️ THE OTHER HALF OF THE ASYMMETRY, SO THE RENAME CANNOT BE "MADE CONSISTENT"
	// IN THE WRONG DIRECTION. Rule names are kebab-case; the JSON KEYS and canonical
	// place symbols carried INSIDE terminals are snake_case wire format and MUST
	// keep their underscores — Zone A's schema, the evaluation corpora and
	// `ParseSiegeAssistantCommand` all assert them. A well-meaning sweep that
	// kebab-cased these would produce a grammar that parses perfectly and emits
	// commands nothing downstream can read.
	{
		const FString Grammar = USiegeAssistantGrammar::Build(Kinds, Places, Regions);
		TestTrue(TEXT("The place symbol `ancient_ground_near` keeps its underscores inside the `zone` terminal"),
			GrammarRuleRhs(Grammar, TEXT("zone")).Contains(TEXT("ancient_ground_near"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("The JSON key `all_except` still keeps its underscore"), Grammar.Contains(TEXT("all_except"), ESearchCase::CaseSensitive));
		TestTrue(TEXT("The JSON key `at_least` still keeps its underscore"), Grammar.Contains(TEXT("at_least"), ESearchCase::CaseSensitive));
	}

	AddInfo(TEXT("⚠️ FLAGGED FOR TASK-550: `Siegebound.Assistant.Grammar.RuleNameCharset` in SiegeAssistantGrammarTest.cpp carries a comment claiming its four grammars are \"every shape the builder can produce\". TASK-546 found that claim became FALSE the moment `Build` gained its third parameter. This test closes the COVERAGE hole; whether that file's comment is also repaired is TASK-550's to rule (see the TASK-549 handoff, DISAGREEMENT 1)."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 24 — ⭐⭐ Siegebound.Assistant.Selection.RegionGrammarCompositionCarriesRegions
//
//  ⛔⛔ THE HAZARD TASK-548 FOUND, WHICH WOULD HAVE SHIPPED THE WHOLE FEATURE DEAD.
//
//  `USiegeAssistantComponent::ComposeTurnGrammar` was the module's ONLY shipped
//  caller of `USiegeAssistantGrammar::Build`, and it OMITTED the new third
//  defaulted argument. That compiled, linked, ran, and produced a grammar with NO
//  `inplace` rule, NO `zone` alternation and NO `in` alternative on `who`.
//  Constrained decoding CANNOT SAMPLE A SHAPE THE GRAMMAR DOES NOT CONTAIN, so
//  `{"in":"ancient_ground_near"}` would have been structurally unreachable at
//  runtime while TASK-544's statics, TASK-545's parser, TASK-546's grammar,
//  TASK-547's snapshot and TASK-548's executor all sat there CORRECT AND UNREACHED.
//
//  ⚠️⚠️ AND ITS NASTIEST PROPERTY IS MISDIAGNOSIS, WHICH IS WHY THIS TEST EXISTS
//  RATHER THAN A COMMENT. `AS-§21.11` outcome 5 already primes every reader to
//  expect that "a rule line is a WEAKER teaching signal than an exemplar for a
//  brand-new output SHAPE" — so "the model never emitted `in`" at Stage 5 would
//  have been read as THE PROMPT UNDER-TEACHING THE SHAPE: a conclusion about the
//  MODEL, drawn from a MISSING FUNCTION ARGUMENT, with a few-shot spend and a
//  sealed-holdout argument queued up behind it.
//
//  ⛔⛔ WHAT THIS TEST HONESTLY IS, AND IS NOT — READ BEFORE RELYING ON IT.
//  `ComposeTurnGrammar` is PRIVATE (`SiegeAssistantComponent.h`, below the
//  `private:` at :1075) and reads the component's `Snapshot` member, which only
//  `CaptureTurnSnapshot()` fills, and that needs a `UWorld` and a resolved team.
//  ⇒ THIS TEST CANNOT CALL IT. What it does instead is (i) MEASURE THE DELTA the
//  third argument buys, and (ii) run the EXACT THREE-EXPRESSION ARGUMENT LIST the
//  shipped caller uses — `GetUnitKinds()`, `GetPlaceNames()`,
//  `GetRegionPlaceNames()` — off a real snapshot object, and assert the composed
//  grammar carries the region alternatives.
//  ⛔ THE RESIDUAL, STATED PLAINLY: a future edit that drops the third argument at
//  that call site AGAIN would compile, run, and leave this suite GREEN. That is
//  TASK-550 criterion (3)'s grep and Jonathan's TASK-552 playtest, ⛔ not this
//  file, and pretending otherwise would be the "guardrail that reports SAFE" shape
//  this batch exists to refuse.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionGrammarCompositionCarriesRegionsTest,
	"Siegebound.Assistant.Selection.RegionGrammarCompositionCarriesRegions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionGrammarCompositionCarriesRegionsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	FScratchSnapshot Scratch = MakeSnapshotWithRegions(ThirteenKindsInCardRowOrder(), /*PerKindTotal*/ 9, /*bPublishRegions*/ true);
	if (!TestTrue(*FString::Printf(TEXT("A snapshot publishing regions was built (missing: %s)"), *Scratch.MissingField), Scratch.IsUsable()))
	{
		return false;
	}

	const USiegeAssistantSnapshot* const Snapshot = Scratch.Snapshot.Get();

	// ── (a) ⭐⭐ THE SHIPPED CALLER'S EXPRESSION, RUN OFF A REAL SNAPSHOT ──────
	// This is `ComposeTurnGrammar`'s return statement with the `Snapshot->` member
	// replaced by a local of the same type. ⛔ If `GetRegionPlaceNames()` ever
	// stopped binding to the parameter — a changed return type, a renamed accessor —
	// this stops compiling, which is the one part of the hazard a compiler CAN see.
	const FString Composed = USiegeAssistantGrammar::Build(
		Snapshot->GetUnitKinds(), Snapshot->GetPlaceNames(), Snapshot->GetRegionPlaceNames());

	TestTrue(TEXT("⭐⭐ The composed grammar DEFINES `inplace` — without it, {\"in\":…} is unsamplable at runtime no matter what the model wants to emit"),
		Composed.Contains(TEXT("inplace ::="), ESearchCase::CaseSensitive));
	TestTrue(TEXT("⭐⭐ …and `zone`"), Composed.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive));
	TestTrue(TEXT("⭐⭐ …and `who` carries the `inplace` alternative, which is the byte the sampler actually needs"),
		GrammarRuleRhs(Composed, TEXT("who")).Contains(TEXT("inplace"), ESearchCase::CaseSensitive));
	TestEqual(TEXT("⭐ `zone` carries one alternative per region the SNAPSHOT published — the list is the snapshot's, never this call's"),
		GrammarAlternatives(GrammarRuleRhs(Composed, TEXT("zone"))).Num(), Snapshot->GetRegionPlaceNames().Num());

	// ── (b) ⭐⭐ THE DELTA THE THIRD ARGUMENT BUYS — THE TEST 11 IDIOM ─────────
	// ⛔ THIS IS THE MEASUREMENT. The two-argument call is EXACTLY what the shipped
	// bug was, and the two grammars must DISAGREE. If a future edit makes them
	// agree — by giving the default a non-empty value, or by emitting the region
	// rules unconditionally — this fires and sends the next reader to the call site
	// instead of to a Stage-5 accuracy table.
	{
		const FString Dropped = USiegeAssistantGrammar::Build(Snapshot->GetUnitKinds(), Snapshot->GetPlaceNames());

		TestFalse(TEXT("⛔ THE TWO-ARGUMENT CALL — the shipped bug, verbatim — produces NO `inplace` rule"),
			Dropped.Contains(TEXT("inplace ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("⛔ …NO `zone` rule"), Dropped.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive));
		TestFalse(TEXT("⛔ …and NO `in` alternative on `who`. The parser, the validator, the snapshot and the executor would all have been correct and UNREACHED."),
			GrammarRuleRhs(Dropped, TEXT("who")).Contains(TEXT("inplace"), ESearchCase::CaseSensitive));

		if (!TestNotEqualSensitive(TEXT("⛔⛔ THE THIRD ARGUMENT IS LOAD-BEARING: dropping it produces a DIFFERENT grammar. If these two ever become equal, the region rules have either become unconditional or become unreachable — and BOTH are silent at runtime."),
			Dropped, Composed))
		{
			AddError(TEXT("⛔ The third-argument hazard has CHANGED SHAPE: Build(2 args) and Build(3 args) now produce IDENTICAL grammars on a board that publishes regions. Re-read USiegeAssistantGrammar::Build's `bHasRegions` gate and re-run `grep -rn \"USiegeAssistantGrammar::Build(\" Source/ --include=*.cpp | grep -v \"/Tests/\"` — there must be exactly ONE shipped call site and it must pass three arguments."));
		}

		AddInfo(FString::Printf(TEXT("MEASURED DELTA: the third argument adds %d characters of grammar on this board (2-arg %d chars, 3-arg %d chars). ⛔ Zero would mean the feature is unsamplable at runtime."),
			Composed.Len() - Dropped.Len(), Dropped.Len(), Composed.Len()));
	}

	// ── (c) THE OTHER DIRECTION — A REGION-FREE SNAPSHOT MUST *NOT* GROW RULES ─
	// ✅ An empty region list is a LEGAL, HANDLED state (a map with no capture zone
	// and no ancient ground), and the builder must then omit the rules ENTIRELY —
	// required, because an empty alternation would leave `zone` undefined and make
	// the WHOLE grammar unparseable. The snapshot decides; the caller only relays.
	{
		FScratchSnapshot Bare = MakeSnapshotWithRegions(ThirteenKindsInCardRowOrder(), /*PerKindTotal*/ 9, /*bPublishRegions*/ false);
		if (TestTrue(*FString::Printf(TEXT("A region-free snapshot was built (missing: %s)"), *Bare.MissingField), Bare.IsUsable()))
		{
			const USiegeAssistantSnapshot* const BareSnapshot = Bare.Snapshot.Get();
			const FString BareComposed = USiegeAssistantGrammar::Build(
				BareSnapshot->GetUnitKinds(), BareSnapshot->GetPlaceNames(), BareSnapshot->GetRegionPlaceNames());
			const FString BareDropped = USiegeAssistantGrammar::Build(
				BareSnapshot->GetUnitKinds(), BareSnapshot->GetPlaceNames());

			TestEqualSensitive(TEXT("⛔ On a map that publishes NO regions the 3-argument composition is BYTE-IDENTICAL to the 2-argument one — the shape is correctly unsamplable, not broken"),
				BareComposed, BareDropped);
			TestFalse(TEXT("⛔ …and no dangling `zone ::=` is left behind, which would make llama.cpp generate UNCONSTRAINED"),
				BareComposed.Contains(TEXT("zone ::="), ESearchCase::CaseSensitive));
		}
	}

	AddInfo(TEXT("⛔ RESIDUAL, STATED: `USiegeAssistantComponent::ComposeTurnGrammar` is PRIVATE and needs a UWorld-backed snapshot, so this test CANNOT observe the call site itself. It measures what the third argument buys and runs the caller's own three-expression argument list. A future edit that drops the argument again would compile, run, and leave this suite GREEN — that gap is TASK-550's grep and Jonathan's TASK-552 playtest, not this file's."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 25 — ⭐⭐ Siegebound.Assistant.Selection.ZoneCIsByteUnchangedByRegions
//
//  ⚖️ THIS IS WHAT TURNS "THE REGION FEATURE COSTS ZERO ZONE-C BUDGET" FROM A
//  PROMISE INTO A TESTED PROPERTY, and it is the reason the whole design was
//  affordable: the model names a region SYMBOL — the identical operation it
//  already performs for `where` — and the geometry never leaves the snapshot
//  except through `ResolvePlaceRegion` into the executor (AS-§3, the coordinate
//  airlock).
//
//  ⭐ THE ASSERTION IS A BYTE COMPARISON BETWEEN TWO SNAPSHOTS THAT DIFFER ONLY IN
//  THEIR REGION STATE. That is stronger than "Zone C looks the same as it used to"
//  — which nothing could check after the batch landed — because it fails the day
//  ANY per-region datum, occupancy count or half-extent reaches the roster block.
//
//  ⛔ AT 13 KINDS AND AT `PerKindTotal` 9 AND 12, because the roster block is
//  ELASTIC: the collapse path is a different code path from the uncollapsed one,
//  and a leak on the collapsed path only would be invisible at 9.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionZoneCIsByteUnchangedByRegionsTest,
	"Siegebound.Assistant.Selection.ZoneCIsByteUnchangedByRegions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionZoneCIsByteUnchangedByRegionsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	// ⚠️ EXPECTED TRAFFIC, NOT A FAILURE: `BuildZoneC` reports every degradation at
	// Warning by design, and the two-digit board below collapses on purpose.
	AddExpectedMessagePlain(TEXT("Snapshot roster TRUNCATED"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	const TArray<FName> Kinds = ThirteenKindsInCardRowOrder();
	const FString Order = TEXT("send everyone in the ancient ground to the enemy castle");

	// The two operating points AS-§20.3 and TASK-549's spec both name: 9 is the
	// single-digit point the shipped headroom figure was derived on; 12 is an
	// ordinary mid-match board, where every roster row is three characters wider.
	const int32 PerKindTotals[] = { 9, 12 };

	for (const int32 PerKindTotal : PerKindTotals)
	{
		FScratchSnapshot WithRegions = MakeSnapshotWithRegions(Kinds, PerKindTotal, /*bPublishRegions*/ true);
		FScratchSnapshot WithoutRegions = MakeSnapshotWithRegions(Kinds, PerKindTotal, /*bPublishRegions*/ false);

		if (!TestTrue(*FString::Printf(TEXT("[PerKindTotal %d] both snapshots were built (missing: '%s' / '%s')"),
				PerKindTotal, *WithRegions.MissingField, *WithoutRegions.MissingField),
			WithRegions.IsUsable() && WithoutRegions.IsUsable()))
		{
			continue;
		}

		// ⛔ THE PRECONDITION, ASSERTED FIRST. If the "with regions" snapshot did not
		// actually publish anything, the byte comparison below is a comparison of
		// two identical inputs and it proves NOTHING — the vacuous-pass shape.
		if (!TestEqual(*FString::Printf(TEXT("⛔ [PerKindTotal %d] the region-bearing snapshot really did publish 3 regions (otherwise the comparison below is vacuous)"), PerKindTotal),
			WithRegions.Snapshot->GetRegionPlaceNames().Num(), 3))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("[PerKindTotal %d] …and the control published none"), PerKindTotal),
			WithoutRegions.Snapshot->GetRegionPlaceNames().Num(), 0);

		const FString ZoneCWith = WithRegions.Snapshot->BuildZoneC(Order, FString());
		const FString ZoneCWithout = WithoutRegions.Snapshot->BuildZoneC(Order, FString());

		// ⭐⭐ THE CLAIM. *Sensitive because it is a claim about the BYTES the
		// tokenizer sees.
		TestEqualSensitive(*FString::Printf(TEXT("⭐⭐ [PerKindTotal %d] ZONE C IS BYTE-IDENTICAL with and without the region data — the region feature costs the prompt NOTHING here, and that is now a tested property rather than a promise"), PerKindTotal),
			ZoneCWith, ZoneCWithout);

		// ── ⛔ THE COORDINATE AIRLOCK, AS A BYTE PROPERTY ─────────────────────
		// The fixture's centres and extents are deliberately unmistakable numbers.
		// ⚠️ THIS IS NOT REDUNDANT WITH THE EQUALITY ABOVE: it names the SPECIFIC
		// failure ("a coordinate reached the prompt") instead of reporting a
		// character offset, and a reader of a red bar needs the former.
		const TCHAR* const ForbiddenFragments[] = { TEXT("91234"), TEXT("75319"), TEXT("1337"), TEXT("8642"), TEXT("4242") };
		for (const TCHAR* Fragment : ForbiddenFragments)
		{
			TestFalse(*FString::Printf(TEXT("⛔ [PerKindTotal %d] Zone C contains no captured coordinate or half-extent (`%s`) — AS-§3: every field the model sees is a SYMBOL, never a number that means a position"), PerKindTotal, Fragment),
				ZoneCWith.Contains(Fragment, ESearchCase::CaseSensitive));
		}

		// ⛔ NOR ANY OCCUPANCY FIGURE. "How many units are standing in the mid" is a
		// per-region datum and printing it would be the same leak in a friendlier
		// costume. There is no `in_region:` / `occupants:` key and there must not be.
		TestFalse(*FString::Printf(TEXT("⛔ [PerKindTotal %d] Zone C has no `in_region:` key"), PerKindTotal),
			HasKeyLine(ZoneCWith, TEXT("in_region")));
		TestFalse(*FString::Printf(TEXT("⛔ [PerKindTotal %d] Zone C has no `occupants:` key"), PerKindTotal),
			HasKeyLine(ZoneCWith, TEXT("occupants")));
		TestFalse(*FString::Printf(TEXT("⛔ [PerKindTotal %d] Zone C has no `regions:` key"), PerKindTotal),
			HasKeyLine(ZoneCWith, TEXT("regions")));

		AddInfo(FString::Printf(TEXT("Zone C at 13 kinds, PerKindTotal %d: %d chars, %d roster row(s) printed, %d collapsed. Byte-identical with and without region data."),
			PerKindTotal, ZoneCWith.Len(), PrintedRosterSymbols(ZoneCWith).Num(), CollapsedSymbols(ZoneCWith).Num()));
	}

	// ── ⭐ AND THE 13-KIND ROSTER STILL BEHAVES AS IT DID ─────────────────────
	// The elastic trimmer is the part of Zone C most likely to be disturbed by a
	// snapshot change, so its shipped operating point is re-asserted rather than
	// assumed intact.
	{
		FScratchSnapshot Scratch = MakeSnapshotWithRegions(Kinds, /*PerKindTotal*/ 9, /*bPublishRegions*/ true);
		if (TestTrue(*FString::Printf(TEXT("A 9-unit board was built (missing: %s)"), *Scratch.MissingField), Scratch.IsUsable()))
		{
			const FString ZoneC = Scratch.Snapshot->BuildZoneC(TEXT("send all units to the middle"), FString());
			TestEqual(TEXT("All 13 kinds still print as FULL roster rows at the single-digit operating point"),
				PrintedRosterSymbols(ZoneC).Num(), 13);
			TestEqualSensitive(TEXT("Nothing collapsed, so `other_kinds:` still reads exactly `none`"),
				ValueOfKey(ZoneC, TEXT("other_kinds")), FString(TEXT("none")));
			TestTrue(TEXT("⭐ `sorcerer` is still visible — TASK-517's fix survives the region batch"),
				ZoneC.Contains(SorcererSymbol, ESearchCase::CaseSensitive));
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 26 — ⭐ Siegebound.Assistant.Selection.ZoneAWhoLineMirrorsGrammar
//
//  AS-§9c, THE MIRROR LAW: Zone A TEACHES the schema, `USiegeAssistantGrammar::
//  Build` CONSTRAINS it, and if they disagree constrained decoding fights the
//  prompt on every token and accuracy collapses for a reason no log line names.
//
//  ⚠️ THERE IS NO COMPILE-TIME LINK BETWEEN THE TWO FILES. Two comments and this
//  test are the whole tie — which is exactly what happened at TASK-518: `except`
//  went into the grammar's `who` rule while Zone A still enumerated three shapes,
//  so the prompt TOLD the model a shape did not exist while the sampler ALLOWED
//  it. It happened AGAIN at TASK-546/547 with `inplace`. ⇒ The order is now
//  written down in both files, and asserted here.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionZoneAWhoLineMirrorsGrammarTest,
	"Siegebound.Assistant.Selection.ZoneAWhoLineMirrorsGrammar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionZoneAWhoLineMirrorsGrammarTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAssistantSelectionTestFixture;

	TStrongObjectPtr<USiegeAssistantSnapshot> Snapshot(NewObject<USiegeAssistantSnapshot>());
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary(NewObject<USiegeAssistantVocabulary>());
	if (!TestTrue(TEXT("A snapshot and a default vocabulary were created"), Snapshot.IsValid() && Vocabulary.IsValid()))
	{
		return false;
	}

	const FString ZoneA = Snapshot->BuildZoneA(Vocabulary.Get());

	// The `WHO    = ` line, read out of the built prompt rather than transcribed.
	FString WhoLine;
	{
		TArray<FString> Lines;
		ZoneA.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);
		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(TEXT("WHO    = "), ESearchCase::CaseSensitive))
			{
				WhoLine = Line;
				break;
			}
		}
	}

	if (!TestTrue(TEXT("Zone A carries a `WHO    = ` schema line"), !WhoLine.IsEmpty()))
	{
		return false;
	}

	// ── (a) THE FIVE SHAPES APPEAR IN THE GRAMMAR'S OWN ORDER ────────────────
	// ⛔ The claim is ORDER, not membership: a set comparison would pass a prompt
	// that listed the right five shapes in the wrong sequence, and the sequence is
	// what AS-§9c pins.
	struct FShape
	{
		const TCHAR* GrammarAlternative;   // as the emitter writes it
		const TCHAR* ZoneAClause;          // as the prose line writes it
	};

	const FShape Shapes[] = {
		{ TEXT("selection"),        TEXT("[{\"kind\":KIND,\"n\":COUNT}]") },
		{ TEXT("except"),           TEXT("{\"all_except\":[KIND]}") },
		{ TEXT("inplace"),          TEXT("{\"in\":ZONE}") },
		{ TEXT("\"\\\"all\\\"\""),  TEXT("\"all\"") },
		{ TEXT("\"\\\"none\\\"\""), TEXT("\"none\"") }
	};

	int32 PreviousIndex = -1;
	for (const FShape& Shape : Shapes)
	{
		const int32 Index = WhoLine.Find(Shape.ZoneAClause, ESearchCase::CaseSensitive);
		if (!TestTrue(*FString::Printf(TEXT("⭐ Zone A's `WHO =` line names the `%s` shape (`%s`)"), Shape.GrammarAlternative, Shape.ZoneAClause),
			Index != INDEX_NONE))
		{
			continue;
		}

		TestTrue(*FString::Printf(TEXT("⭐ …and `%s` comes AFTER the shape before it — the prose order IS the grammar's alternation order (AS-§9c). A set comparison would pass a prompt that listed the right five shapes in the wrong sequence."), Shape.ZoneAClause),
			Index > PreviousIndex);
		PreviousIndex = Index;
	}

	// ── (b) THE GRAMMAR'S OWN `who` ALTERNATION, READ OFF THE REAL EMITTER ────
	{
		const FString Grammar = USiegeAssistantGrammar::Build(
			ThirteenKindsInCardRowOrder(), SevenPlaces(), ThreeRegionPlaces());
		const TArray<FString> WhoAlternatives = GrammarAlternatives(GrammarRuleRhs(Grammar, TEXT("who")));

		if (TestEqual(TEXT("The grammar's `who` offers five alternatives"), WhoAlternatives.Num(), static_cast<int32>(UE_ARRAY_COUNT(Shapes))))
		{
			for (int32 Index = 0; Index < WhoAlternatives.Num(); ++Index)
			{
				TestEqualSensitive(*FString::Printf(TEXT("⭐ Grammar `who` alternative %d is `%s` — the order Zone A's prose mirrors"), Index + 1, Shapes[Index].GrammarAlternative),
					WhoAlternatives[Index], FString(Shapes[Index].GrammarAlternative));
			}
		}
	}

	// ── (c) THE `ZONE = ` METAVARIABLE LINE, AND WHY IT IS THE FIXED VOCABULARY
	// ⚠️ Zone A prints ALL THREE region symbols; the GRAMMAR is what enforces which
	// exist this match. That is the same split the `places` block already ships for
	// `where`, and it is what keeps Zone A byte-frozen (the KV-prefix contract).
	TestTrue(TEXT("⭐ Zone A defines the `ZONE` metavariable it just used in the `WHO =` line — an undefined metavariable is a shape the model can never fill"),
		ZoneA.Contains(TEXT("ZONE   = an area place symbol: "), ESearchCase::CaseSensitive));

	// ⛔ AND THE FOUR NON-REGION PLACES ARE NOT ON IT. Teaching `own_castle` as an
	// area would teach a shape the grammar can never sample — a blocked attempt
	// still costs the whole turn (AS-§21.4: the other four would each need an
	// INVENTED RADIUS, which is Jonathan's call and not a tuner's).
	{
		const int32 ZoneLineStart = ZoneA.Find(TEXT("ZONE   = "), ESearchCase::CaseSensitive);
		int32 ZoneLineEnd = INDEX_NONE;
		if (ZoneLineStart != INDEX_NONE)
		{
			ZoneLineEnd = ZoneA.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ZoneLineStart);
		}

		if (TestTrue(TEXT("The `ZONE =` line is terminated"), ZoneLineStart != INDEX_NONE && ZoneLineEnd != INDEX_NONE))
		{
			const FString ZoneLine = ZoneA.Mid(ZoneLineStart, ZoneLineEnd - ZoneLineStart);
			const TCHAR* const NonRegions[] = { TEXT("own_castle"), TEXT("enemy_castle"), TEXT("nearest_mine"), TEXT("hero") };
			for (const TCHAR* Place : NonRegions)
			{
				TestFalse(*FString::Printf(TEXT("⛔ `%s` is NOT offered as an area — it has no region primitive and would need an invented radius (AS-§21.4)"), Place),
					ZoneLine.Contains(Place, ESearchCase::CaseSensitive));
			}

			TestEqualSensitive(TEXT("⭐ The whole `ZONE =` line, character for character — generated from PlaceVocabulary's `bHasRegion` column, in fixed vocabulary order"),
				ZoneLine, FString(TEXT("ZONE   = an area place symbol: mid, ancient_ground_near, ancient_ground_far")));
		}
	}

	// ── (d) THE TEACHING RULE THAT MAKES THE TWO PLACE-VALUED KEYS DISTINGUISHABLE
	// ⭐ Jonathan's sentence B fills BOTH `who` and `where` from the SAME seven-symbol
	// vocabulary, and before this line nothing in the prompt said which key takes
	// which — a coin flip on the exact distinction the feature exists for.
	TestTrue(TEXT("⭐ Zone A carries the in-vs-where rule, and it says BOTH-IN-ONE-ORDER explicitly"),
		ZoneA.Contains(TEXT("- Units already in a place: who is {\"in\":ZONE}, where is still where they go, and one order may set both. Only with send, guard, ambush or follow.\n"), ESearchCase::CaseSensitive));

	// ⚠️ THE DECLARED OMISSION, PINNED SO IT IS NOT "FIXED" WITHOUT RE-COUNTING.
	// The exclusion rule directly above carries BOTH halves; this one carries only
	// the positive half, because the negative half measures 52 more characters and
	// the batch's Zone-A ceiling is +250 against a spend that is already 239.
	// ⇒ The residual is bounded: a region on charge/fallback/rally is REFUSED BY THE
	// PARSER, never silently dropped, so the cost is a refusal the player SEES.
	TestFalse(TEXT("⚠️ The in-vs-where rule deliberately does NOT carry the negative half (`On charge, fallback or rally: {\"ask\":\"unsupported\"}`) — 52 chars the +250 ceiling does not have. Adding it needs a re-count, not a nudge."),
		ZoneA.Contains(TEXT("one order may set both. Only with send, guard, ambush or follow. On charge"), ESearchCase::CaseSensitive));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 27 — ⭐ Siegebound.Assistant.Selection.DefendNoteScoped
//
//  AS-§21.2, AND THE DELIVERABLE IS THE DIFF. Two shipped prompt lines asserted
//  contradictory things about the same input class: the `[notes]` row said
//  "defend = ambiguous between guard and fallback", conditioned on the harm that
//  "picking one would move an army the player never mentioned" — and a NEWER,
//  UNCONDITIONAL rule ("If the player names units, the intent is send, guard,
//  ambush or follow, never charge, fallback or rally") had already made that harm
//  STRUCTURALLY IMPOSSIBLE whenever a selection is present.
//
//  ⚖️ THE GENERAL SHAPE, WHICH IS THE PART WORTH KEEPING: the note was TRUE WHEN
//  WRITTEN; a later line elsewhere in the prompt falsified its antecedent; nobody
//  re-read it. ⇒ A prompt line is not a constant — it is an ASSERTION ABOUT THE
//  REST OF THE PROMPT.
//
//  ⛔⛔ NO ACCURACY ASSERTION OF ANY KIND APPEARS HERE (AS-§12f). Not a predicted
//  score, not a row list, not a percentage. Nobody has ever seen what the model
//  emits for either wording; TASK-542's raw-output log is what will show it. The
//  whole claim is the diff, and the diff is what is asserted.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionDefendNoteScopedTest,
	"Siegebound.Assistant.Selection.DefendNoteScoped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionDefendNoteScopedTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<USiegeAssistantVocabulary> Vocabulary(NewObject<USiegeAssistantVocabulary>());
	if (!TestTrue(TEXT("A default vocabulary was created"), Vocabulary.IsValid()))
	{
		return false;
	}

	const FString Table = Vocabulary->BuildSynonymTable();

	// The two clauses, as their own literals so the compiler measures them.
	static const TCHAR* const OldClause = TEXT("defend = ambiguous between guard and fallback -> ask which_intent.");
	static const TCHAR* const NewClause = TEXT("defend with units -> guard. defend alone -> ask which_intent.");

	// ── (a) THE CONTRADICTION IS GONE ────────────────────────────────────────
	TestFalse(TEXT("⭐ The `[notes]` line NO LONGER contains `ambiguous between guard and fallback`"),
		Table.Contains(TEXT("ambiguous between guard and fallback"), ESearchCase::CaseSensitive));
	TestFalse(TEXT("⛔ …not even as a fragment: the whole old clause is absent"),
		Table.Contains(OldClause, ESearchCase::CaseSensitive));

	// ── (b) AND THE REPLACEMENT IS THERE, IN HOUSE ARROW NOTATION ────────────
	TestTrue(TEXT("⭐ The repaired clause is present: `defend with units -> guard. defend alone -> ask which_intent.`"),
		Table.Contains(NewClause, ESearchCase::CaseSensitive));
	TestTrue(TEXT("The frozen clause AHEAD of it is byte-identical — nothing outside the `defend` sentence moved"),
		Table.Contains(TEXT("send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend with units -> guard."), ESearchCase::CaseSensitive));

	// ── (c) ⭐ EXACTLY 5 CHARACTERS SHORTER — MEASURED TWO WAYS ──────────────
	// (i) the two literals against each other, which the compiler measures; and
	// (ii) the WHOLE TABLE with the old clause restored, which measures the edit as
	//      it lands rather than in isolation.
	// ⛔ NEITHER WAY COPIES THE BUILDER'S OUTPUT INTO A FIXTURE. A test whose
	// expectation is transcribed from its own subject is a guardrail that reports
	// SAFE (AS-§12g).
	{
		const int32 LiteralDelta = FCString::Strlen(NewClause) - FCString::Strlen(OldClause);
		TestEqual(TEXT("⭐ The clause is exactly 5 characters shorter — CHAR-NEGATIVE, so it spends none of the batch's Zone-A budget"),
			LiteralDelta, -5);

		FString Restored = Table;
		const bool bRestored = Restored.Contains(NewClause, ESearchCase::CaseSensitive);
		if (TestTrue(TEXT("The new clause was found in the live table, so the restoration below is meaningful"), bRestored))
		{
			Restored.ReplaceInline(NewClause, OldClause, ESearchCase::CaseSensitive);
			TestEqual(TEXT("⭐ Restoring the OLD clause makes the WHOLE synonym table exactly 5 characters longer — the edit measured where it actually lands"),
				Restored.Len() - Table.Len(), 5);
		}
	}

	// ── (d) ⛔ `defend` DID NOT BECOME AN EIGHTH INTENT ──────────────────────
	// AS-§21.3 REJECTED that with its reason recorded so it is not re-proposed: it
	// would be the ONLY intent carrying an implicit `where` (defend ⇒ your own
	// castle), giving TWO SPELLINGS FOR ONE ORDER — `{"intent":"defend","where":
	// "none"}` and `{"intent":"guard","where":"own_castle"}` meaning the identical
	// thing — which is precisely the ambiguity the seven-verb split exists to
	// remove. ⇒ A 5-character deletion and an 8th enum value are not two options at
	// different sizes; they are a repair and a schema change.
	{
		FSiegeAssistantCommand Command;
		FString Error;
		TestFalse(TEXT("⛔ `\"intent\":\"defend\"` is still REFUSED — the repair is a prompt-line diff, NOT an eighth intent (AS-§21.3)"),
			ParseSiegeAssistantCommand(
				TEXT("{\"intent\":\"defend\",\"who\":\"all\",\"where\":\"own_castle\",\"when\":\"now\"}"), Command, Error));
		TestTrue(TEXT("…and the command is left fully reset"), Command.Intent == ESiegeAssistantIntent::None);
	}

	// ── (e) THE TABLE IS OTHERWISE INTACT ───────────────────────────────────
	// The `[notes]` block's other three rows are the neighbours most likely to be
	// disturbed by an edit located by substring rather than by line.
	TestTrue(TEXT("The wizard/sorcerer note is untouched"),
		Table.Contains(TEXT("wizard != sorcerer."), ESearchCase::CaseSensitive));
	TestTrue(TEXT("The archer/longbowman note is untouched"),
		Table.Contains(TEXT("archer != longbowman."), ESearchCase::CaseSensitive));
	TestTrue(TEXT("The place-symbol note is untouched"),
		Table.Contains(TEXT("use only the place symbols listed in the state block."), ESearchCase::CaseSensitive));

	// ⛔⛔ THE ONE THING THIS TEST DELIBERATELY DOES NOT DO.
	AddInfo(TEXT("⛔ AS-§12f: NO accuracy figure is asserted, predicted or implied here, and none may be added. The deliverable is the DIFF — two shipped prompt lines asserted contradictory things about the same input class, and one of them is gone at NEGATIVE character cost. What the model emits for \"send all units except miners to defend the castle\" has never been observed; TASK-542's raw-output log is what will finally show it (AS-§21.8)."));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 28 — ⭐ Siegebound.Assistant.Selection.RegionRenderedInPlayerSummary
//
//  `USiegeAssistantComponent::DescribeCommandForPlayer` — the sentence the player
//  is asked to ACCEPT.
//
//  ⚖️ THE TASK-522 ARGUMENT, FOR THE SECOND TIME: a confirm prompt that describes
//  a DIFFERENT ORDER FROM THE ONE THAT WILL EXECUTE defeats the confirm step's
//  entire purpose. Before TASK-548, a region order rendered as a WHOLE-ARMY
//  sentence — `Send (enemy_castle)` — and the player would have accepted an order
//  nobody gave. ⚠️ And the ghost-circle preview structurally cannot carry it:
//  `SpawnConfirmPreview` draws two PLACE decals at the destination and nothing
//  per-unit, so no preview geometry depends on WHICH units were selected. ⇒ THIS
//  SENTENCE IS THE UNIT-FACING HALF OF THE REVIEW.
//
//  ✅ REACHABILITY WAS ANSWERED BY TASK-548 RATHER THAN GUESSED (its handoff §5):
//  the function is `public`, `const`, not a `UFUNCTION`, and its body touches no
//  member, no `UWorld`, no snapshot and no actor — so `NewObject` on the transient
//  package is enough. The component is held in a `TStrongObjectPtr` so a GC pass
//  mid-test cannot collect it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAssistantSelectionRegionRenderedInPlayerSummaryTest,
	"Siegebound.Assistant.Selection.RegionRenderedInPlayerSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAssistantSelectionRegionRenderedInPlayerSummaryTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<USiegeAssistantComponent> Component(NewObject<USiegeAssistantComponent>());
	if (!TestTrue(TEXT("A throwaway assistant component was created on the transient package (no world, no actor, no RegisterComponent — TASK-548 §5)"), Component.IsValid()))
	{
		return false;
	}

	auto Describe = [&Component](ESiegeAssistantIntent Intent, const TCHAR* Region, const TCHAR* Where) -> FString
	{
		FSiegeAssistantCommand Command;
		Command.Intent = Intent;
		Command.RegionPlace = Region ? FName(Region) : NAME_None;
		Command.Where = Where ? FName(Where) : NAME_None;
		return Component->DescribeCommandForPlayer(Command).ToString();
	};

	// ── (a) ⭐⭐ THE THREE RENDERINGS TASK-548 PINNED ─────────────────────────
	// ⛔ TestEqualSensitive throughout: `FAutomationTestBase::TestEqual(const
	// FString&, const FString&)` forwards to the TCHAR* overload, which is
	// CASE-INSENSITIVE (SC-§13). These are the bytes a player reads.
	TestEqualSensitive(TEXT("⭐⭐ Jonathan's sentence B renders in full: `Send all in ancient_ground_near (enemy_castle)` — before TASK-548 it read `Send (enemy_castle)`, a whole-army sentence for a filtered order"),
		Describe(ESiegeAssistantIntent::Send, TEXT("ancient_ground_near"), TEXT("enemy_castle")),
		FString(TEXT("Send all in ancient_ground_near (enemy_castle)")));

	TestEqualSensitive(TEXT("⭐ `Guard all in mid (own_castle)`"),
		Describe(ESiegeAssistantIntent::Guard, TEXT("mid"), TEXT("own_castle")),
		FString(TEXT("Guard all in mid (own_castle)")));

	TestEqualSensitive(TEXT("⭐ A region with NO destination renders through the selection-only frame: `Follow all in ancient_ground_far`"),
		Describe(ESiegeAssistantIntent::Follow, TEXT("ancient_ground_far"), nullptr),
		FString(TEXT("Follow all in ancient_ground_far")));

	// ── (b) ⛔ NO REGION ⇒ BYTE-IDENTICAL TO WHAT SHIPPED ────────────────────
	// The clause is inside `if (Command.RegionPlace != NAME_None)`, so every shipped
	// rendering is untouched. Asserted as behaviour rather than read off the diff
	// (SC-§18: "additive" is a claim about BEHAVIOUR, not about diff arithmetic).
	TestEqualSensitive(TEXT("⛔ A place-only order is unchanged: `Guard (own_castle)`"),
		Describe(ESiegeAssistantIntent::Guard, nullptr, TEXT("own_castle")),
		FString(TEXT("Guard (own_castle)")));
	{
		FSiegeAssistantCommand Command;
		Command.Intent = ESiegeAssistantIntent::Send;
		Command.Kinds.Add(FName(TEXT("footman")));
		Command.Counts.Add(8);
		Command.Where = FName(TEXT("mid"));

		TestEqualSensitive(TEXT("⛔ A positive selection is unchanged: `Send 8 footman (mid)`"),
			Component->DescribeCommandForPlayer(Command).ToString(), FString(TEXT("Send 8 footman (mid)")));
	}
	{
		FSiegeAssistantCommand Command;
		Command.Intent = ESiegeAssistantIntent::Send;
		Command.ExcludeKinds.Add(FName(TEXT("miner")));
		Command.Where = FName(TEXT("mid"));

		TestEqualSensitive(TEXT("⛔ An exclusion is unchanged: `Send all except miner (mid)`"),
			Component->DescribeCommandForPlayer(Command).ToString(), FString(TEXT("Send all except miner (mid)")));
	}
	{
		FSiegeAssistantCommand Command;   // Intent::None
		TestEqualSensitive(TEXT("⛔ An empty command still renders as the empty string"),
			Component->DescribeCommandForPlayer(Command).ToString(), FString());
	}

	// ── (c) THE UNREACHABLE BRANCHES ARE STILL CORRECT ──────────────────────
	// ⚠️ A region beside a selection or an exclusion is refused by the parser, by
	// SiegeAssistantValidateSelection AND by the selector's own backstop — but a
	// DESCRIBER that DROPS a filter it was handed prints a reassuring sentence about
	// an order nobody gave, and this function is called from paths (LastMessage
	// re-seeding, the deferred summary) that do NOT re-run those gates. So the
	// branch is written, and it is asserted.
	{
		FSiegeAssistantCommand Command;
		Command.Intent = ESiegeAssistantIntent::Send;
		Command.Kinds.Add(FName(TEXT("footman")));
		Command.Counts.Add(8);
		Command.RegionPlace = FName(TEXT("mid"));
		Command.Where = FName(TEXT("enemy_castle"));

		TestEqualSensitive(TEXT("⚠️ The UNREACHABLE region-plus-selection branch still names the region: `Send 8 footman, in mid (enemy_castle)` — a describer that dropped it would print a reassuring sentence about an order nobody gave"),
			Component->DescribeCommandForPlayer(Command).ToString(),
			FString(TEXT("Send 8 footman, in mid (enemy_castle)")));
	}
	{
		FSiegeAssistantCommand Command;
		Command.Intent = ESiegeAssistantIntent::Send;
		Command.ExcludeKinds.Add(FName(TEXT("miner")));
		Command.RegionPlace = FName(TEXT("mid"));
		Command.Where = FName(TEXT("enemy_castle"));

		TestEqualSensitive(TEXT("⚠️ …and the region-plus-exclusion branch: `Send all except miner, in mid (enemy_castle)`"),
			Component->DescribeCommandForPlayer(Command).ToString(),
			FString(TEXT("Send all except miner, in mid (enemy_castle)")));
	}

	// ── (d) THE SYMBOL IS PRINTED RAW, DELIBERATELY ─────────────────────────
	// A display-name table would be a SECOND source of truth for a name the place
	// vocabulary already owns, and the log the player can see prints the same
	// symbol. Pinned so a "prettifier" pass has to delete an assertion rather than
	// merely overlook a comment.
	TestEqualSensitive(TEXT("The region symbol is printed RAW (`ancient_ground_near`, not \"the near ancient ground\") — one source of truth, as the exclusion already does"),
		Describe(ESiegeAssistantIntent::Ambush, TEXT("ancient_ground_near"), nullptr),
		FString(TEXT("Ambush all in ancient_ground_near")));

	// ⛔ THE PRESENTATION LAYER DOES NOT REPAIR ITS INPUT (SC-§31): it describes
	// what WILL RUN, including a region that will resolve to nobody. The refusal and
	// its arithmetic belong to the selector.
	TestEqualSensitive(TEXT("⛔ An UNRESOLVABLE region is still described, not softened — the refusal belongs to the selector, which owns the arithmetic and the log (SC-§31)"),
		Describe(ESiegeAssistantIntent::Send, TEXT("atlantis"), TEXT("mid")),
		FString(TEXT("Send all in atlantis (mid)")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
