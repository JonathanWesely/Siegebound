// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeMapMarkSubsystem.h"

/**
 *  ⛔ THE ONLY THING THIS FILE EVER PRINTS IS NUMBERS AND COUNTS — ⛔ never a coordinate and
 *  never a radius. A log line is not a prompt zone, so this is stricter than `MARK-§5`'s
 *  airlock requires; it is written that way on purpose, so that "does any string produced by
 *  this class contain arena geometry?" has ONE answer (no) rather than a case analysis.
 */
DEFINE_LOG_CATEGORY(LogSiegeMapMark);

bool USiegeMapMarkSubsystem::AddMark(const FVector2D& WorldXY, float RadiusUU, FSiegeMapMark& OutMark)
{
	// ⛔ EXACTLY ONE REFUSAL GATE, AND IT IS THE ALLOCATOR ITSELF. A second, independent
	// `Marks.Num() >= MaxMapMarks` check would be a rule that CAN DISAGREE with the one that
	// hands out numbers (they diverge the moment a number outside [1, MaxMapMarks] exists for
	// any reason), and two rules that can disagree about the same question is how a cap ships
	// off-by-one. FindLowestFreeNumber answers INDEX_NONE for "full", and that is the cap.
	const int32 Number = FindLowestFreeNumber();

	if (Number == INDEX_NONE)
	{
		// ⛔ REFUSE, AND ⛔ MUTATE NOTHING — `OutMark` is deliberately LEFT UNTOUCHED so the
		// caller's own value survives a refused add (the pinned contract, and it is asserted).
		// ⚠️ THE CALLER MUST SHOW A STATUS LINE NAMING THE CAP (`M-5`, the shipped refusal
		// doctrine): TASK-745 owns that line. ⛔ A silent no-op here would be the defect — the
		// player would click, see nothing appear, and have no way to learn why.
		UE_LOG(LogSiegeMapMark, Log,
			TEXT("AddMark REFUSED: the map already holds the maximum of %d marks (MaxMapMarks). Nothing was added and no number was consumed."),
			MaxMapMarks);

		return false;
	}

	FSiegeMapMark NewMark;
	NewMark.Number   = Number;
	NewMark.WorldXY  = WorldXY;
	NewMark.RadiusUU = SanitizeRadius(RadiusUU);

	// The ascending-by-Number invariant, maintained by INSERTION rather than by a sort: the
	// array is at most 9 long, this runs once per click, and an insert cannot reorder anything
	// it did not need to (a `Sort` with a sloppy predicate could).
	int32 InsertIndex = 0;
	while (InsertIndex < Marks.Num() && Marks[InsertIndex].Number < NewMark.Number)
	{
		++InsertIndex;
	}

	Marks.Insert(NewMark, InsertIndex);

	OutMark = NewMark;

	UE_LOG(LogSiegeMapMark, Verbose, TEXT("AddMark: mark %d added (%d live)."), Number, Marks.Num());

	return true;
}

bool USiegeMapMarkSubsystem::RemoveMark(int32 Number)
{
	// ⛔ BY NUMBER, ⛔ NEVER BY INDEX. `RemoveMark(0)` must be a REFUSAL, not "delete the first
	// one" — an index-based implementation would compile, look right, and delete the wrong
	// circle. That confusion is asserted against directly
	// (`Siegebound.MapMarks.RemoveRefusesUnknownNumbers`).
	const int32 Index = Marks.IndexOfByPredicate(
		[Number](const FSiegeMapMark& Mark) { return Mark.Number == Number; });

	if (Index == INDEX_NONE)
	{
		return false;
	}

	// ⛔⛔ THE HOLE IS THE WHOLE POINT (`M-1`). `RemoveAt` shifts INDICES — it does not touch
	// any surviving mark's `Number`, and ⛔ NOTHING BELOW THIS LINE MAY EITHER. If you are
	// reading this because you were about to add a renumbering loop "so the circles read 1, 2,
	// 3 again", read `FindLowestFreeNumber`'s comment first: a symbol already composed and
	// sitting unsent in the player's input box would silently start denoting different ground.
	// The ascending-by-Number invariant survives a removal untouched.
	Marks.RemoveAt(Index);

	UE_LOG(LogSiegeMapMark, Verbose,
		TEXT("RemoveMark: mark %d removed; its number is now a HOLE and stays free for reuse (%d live)."),
		Number, Marks.Num());

	return true;
}

