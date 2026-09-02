// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// FVector2D is held BY VALUE below and FString is RETURNED BY VALUE by MakeSymbol, so both
// need COMPLETE types here; CoreMinimal.h carries both and is the WHOLE include cost of this
// file. ⭐ That cheapness IS the file's reason to exist (`MARK-§5`): BOTH `UWarMapWidget` and
// `USiegeAssistantSnapshot` include this header, so it must stay as light as the shipped
// pure-data precedents `TeamId.h` and `UnitCommand.h` — and must never pull either heavy
// header toward the other.
#include "CoreMinimal.h"

/**
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *  ONE MAP MARK — the numbered circle the player drew on his own war map
 *  (MARKS batch, TASK-744. Law: `MARK-§0`..`MARK-§6`. QA gate: TASK-753. Compile + suite
 *   gate: TASK-754.)
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *
 *  Jonathan's directive, verbatim, because it IS the spec: *"click on anywhere on the map, to
 *  create a circle on the map at that location… Every time you make a new circle, it gets its
 *  own number in the middle of it… This number system will also us to then communicate with
 *  the AI about where exactly we want units to be. For example, I can make 3 different circles
 *  and then tell the commander something like "move all units to hold 1" or "move all units to
 *  ambush 2", and the AI can use that indicated circle on the map to carry out the command."*
 *
 *  ⭐⭐ THIS IS ⛔ NOT A UI TYPE. IT IS A REFERENT IN THE AI COMMAND GRAMMAR (`MARK-§0`), and
 *      that is why it is a shared pure-data header rather than a member of the widget: the
 *      widget DRAWS one, the snapshot PUBLISHES one, and neither may own the definition.
 *
 *  ── THE THREE CONSUMERS OF THIS TYPE, so a later reader knows who breaks when it changes ──
 *    • TASK-745 `UWarMapWidget`         — places, resizes, deletes, draws `Number` in the centre.
 *    • TASK-746 `USiegeAssistantSnapshot` — publishes `MakeSymbol(Number)` into the per-match
 *      place list and answers for it in `ResolvePlace` (this is the whole AI half of the feature).
 *    • TASK-751 `FSiegeControlsHelpRegistry` — the help rows that must distinguish these
 *      circles from the world-space GROUP-PICK circles (`MARK-§4`'s named confusion hazard).
 *
 *  ⛔⛔ THE AIRLOCK, RE-ASSERTED HERE BECAUSE THIS IS THE FILE THAT HOLDS THE COORDINATES
 *      (`MARK-§5`, `WR-§6`): ⛔ NO coordinate, radius, dot, count or marker geometry may ever
 *      enter a prompt zone. The ONLY thing about a mark that a prompt is ever allowed to see
 *      is the SYMBOL STRING below. `WorldXY` and `RadiusUU` are resolved on the GAME side, in
 *      exactly the way `nearest_mine` resolves to a world position today without a single
 *      coordinate ever being printed.
 *
 *  ⛔ PLAIN STRUCT, ⛔ NOT A `USTRUCT` — and that is the PINNED shape (TASKBOARD registry), not
 *  an omission. It is never replicated, never saved and never seen by Blueprint; the shipped
 *  `FSiegeWarMapMarker` (WarMapWidget.h:200) is the same call made for the same reasons.
 *  ⇒ CONSEQUENCE A READER MUST KNOW: a `TArray<FSiegeMapMark>` can ⛔ NOT be a `UPROPERTY`.
 *    That is harmless HERE — this struct holds no `UObject` pointer of any kind, so there is
 *    nothing for the GC to keep alive — and it is why `USiegeMapMarkSubsystem::Marks` is a
 *    plain member. ⚠️ Adding a `UObject*` field to this struct silently breaks that argument.
 *
 *  M8 DECLARATION (verbatim, `MARK-§6`): adds no replicated property, no new replicated class,
 *  no RPC and no new relevancy tier. ✅ Structural, ⛔ not disciplinary: marks live on a
 *  `ULocalPlayerSubsystem` (client-local by construction) and `M-3` rules them NOT
 *  enemy-visible. ⚠️ If `M-3` is ever overruled, this declaration is void and the feature
 *  acquires a replication design.
 */
