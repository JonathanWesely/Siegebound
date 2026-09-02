// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// FSiegeMapMark — the ELEMENT TYPE of the TArray<> member below (held BY VALUE) and the type
// returned by reference from GetMarks(), so the COMPLETE type is required here, not a forward
// declaration (complete-type include law). ⭐ It is deliberately the cheapest header in the
// module to include; see its own comment for why (`MARK-§5`).
#include "Siegebound/SiegeMapMark.h"

#include "Subsystems/LocalPlayerSubsystem.h"

#include "SiegeMapMarkSubsystem.generated.h"

/**
 *  The map-marks log category — the standing `LogSiege<Domain>` law (CONVENTIONS "Logging
 *  (C++)"). Declared here, defined in `SiegeMapMarkSubsystem.cpp`.
 *
 *  ⚠️ ITS ONE JOB THAT MATTERS: the CAP REFUSAL is logged at `Log` verbosity, so a playtest
 *  where Jonathan says "it stopped making circles" is answerable by grepping the log rather
 *  than by guessing. ⛔ The refusal is NOT a Warning — hitting a cap is a legal player action,
 *  and a warning-level line for normal play trains everyone to ignore the category.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMapMark, Log, All);

/**
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *  THE MAP-MARK STORE (MARKS batch, TASK-744)
 *  `USiegeMapMarkSubsystem` — the array, the lowest-free-number allocator, the cap.
 *  Law: `MARK-§0`..`MARK-§6`. QA gate: TASK-753. Compile + suite gate: TASK-754.
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *
 *  ⭐⭐ WHAT THIS CLASS IS FOR, IN ONE SENTENCE: it is the SHARED TRUTH about the player's
 *      numbered circles that the war map (TASK-745), the AI snapshot (TASK-746) and the help
 *      screen (TASK-751) all read — and it is the reason those three can never disagree about
 *      which circle is `circle_2`.
 *
 *  ⛔ IT READS NONE OF THEM BACK. There is no Slate in this file, no widget knowledge, no
 *  snapshot knowledge, no `UWorld` and no actor. A mark is a number, a point and a radius.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ WHY `ULocalPlayerSubsystem` AND ⛔ NOTHING ELSE (`M-2`, `MARK-§6`)
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  Marks are the PLAYER'S OWN tactical notation. A `ULocalPlayerSubsystem` is ONE PER LOCAL
 *  PLAYER and lives entirely inside that client's process ⇒ ⭐ "per-player", "zero bytes on
 *  the wire" and the whole M8 declaration are STRUCTURAL PROPERTIES OF THE BASE CLASS, ⛔ not
 *  promises an agent has to keep. `M-3` (⛔ not enemy-visible) is what keeps it that way.
 *
 *  ⚠️ THE ALTERNATIVES AND WHY EACH IS WRONG, recorded so nobody "tidies" this later:
 *    • `UGameInstanceSubsystem` — one per PROCESS, so a future second local player (splitscreen)
 *      would share one set of circles, silently violating `M-2`.
 *    • `UWorldSubsystem` — dies with the world, so the marks would vanish on any travel and the
 *      class would quietly become "clear on level load", which is ⛔ NOT what `M-4` rules.
 *    • A member on the widget — the snapshot would then have to include a Slate widget to read
 *      a place name. ⚖️ That is the coupling this whole file exists to prevent.
 *  ⇒ ⭐ The type is ASSERTED BY A TEST (`Siegebound.MapMarks.StoreIsLocalPlayerScoped`) so the
 *    day someone "simplifies" the base class, the suite goes red and names the ruling.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⛔ NO `Initialize` / `Deinitialize` OVERRIDE, AND IT IS ⛔ NOT AN OMISSION
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  There is nothing to set up: the array default-constructs empty, there is no timer, no
 *  delegate binding, no asset load and no OS handle. ⭐ THE PRACTICAL CONSEQUENCE THAT MAKES
 *  THE TESTS POSSIBLE: an instance is fully functional straight out of `NewObject`, so the
 *  headless suite drives this class with ⛔ no PIE, ⛔ no viewport and ⛔ no `GameInstance` —
 *  only a throwaway `ULocalPlayer` outer, which `UCLASS(Within = LocalPlayer)` requires.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⛔ NOT ONE `UFUNCTION` ON THIS CLASS, AND THAT IS ALSO DELIBERATE
 *  ─────────────────────────────────────────────────────────────────────────────────────────
 *  ⚠️ UHT would REJECT every one of the reflected forms: `FSiegeMapMark` is a plain struct (the
 *  PINNED shape), so it may not appear as a reflected parameter, and `GetMarks()` returns a
 *  `const TArray<...>&`, which is not a legal reflected return type either. ⇒ ⛔ A well-meaning
 *  "expose it to Blueprint" edit does not fail code review — it fails UHT, loudly, at this
 *  batch's ONE compile gate (TASK-754), exactly as `KBD-§8` records for the same trap.
 *  ✅ Every consumer of this class is C++.
 *
 *  ⛔ NEVER WRITTEN TO A SAVE GAME (`M-4`). There is no `USiegeMapMarkSaveGame`, no field in
 *  `USiegeAccountSaveGame`, and no serialisation of any kind. A mark is a note about THIS
 *  match's ground.
 *
 *  M8 DECLARATION (verbatim, `MARK-§6`): adds no replicated property, no new replicated class,
 *  no RPC, no new relevancy tier. ✅ Structural — see the `ULocalPlayerSubsystem` section above.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeMapMarkSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  THE PINNED CROSS-TASK API (TASKBOARD registry under TASK-744).
	//  ⚠️⚠️ TASK-745 AND TASK-746 COMPILE AGAINST THESE EXACT SIGNATURES. ⛔ Do not "improve"
	//       a name or a parameter here — UBT compiles the whole module, so a unilateral change
	//       breaks two files whose authors cannot see this one.
	// ═════════════════════════════════════════════════════════════════════════════════════

	/** Adds at the LOWEST FREE number. Returns false (and mutates NOTHING) at MaxMapMarks. */
	bool AddMark(const FVector2D& WorldXY, float RadiusUU, FSiegeMapMark& OutMark);

	bool RemoveMark(int32 Number);                       // ⛔ LEAVES A HOLE. ⛔ NEVER renumbers (M-1).

	bool SetMarkRadius(int32 Number, float NewRadiusUU);

	const TArray<FSiegeMapMark>& GetMarks() const;

	void ClearMarks();                                   // match reset / PlayAgain (M-4)

	/**
	 *  ⭐ ADDITIVE BEYOND THE PINNED REGISTRY, AND DECLARED AS SUCH (handoffs/TASK-744-programmer.md).
	 *  It is a convenience over `GetMarks()`, ⛔ never a second source of truth — it walks the
	 *  same array the pinned accessor returns. Nothing in TASK-745/746 is required to use it,
	 *  so its existence cannot break either of them.
	 *
	 *  ⚠️ THE RETURNED POINTER IS INTO THE LIVE ARRAY: it is invalidated by the very next
	 *  `AddMark` / `RemoveMark` / `ClearMarks`. Read it, copy what you need, and ⛔ never store
	 *  it across a frame.
	 *
	 *  @return the mark carrying `Number`, or `nullptr` when no mark does (a deleted number's
	 *          HOLE answers `nullptr`, which is precisely what makes a stale `circle_2` sitting
	 *          in the player's input box resolve to NOTHING instead of to different ground).
	 */
	const FSiegeMapMark* FindMark(int32 Number) const;

	// ═════════════════════════════════════════════════════════════════════════════════════
	//  THE TUNABLES — `EditDefaultsOnly`, each with its CONSEQUENCE written beside it
	//  (`HIGH-§1`'s law: a number whose consequence is not written next to it gets retuned by
	//   someone who does not know what they are changing).
	// ═════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  ⛔ JONATHAN'S CAP (`M-5`). NINE. ⛔ Do ⛔ NOT raise it without escalating — the number is
	 *  not a feel choice, it is the answer to a MEASURED budget, and three independent reasons
	 *  agree on it:
	 *    (i)  ⚠️ PROMPT COST — each published mark costs `, circle_N` = 10 characters of ZONE C,
	 *         and the 13-kind board has ~6 characters of headroom (`MARK-§2`, measured from
	 *         `SiegeAssistantSnapshot.h:290`, `:379-381`). ⇒ 9 marks = 90 chars, which fits the
	 *         ~121 that the `ZoneBCharReserve` lever buys (TASK-528, MEASURE-FIRST, ⛔ NOT taken
	 *         here). ⭐ THE OVERRUN IS ⛔ NOT A CRASH: the shipped elastic trimmer collapses the
	 *         roster tail into `other_kinds:`, which prints NAMES — kinds keep their EXISTENCE
	 *         and lose their COUNTS, and `BuildZoneC` logs every degradation.
	 *    (ii) a SINGLE DIGIT stays legible drawn inside a circle at map scale.
	 *    (iii) a SPOKEN order stays unambiguous — *"hold 1"*, never *"hold 11"*.
	 *  ⇒ Raising this to 10+ spends real prompt characters AND makes the drawn number
	 *    two digits AND makes the spoken order ambiguous, all at once.
	 *
	 *  ⚠️ RETUNING IT DOWN mid-match is safe but not free: existing marks above the new cap are
	 *  KEPT (deleting the player's notation behind his back would be worse), further adds simply
	 *  refuse until enough are removed. That is the only behaviour this class has for it.
	 *
	 *  📌 `EditDefaultsOnly` is `MARK-§5`'s tunables law. ⚠️ Honest note for the next reader: a
	 *  `ULocalPlayerSubsystem` has no asset to open, so in practice this is retuned in C++ —
	 *  the specifier is here because the law pins it and because it keeps the number a DATA
	 *  DECISION rather than a hardcoded literal buried in the allocator.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Map Marks")
	int32 MaxMapMarks = 9;

	/**
	 *  The smallest radius a mark may hold, in WORLD uu (1 uu = 1 cm, `HIGH-§1`) ⇒ 250 uu = 2.5 m.
	 *
	 *  CONSEQUENCE: below this a mark stops being a PLACE. It is smaller than a single summoned
	 *  unit's footprint, the drawn digit no longer fits inside its own circle at map scale
	 *  (the arena is 52,000 × 24,000 uu — `USiegeScatterConfig::ArenaHalfExtent` ships
	 *  `(26000, 12000)`, WarMapWidget.h:107 — so 250 uu is already under 1% of the map's width),
	 *  and an order to "hold" there would name ground no group could stand on.
	 *
	 *  ⛔ THIS IS THE MODEL'S SANITY FENCE, ⛔ NOT THE WHEEL'S FEEL. The wheel step and the
	 *  widget-space min/max belong to `UWarMapWidget` (TASK-745, `MARK-§4`), are separately
	 *  named, and ⛔ must not be collapsed into these two.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Map Marks")
	float MinMarkRadiusUU = 250.f;

	/**
	 *  The largest radius a mark may hold, in WORLD uu ⇒ 12,000 uu = 120 m.
	 *
	 *  CONSEQUENCE: 12,000 uu is the arena's SHORT half-extent, so a mark at this cap already
	 *  spans the battlefield from one long edge to the other. Anything larger denotes
	 *  "everywhere", and a place that means everywhere is not a place — it would make
	 *  *"move all units to hold 3"* indistinguishable from *"move all units"*, which is the
	 *  valid-shaped-wrong-command class this project keeps paying for.
	 *
	 *  ⚠️ IF THIS IS EVER SET BELOW `MinMarkRadiusUU` BY A MIS-RETUNE, THE MINIMUM WINS: the
	 *  clamp fails toward the small, visible, harmless value rather than toward a circle that
	 *  swallows the map.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Map Marks")
	float MaxMarkRadiusUU = 12000.f;

private:

	/**
	 *  ⛔⛔ THE NUMBERING LAW LIVES HERE (`M-1`), AND IT IS THE RULING MOST LIKELY TO BE
	 *      "IMPROVED" BY A LATER READER. Returns the LOWEST number in
	 *      `[FirstMarkNumber, MaxMapMarks]` that no live mark holds, or `INDEX_NONE` when every
	 *      one of them is taken.
	 *
	 *  ⛔ DELETING A MARK LEAVES A HOLE. ⛔ NOTHING IS EVER RENUMBERED.
	 *
	 *  ⚖️⭐ THE REASON IS THE AIRLOCK, ⛔ NOT ERGONOMICS — and if you only read one comment in
	 *      this file, read this one. `WR-§6` law is that the map writes a symbol into the
	 *      console input box and ⭐ THE PLAYER SENDS IT HIMSELF ⇒ there is a window of ARBITRARY
	 *      LENGTH between the moment `circle_2` is composed and the moment Enter is pressed. If
	 *      deleting mark 1 renumbered 2 → 1, the symbol already sitting unsent in the player's
	 *      box would silently denote DIFFERENT GROUND. ⚖️ That is an order the player already
	 *      gave being quietly redirected — the worst failure this whole feature could have, and
	 *      it would be invisible to him at the moment it happened.
	 *
	 *  ⚠️ LOWEST-FREE REUSE KEEPS A SMALLER VERSION OF THE SAME HAZARD (delete `circle_2`, draw
	 *  a new one, and an unsent `circle_2` now means the new ground) and it is ACCEPTED ON THE
	 *  RECORD (`M-1`) because the cap is 9 and monotonic numbering would exhaust it in a single
	 *  match. ⛔ It is not an oversight to be "fixed" by making numbers monotonic.
	 *
	 *  ⛔ THE SCAN IS OVER NUMBERS, ⛔ NEVER OVER INDICES, and it does not depend on the array's
	 *  order — so the ordering invariant below can never silently corrupt the allocator.
	 *  At most 9 × 9 comparisons; this is called once per player click.
	 */
	int32 FindLowestFreeNumber() const;

	/**
	 *  The ONE radius gate, used by BOTH `AddMark` and `SetMarkRadius` so a mark cannot be
	 *  born outside the range that a later resize would be held to.
	 *
	 *  ⛔ A NON-FINITE INPUT (NaN / ±Inf) BECOMES `MinMarkRadiusUU`, ⛔ never NaN: `FMath::Clamp`
	 *  propagates a NaN straight through (every comparison against NaN is false), and a NaN
	 *  radius stored here would travel to a Slate paint call and to a world-space resolution in
	 *  the snapshot. ⚠️ It cannot arrive from the shipped wheel path — this guard exists so that
	 *  it can never arrive from ANY path, including a future one.
	 */
	float SanitizeRadius(float InRadiusUU) const;

	/**
	 *  The live marks, ⭐ HELD SORTED ASCENDING BY `Number` — a declared invariant, ⛔ not an
	 *  accident of insertion order. Two consumers depend on it: the painter draws in a stable,
	 *  predictable order rather than in "whenever you happened to make it" order, and a reader
	 *  scanning `GetMarks()` sees the holes exactly where the player left them (1, 3, 4 reads
	 *  as "2 was deleted").
	 *
	 *  ⛔ THE INDEX IS ⛔ NEVER THE IDENTITY. `Marks[1]` is not `circle_2`, and after any delete
	 *  it usually is not. Every public function on this class takes a NUMBER.
	 *
	 *  ⛔ NOT A `UPROPERTY`, and it cannot be one: `FSiegeMapMark` is a plain struct by the
	 *  pinned design. ✅ Safe, because it holds no `UObject` reference of any kind — there is
	 *  nothing here for the GC to keep alive (see `SiegeMapMark.h`'s note about the day someone
	 *  adds a `UObject*` field to that struct).
	 */
	TArray<FSiegeMapMark> Marks;
};