bool USiegeMapMarkSubsystem::SetMarkRadius(int32 Number, float NewRadiusUU)
{
	FSiegeMapMark* const Mark = Marks.FindByPredicate(
		[Number](const FSiegeMapMark& Candidate) { return Candidate.Number == Number; });

	if (Mark == nullptr)
	{
		return false;
	}

	// ⚠️ CLAMPING IS ⛔ NOT A REFUSAL, and the return value says so: `true` means "the mark
	// exists and its radius now reflects your request, held inside [Min, Max]". A wheel notch
	// against the stop is a legal, expected, silent event — it must not surface to the player
	// as a failure, and TASK-745's own widget-space min/max are what give it feel.
	Mark->RadiusUU = SanitizeRadius(NewRadiusUU);

	return true;
}

const TArray<FSiegeMapMark>& USiegeMapMarkSubsystem::GetMarks() const
{
	// Ascending by `Number`, holes included and visible (1, 3, 4 reads as "2 was deleted").
	return Marks;
}

void USiegeMapMarkSubsystem::ClearMarks()
{
	// ⭐ NUMBERING RESTARTS AT 1 AFTERWARDS, AND IT DOES SO STRUCTURALLY: this class holds NO
	// monotonic counter anywhere — the next number is DERIVED from what is live
	// (FindLowestFreeNumber), so emptying the array IS resetting the allocator. ⛔ A "next id"
	// member would have to be reset here too, and the day someone forgot, the first circle of
	// the new match would be `circle_4`.
	const int32 PreviousCount = Marks.Num();

	Marks.Reset();

	UE_LOG(LogSiegeMapMark, Verbose, TEXT("ClearMarks: %d mark(s) cleared; numbering restarts at %d."),
		PreviousCount, FSiegeMapMark::FirstMarkNumber);
}

const FSiegeMapMark* USiegeMapMarkSubsystem::FindMark(int32 Number) const
{
	return Marks.FindByPredicate(
		[Number](const FSiegeMapMark& Mark) { return Mark.Number == Number; });
}

int32 USiegeMapMarkSubsystem::FindLowestFreeNumber() const
{
	// ⛔ LOWEST FREE, ⛔ NEVER `Marks.Num() + 1` AND ⛔ NEVER `Highest + 1`. With marks 1 and 3
	// live, this returns 2 — the hole the player left — and both of those "obvious"
	// alternatives would return 3 or 4 instead, silently skipping it. See the header comment
	// for why the hole exists at all (the airlock, not ergonomics).
	for (int32 Candidate = FSiegeMapMark::FirstMarkNumber; Candidate <= MaxMapMarks; ++Candidate)
	{
		const bool bTaken = Marks.ContainsByPredicate(
			[Candidate](const FSiegeMapMark& Mark) { return Mark.Number == Candidate; });

		if (!bTaken)
		{
			return Candidate;
		}
	}

	// Every number in [FirstMarkNumber, MaxMapMarks] is taken ⇒ the store is FULL. This is
	// also the answer when `MaxMapMarks` has been retuned to 0 or below, which correctly
	// refuses every add rather than handing out a number the cap does not admit.
	return INDEX_NONE;
}

float USiegeMapMarkSubsystem::SanitizeRadius(float InRadiusUU) const
{
	if (!FMath::IsFinite(InRadiusUU))
	{
		// ⛔ NaN / ±Inf NEVER REACHES A MARK, AND THE GUARD IS ⛔ NOT DECORATIVE — read at the
		// engine source (UnrealMathUtility.h:592-595, UE 5.8): `Clamp(X, Min, Max)` is
		// `Max(Min(X, Max), Min)`, and every comparison against a NaN is false, so `Min(NaN, Max)`
		// yields Max ⇒ ⚠️ A NaN WOULD CLAMP **UP TO THE MAXIMUM** — the single worst outcome
		// this function has (a circle that swallows the arena). It would then travel to a Slate
		// paint call and to a world-space resolution in the snapshot.
		return MinMarkRadiusUU;
	}

	// ⚠️ ON A MIS-RETUNE WHERE Min > Max, THE MINIMUM WINS — same source: the low bound is
	// applied LAST — and that is the direction this should fail: a too-small circle is visible
	// and harmless, a circle that swallows the arena makes every order mean "everywhere".
	return FMath::Clamp(InRadiusUU, MinMarkRadiusUU, MaxMarkRadiusUU);
}