struct FSiegeMapMark
{
	int32     Number   = 0;             // 1..MaxMapMarks. PERMANENT IDENTITY, never a list position (MARK-§ M-1).
	FVector2D WorldXY  = FVector2D::ZeroVector;   // arena world X/Y. NEVER printed into any prompt zone (MARK-§5 airlock).
	float     RadiusUU = 0.f;           // world-space radius the mark denotes.

	/** "circle_1". ⛔ NOT a bare digit — Zone A already ships `COUNT = 1 to 30` (MARK-§2).
	 *  A pure static on purpose: the ONE seam the widget and the snapshot must agree on,
	 *  and it is assertable by string equality with no world. */
	static FString MakeSymbol(int32 InNumber);

	/**
	 *  The lowest number a mark may ever carry. ⭐ ONE named constant rather than two literal
	 *  `1`s, because the allocator in `USiegeMapMarkSubsystem` and the validity guard in
	 *  `MakeSymbol` below MUST agree about where numbering starts: if they ever disagreed, the
	 *  store would hand out a number for which the symbol seam returns the empty string, and
	 *  the player would get a circle he cannot name to the commander.
	 *
	 *  ⛔ 1, ⛔ never 0 — the number is drawn in the circle for a human to read and spoken
	 *  aloud in an order ("hold 1"), and no player counts his own circles from zero.
	 */
	static constexpr int32 FirstMarkNumber = 1;
};

/**
 *  ⭐ HEADER-ONLY BY DESIGN (`MARK-§5`, the `TeamId.h` precedent): there is ⛔ NO
 *  `SiegeMapMark.cpp`, so the definition is `inline` right here. A test, the widget and the
 *  snapshot all get the SAME function with no link step and no module boundary in between.
 *
 *  ⛔ THE SPELLING IS THE CONTRACT AND IT IS ASSERTED BY BYTE-SENSITIVE STRING EQUALITY
 *     (`Siegebound.MapMarks.SymbolIsCircleUnderscoreNumber`, and `SC-§13`: `TestEqual` on two
 *     FStrings compares case-INSENSITIVELY, so a byte claim made with it is vacuous):
 *       lowercase `circle`, ONE underscore, the decimal number, nothing else.
 *     It matches the shipped underscore family (`own_castle`, `ancient_ground_near`,
 *     `nearest_mine`) and it matches Jonathan's own word for the thing.
 *
 *  ⛔ AN INVALID NUMBER RETURNS THE EMPTY STRING, ⛔ NEVER `circle_0` OR `circle_-3`, and the
 *     reason is this project's ENTIRE measured failure history — *valid-shaped-wrong-command*.
 *     A `circle_0` would look exactly like a real place symbol in the player's input box, sample
 *     legally out of any grammar that was built from a list containing it, and then resolve to
 *     nothing. An empty string is visibly nothing at every one of those three stations.
 *
 *  ⚠️ IT DOES ⛔ NOT ENFORCE THE UPPER BOUND, AND THAT IS DELIBERATE: the cap lives on
 *     `USiegeMapMarkSubsystem::MaxMapMarks`, which is `EditDefaultsOnly` precisely so Jonathan
 *     can retune it. A second copy of the cap here would be a number that silently drifts out
 *     of agreement with the one that actually refuses the add. `MakeSymbol(10)` therefore
 *     returns `"circle_10"` — a well-formed string that names no mark, which the store's
 *     lookup answers with "no such mark", which is the correct and observable outcome.
 *     ⚠️ Note also `M-5`'s third reason for the cap of 9: a SPOKEN order stays unambiguous at
 *     one digit ("hold 1", never "hold 11").
 *
 *  ⭐ THE SANCTIONED WAY TO GO BACKWARDS (symbol → mark), FOR TASK-746's `ResolvePlace`:
 *     compare an incoming symbol against `MakeSymbol(Mark.Number)` for each of the (at most 9)
 *     marks. ⛔ Do ⛔ NOT write a parser. A parser is a SECOND spelling of this rule, and the
 *     day the two disagree the map inserts one string while the AI resolves another — the
 *     exact drift this pure static exists to make impossible.
 */
inline FString FSiegeMapMark::MakeSymbol(int32 InNumber)
{
	if (InNumber < FirstMarkNumber)
	{
		return FString();
	}

	return FString::Printf(TEXT("circle_%d"), InNumber);
}
