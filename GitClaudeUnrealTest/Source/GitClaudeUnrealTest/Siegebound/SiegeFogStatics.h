// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// FMath (Math/UnrealMathUtility.h) arrives complete through CoreMinimal and it is the ONLY
// engine type this pair names. That is deliberate and it is the whole point of the file:
// there is no UWorld, no AActor, no ITeamAgent, no AController and no clock read anywhere in
// SiegeFogStatics.{h,cpp}, which is what lets the entire fog rule be exercised headlessly in
// Tests/SiegeFogTest.cpp with no PIE session (complete-type include law, TASK-110; the
// FSiegeStuckStatics / FSiegeCombatStatics precedent).
#include "CoreMinimal.h"

#include "SiegeFogStatics.generated.h"

/**
 *  ═══ Siegebound FOG statics — the pure visibility rule (TASK-837, FOG-§1 / FOG-§6) ═══
 *
 *  Jonathan, verbatim (2026-09-03): "The fog makes it to where players can really only see up
 *  until about 20 feet in front of you. And that includes AI and all units. Meaning that ranged
 *  units will not be able to fire beyond this range and players will not be able to see anything
 *  beyond this range. … I want the player to begin to see a light amount of fog starting at
 *  about 10 feet away and it gets thicker and thicker until they really cannot see anything
 *  beyond 20 feet away."
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE TRAP THIS WHOLE FILE IS POINTED AT: **"20 FEET" IS NOT `20` UNREAL UNITS**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Unreal is CENTIMETRES (1 uu = 1 cm) and 1 ft = 30.48 cm exactly, so his ceiling is
 *  20 × 30.48 = 609.6 uu. A `20` there would be wrong by 30× — it would put the fog ceiling
 *  INSIDE the unit's own capsule (the hero capsule is 192 uu tall, FOG-§2) — and it would look
 *  entirely plausible in review. ⛔ This is the SECOND time he has specified a distance in feet
 *  (HIGH-§1's 152.4 was the first), so feet-in-prose is now a recognised project idiom:
 *  converted at 30.48, pinned once, with the arithmetic written beside it.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔⭐⭐ AMENDED 2026-09-04 (TASK-981, law FOG-§9.2) — THE FALLOFF IS **BEER-LAMBERT** NOW,
 *  AND THE QUADRATIC EASE-IN THAT SHIPPED HERE IS **SUPERSEDED, NOT BUGGED**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⚠️ READ THIS BEFORE JUDGING THE OLD CURVE. The `t^FogDensityExponent` ramp that used to live
 *  here was CORRECT under the reading of his sentence available at the time ("a LIGHT amount of
 *  fog STARTING at about 10 feet" ⇒ an ease-in leaving the onset with zero slope). ⛔ It is not
 *  being repaired. It is being REPLACED, because Jonathan then ruled the opposite outcome
 *  explicitly, after being shown the arithmetic:
 *
 *      "As far as the 'transmittance at half the distance is locked to the square root of the
 *       far value. Tuned to 98% obscured at 20 feet, it forces 86% at 10'. I am fine with tuning
 *       it to that, in fact, that is probably best because that probably looks the best, so lets
 *       do that."                                              — Jonathan, 2026-09-04
 *
 *  ⛔⛔ THE CONFLICT, STATED SO NOBODY TRIES TO RECONCILE IT: the old curve returned EXACTLY 0
 *  at `FogVisionOnsetUU`. He has ruled ~86% obscured there. That is a direct contradiction, not
 *  a nuance, and HIS WORD WINS. ⇒ `FogDensityAt` was REWRITTEN, not tuned, and
 *  `FogDensityExponent` was RETIRED (FOG-§9.2; SC-§40 cl. 2 — dead surface is not an option).
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⛔ WHAT THIS FILE IS, AND — MORE IMPORTANTLY — WHAT IT IS NOT
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  It is the PURE RULES ONLY: the extinction curve the VISUAL is drawn from, and the visibility
 *  predicate the acquisition range clamp consumes. It is floats in, floats out.
 *
 *  ⛔⛔⭐ AND THOSE TWO ARE NOW DELIBERATELY DIFFERENT SHAPES — **THE MECHANIC IS NOT THE LOOK.**
 *  `FogDensityAt` is ASYMPTOTIC (Beer-Lambert never quite reaches 1). `EffectiveVisionRadius` is
 *  a HARD CUT at the ceiling. ⛔ Nobody "harmonises" them: an asymptotic MECHANIC would leave 2%
 *  visibility forever, and at Longbowman range 2% is a LETHAL SHOT (FOG-§9.2, and it is
 *  FOG-§7a's own argument, still correct). ⭐ He chose the look; the hard cut is what keeps the
 *  card assertable and non-lethal.
 *
 *    ⛔ TASK-838 applies the ceiling inside `FSiegeCombatStatics::GatherHostileAgents`.
 *    ⛔ TASK-839 owns `AFogVolume`, the duration, the refresh and the spell.
 *    ⛔ TASK-841 owns the visual (`/Game/Blueprints/BP_SiegeFog`).
 *
 *  ⛔⛔ ZERO BEHAVIOUR CHANGE. NOTHING IN THE SHIPPED GAME CONSULTS THIS FILE YET. A landed
 *  file is not a landed feature — until TASK-838 routes the clamp through the funnel, fog does
 *  not exist at runtime and no unit's range changes by one unit.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  ⭐ M8 DECLARATION (FOG-§6's clause, restated at the seam that has to honour it)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Fog is SYMMETRIC and WORLD-GLOBAL, so unlike WITCH-§ it leaks nothing: the ceiling derives
 *  from ONE replicated "fog is active" scalar (TASK-839's), ⛔ never from per-actor visibility.
 *  ⭐ AND THAT IS ENFORCED STRUCTURALLY RATHER THAN BY DISCIPLINE: not one function below takes
 *  a team, a viewer, a controller or an actor. An asymmetric fog is UNREPRESENTABLE in this
 *  API — you cannot write the favouritism, because there is no parameter to branch on.
 *  ⚠️ Contrast TASK-827's invisibility predicate, which DOES take a viewer team and a target
 *  team precisely because that feature IS asymmetric. The two signatures disagree on purpose.
 *
 *  QA gate: the TASK-837 report. Compile: deferred (QUIET-MODULE — three compiles queued).
 */

/**
 *  The fog's tunable numbers, together, as a reflected struct.
 *
 *  ⚠️ WHY A USTRUCT AND NOT BARE `constexpr`s, SINCE FOG-§6 ALSO SAYS "PLAIN C++ STATICS":
 *  FOG-§1 requires these to be `EditDefaultsOnly` ("his next sentence retunes them with no code
 *  change" — J-F2's whole reasoning), and `EditDefaultsOnly` requires reflection, which a plain
 *  `class FSiegeFogStatics` cannot carry. ⭐ The project has already solved exactly this tension
 *  once: `FSiegeStuckTuning` (SiegeStuckStatics.h:135) is a reflected tuning struct sharing a
 *  header with a plain static library, and NAV-§7 explicitly instructs QA not to flag it as a
 *  one-class-per-header violation. This is that precedent, reused, not a new pattern.
 *  ⛔ The struct is pure data: it has no world, so the headless-testability half of FOG-§6 is
 *  untouched.
 */
USTRUCT(BlueprintType)
struct FSiegeFogTuning
{
	GENERATED_BODY()

	/**
	 *  ⭐⛔⛔ THE ONSET, IN UNREAL UNITS, AND THE ARITHMETIC IS WRITTEN OUT HERE SO ⛔ NOBODY
	 *  EVER RE-DERIVES IT:
	 *
	 *      Jonathan said "about 10 FEET". "10 feet" is ⛔ NOT an engine unit.
	 *      Unreal is CENTIMETRES, and 1 uu = 1 cm.
	 *      1 ft = 30.48 cm  (exact, by international definition since 1959)
	 *      10 ft = 10 × 30.48 = 304.8 cm  ⇒  ⭐ 304.8 uu
	 *
	 *  ⛔⛔ 304.8 IS THE ONLY NUMBER. ⛔ NOT 300 ("close enough" — a designer's round number),
	 *  ⛔ NOT 305, and ⛔⛔ ABOVE ALL NOT 10 — feet-as-units is wrong by 30× and it would put the
	 *  onset 10 cm from the camera, i.e. the whole world permanently fogged. ⛔ There is exactly
	 *  ONE `304.8` in the codebase and it is this line.
	 *
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *  ⛔⛔⭐ CORRECTED 2026-09-04 (TASK-981, FOG-§9.2, SC-§53 cl. 3). THE PARAGRAPH THAT USED TO
	 *  STAND HERE IS NOW HALF-FALSE, SO IT IS **REWRITTEN — THE CONSTANT IS NOT DELETED.**
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *
	 *  ⛔ IT USED TO SAY: *"this is where the fog BECOMES VISIBLE, and everything closer than it
	 *  is perfectly clear"*, and that values >= the ceiling *"degenerate the ramp to a hard step"*.
	 *  ⛔⛔ BOTH SENTENCES DIED WITH THE `t²` CURVE. Under Beer-Lambert there is **NO CLEAR BUBBLE
	 *  AND NO KNEE**: the fog is already ~85.9% obscured AT this distance, ~62.4% at half of it,
	 *  and ~53.7% out at the 120-uu melee band. ⭐ That is HIS RULING, arrived at deliberately —
	 *  see the file header. `FogDensityAt` does not read this member at all any more.
	 *
	 *  ⭐⭐ SO WHAT IS IT STILL FOR? **TWO LIVE ROLES, AND THE FIRST ONE IS LOAD-BEARING:**
	 *    1. ⛔⛔ It is `FogVisionCeilingUU`'s **`ClampMin` FLOOR** (`FOG-§7b` half (a)) — the
	 *       tightest ceiling that is still a fog. ⛔ Deleting this constant would leave that meta
	 *       string with nothing to track and would re-open the slider-stop trap that blinded the
	 *       entire army. ⛔ `FOG-§7b` IS UNAFFECTED BY THE CURVE CHANGE AND STILL BINDS.
	 *    2. It is the **REPORTING POINT** — the distance FOG-§9.2's "86% at 10 feet" is quoted at,
	 *       and the distance the suite anchors its middle assertion on.
	 *
	 *  Measured against the shipped hero capsule (`InitCapsuleSize(42, 96)` ⇒ 192 uu tall,
	 *  FOG-§2): 304.8 / 192 = 1.59 hero body-lengths. ⚠️ Note that this — NOT the ceiling — is the
	 *  "roughly one-and-a-half body lengths" figure; the brief attached it to the wrong end of his
	 *  own sentence and FOG-§2 corrects it.
	 *
	 *  ⚠️ RETUNING IT NO LONGER CHANGES THE PICTURE AT ALL — it only moves the ceiling's slider
	 *  floor. ⛔ Anyone who edits this expecting the fog to look different is editing the wrong
	 *  number; the shape knob is `FogTransmittanceAtCeiling` below.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "0"))
	float FogVisionOnsetUU = 304.8f; // FOG-§1: 10 ft × 30.48 cm/ft

	/**
	 *  ⭐⛔⛔ THE CEILING, IN UNREAL UNITS. SAME ARITHMETIC, SAME LAW, SAME TRAP:
	 *
	 *      Jonathan said "about 20 FEET".
	 *      1 ft = 30.48 cm  (exact, by international definition since 1959)
	 *      20 ft = 20 × 30.48 = 609.6 cm  ⇒  ⭐ 609.6 uu
	 *
	 *  ⛔⛔ 609.6 IS THE ONLY NUMBER. ⛔ NOT 600, ⛔ NOT 610, and ⛔⛔ ABOVE ALL NOT 20 — at
	 *  feet-as-units the ceiling would sit INSIDE the unit's own 192-uu capsule and every ranged
	 *  unit in the game would be unable to fire at all. ⛔ There is exactly ONE `609.6` in the
	 *  codebase and it is this line.
	 *
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *  ⭐⭐⭐ READ THIS BEFORE YOU RETUNE IT. THIS IS THE LARGEST COMBAT MODIFIER IN THE GAME.
	 *  FOG-§2's table, measured from Docs/Data/cards.csv — ⛔ nothing here is estimated:
	 *
	 *  ⛔⛔⭐⭐ …AND *WHICH QUANTITY* IT MEASURES, SAID OUT LOUD — AMENDED 2026-09-04 (TASK-997,
	 *  law ⭐⭐⭐ FOG-§9.11). ⛔ EVERY PERCENTAGE BELOW IS A **FIRING-RANGE** CUT. ⛔ NOT A NOTICE
	 *  CUT, ⛔ NOT A CHASE CUT — and the rows reading ✅ UNAFFECTED are unaffected ⛔ IN FIRING
	 *  RANGE ONLY. ⛔ Nothing in this table was corrected; ⛔ it was AMBIGUOUS, not wrong.
	 *
	 *  ⛔ The distinction did not exist when the table was written: notice == firing for a ranged
	 *  unit, so ONE number served BOTH gates and this block was unambiguous ⛔ by accident.
	 *  ⭐⭐⭐ FOG-§9.11 RETIRED that identity. NOTICE is now its own gate — ASummonedUnit::
	 *  UnitEngagementRadiusUU, 5000 uu, the SAME value for every shipped card (a DEFAULT, ⛔ never
	 *  a cap) — while the `shipped Range` column below stays ⛔ PER-CARD.
	 *  ⇒ ⛔⛔ THE NOTICE CUT IS THEREFORE A SINGLE FIGURE, ⛔ NOT A TABLE, AND IT IS ⛔ NONE OF THE
	 *  NUMBERS BELOW: fog costs EVERY unit ⛔ 87.8% of its notice radius (`1 − 609.6/5000`,
	 *  FOG-§9.11) — ⛔ steeper than every firing cut in this table, the Longbowman's included, and
	 *  it bites the ⛔ MELEE rows too, whose FIRING range is genuinely untouched.
	 *
	 *  ⇒ ⚖️ ⛔ CITE A ROW BELOW ⛔ ONLY FOR FIRING RANGE. For acquisition, chase or leash, cite
	 *  ⭐⭐⭐ FOG-§9.11 and ⛔ NEVER this block. ⚖️ TWO TRUE NUMBERS SITTING ADJACENT IN ONE LANE IS
	 *  EXACTLY THE CONDITION UNDER WHICH A MISREAD LOOKS ⛔ VERIFIED (J-F31, ⛔ third surface in
	 *  this lane in one day) — which is why this is stated ⛔ HERE, at the point of reading, and
	 *  ⛔ not left to the reader to notice that a table has quietly changed subject.
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *
	 *      card            shipped Range   under a 609.6 ceiling      cut
	 *      Longbowman           3600  uu   ->  609.6              ⛔⛔ -83.1%
	 *      Archer               2100  uu   ->  609.6              ⛔  -71.0%
	 *      Wizard               2100  uu   ->  609.6              ⛔  -71.0%
	 *      BallistaTower        1400  uu   ->  609.6              ⛔  -56.5%   (MinRange 300 still
	 *                                                              applies ⇒ its usable band
	 *                                                              collapses to a 300-609.6 ANNULUS)
	 *      ArrowTower            900  uu   ->  609.6                  -32.3%
	 *      BombTower / Crystal   800  uu   ->  609.6                  -23.8%
	 *      Cleric (heal)         400  uu   ->  400                ✅ UNAFFECTED — already inside
	 *      every melee unit      120  uu   ->  120                ✅ UNAFFECTED — already inside
	 *
	 *  ⭐ AND THE SCALE, so the table is not read in a vacuum: the arena is 50,000 uu castle to
	 *  castle (ArenaHalfExtent, castles at +/-25,000) = 1,640 ft. 609.6 / 50,000 = ⛔ 1.22%.
	 *  ⇒ under fog a player sees ONE-EIGHTIETH of the way to the enemy castle.
	 *  ⇒ 609.6 / 192 = 3.18 hero body-lengths.
	 *
	 *  ⚖️ THIS IS DISCLOSURE, ⛔ NOT A COUNTER-PROPOSAL. Jonathan has been shown this table and
	 *  his number STANDS (J-F2). It converts a ranged siege into a melee brawl in one 50-gold
	 *  play, which may be exactly what he wants from the card. ⛔ It is not adjusted here, ⛔ and
	 *  it is not softened with an invented floor. `EditDefaultsOnly` is the retune path: his next
	 *  sentence changes this value with no code change and no recompile of anything but defaults.
	 *
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *  ⚖️⛔⛔ THE SLIDER STOP IS NOT `0` ANY MORE — `FOG-§7b`, RULED 2026-09-03, SHIPPED BY
	 *  TASK-838 (the task that made this value able to fire). HALF (a) OF TWO.
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *
	 *  `ClampMin` was `"0"`, which put `min(Range, 0) == 0` on the editor's own slider stop:
	 *  every acquisition in the game would return NOTHING while fog was up, i.e. ALL COMBAT
	 *  STOPS — the exact outcome EffectiveVisionRadius' guard comment says must never happen,
	 *  reached through the one input adjacent to the guarded one.
	 *
	 *  ⭐ The new floor is `304.8` — the ONSET's OWN pinned value (FOG-§1, 10 ft), ⛔ NOT a new
	 *  invented number. Below the onset the model has no band at all (Ceiling <= Onset already
	 *  degenerates to a hard step), so the tightest ceiling that is still a FOG is the onset
	 *  itself. ⛔ Documenting `0` as a deliberate "blind" capability was REFUSED on the record:
	 *  there is no design ask for a blindness mode, and a value that stops the game is not made
	 *  safe by a comment.
	 *
	 *  ⛔⛔ THIS HALF PROTECTS THE DESIGNER AND ⛔ NOTHING ELSE. `ClampMin` constrains the editor
	 *  SPINNER; an `.ini`, a Blueprint default or a line of C++ can still write `0`. ⇒ half (b) —
	 *  the `Ceiling <= 0.f` guard in EffectiveVisionRadius — is the one that protects the GAME,
	 *  and shipping (a) without (b) is exactly the class of fix that looks complete and holds
	 *  nothing.
	 *
	 *  📌 DECLARED, so a future FOG-§1 "no second literal" grep does not read it as a violation:
	 *  the `"304.8"` below is the SECOND textual occurrence of that number in Source/. It is a
	 *  META STRING, not a float literal, and UHT meta values cannot reference a C++ constant, so
	 *  it is unavoidable. It must track FogVisionOnsetUU above; `SiegeFogClampTest.cpp` asserts
	 *  the two agree, so an onset retune that forgets this line goes RED.
	 *
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *  ⭐⭐ NEW ROLE, ADDED 2026-09-04 (TASK-981, FOG-§9.2): THIS IS ALSO **σ's ANCHOR**, WHICH
	 *  MAKES IT THE ONE NUMBER THAT SETS BOTH THE MECHANIC AND THE LOOK
	 *  ───────────────────────────────────────────────────────────────────────────────────────
	 *
	 *  The extinction coefficient is DERIVED, never typed:
	 *
	 *      σ = −ln(FogTransmittanceAtCeiling) / FogVisionCeilingUU
	 *        = 3.9120230 / 609.6 = 0.0064174 per uu
	 *
	 *  ⇒ this value is what "98% obscured" is 98% obscured AT. ⭐ Retuning it moves the whole
	 *  curve with it (the suite asserts exactly that, so a hardcoded σ goes RED).
	 *
	 *  ⚠️⚠️ AND IT IS NOW A **DENOMINATOR**, WHICH IT WAS NOT UNDER THE OLD CURVE. The `t²` ramp
	 *  divided by `(ceiling − onset)` and guarded that; Beer-Lambert divides by THIS. ⇒ a zero or
	 *  negative ceiling is a divide-by-zero in `FogDensityAt` and is guarded there explicitly.
	 *  ⭐ `FOG-§7b`'s `ClampMin = "304.8"` below already keeps the designer away from it; the
	 *  guard in the .cpp is what protects the GAME from an `.ini` or a line of C++.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "304.8"))
	float FogVisionCeilingUU = 609.6f; // FOG-§1: 20 ft × 30.48 cm/ft ⛔ ClampMin = the onset (FOG-§7b(a))

	/**
	 *  ⭐⭐ THE SHAPE KNOB, AND IT IS **HIS OWN NUMBER**: *"98% obscured at 20 feet"*.
	 *  (NEW 2026-09-04, TASK-981, FOG-§9.2 / FOG-§9.4. ⛔ It REPLACES the retired
	 *  `FogDensityExponent`, which is gone rather than dormant — SC-§40 cl. 2.)
	 *
	 *  This is the fraction of light still arriving from a target sitting exactly at
	 *  `FogVisionCeilingUU` — the TRANSMITTANCE. `0.02` is his 98% obscuration, written the way
	 *  the physics is written.
	 *
	 *  ⭐⛔ IT IS A **RATIO**, NOT A DISTANCE, AND THAT IS WHY IT IS SAFE. `FOG-§1`'s 30× trap
	 *  ("20 feet" is not `20` uu) can only bite a number carrying a UNIT. This one is
	 *  dimensionless — it cannot be wrong by 30×, and it does not touch FOG-§1's "exactly one
	 *  `304.8` and one `609.6` in the codebase" law.
	 *
	 *  ── THE WHOLE MODEL FALLS OUT OF THIS ONE NUMBER ────────────────────────────────────
	 *
	 *      σ = −ln(FogTransmittanceAtCeiling) / FogVisionCeilingUU     ⛔ DERIVED, never typed
	 *        = −ln(0.02) / 609.6  =  3.9120230 / 609.6  =  0.0064174 per uu
	 *
	 *      Transmittance(d) = exp(−σ·d)          Obscuration(d) = 1 − exp(−σ·d)
	 *
	 *      d =   0.0 uu  ⇒  0.0000  (exactly clear at the camera)
	 *      d = 152.4 uu  ⇒  0.6239  ( 5 ft — 62.4% obscured)
	 *      d = 304.8 uu  ⇒  0.8586  (10 ft — ⭐ HIS "86% at 10 feet")
	 *      d = 609.6 uu  ⇒  0.9800  (20 ft — ⭐ HIS "98% obscured at 20 feet")
	 *
	 *  ⭐⭐ HE DERIVED THE 86% HIMSELF AND ACCEPTED IT. It is not a side effect we are living
	 *  with: transmittance at half a distance is locked to the SQUARE ROOT of the far value for
	 *  EVERY σ, so 98% at 20 ft FORCES ~86% at 10 ft. He was shown that arithmetic and answered
	 *  *"that is probably best because that probably looks the best, so lets do that."*
	 *
	 *  ⛔⛔ DO **NOT** WRITE A TEST FOR THAT √ RELATION. `exp(−σd/2) = √(exp(−σd))` is a THEOREM
	 *  of the model — true for every σ and every d — so there is nothing about it that could ever
	 *  fail and such a test would report SAFE forever. (Same trap FOG-§7a already names for the
	 *  "both teams get the same answer" symmetry test.) ⭐ The suite asserts the four ABSOLUTE
	 *  numbers above instead, and those CAN fail.
	 *
	 *  ⚠️⚠️ THE DECLARED CONSEQUENCE, SAID PLAINLY BECAUSE IT IS A REAL CHANGE TO THE PICTURE:
	 *  the old curve rendered the entire melee band (120 uu) PERFECTLY CLEAR. This one renders it
	 *  **~53.7% obscured**. ⛔ The MECHANIC is untouched — `EffectiveVisionRadius` is a `min`, and
	 *  120 < 609.6, so every melee unit still fights at exactly the range it always did. Only the
	 *  LOOK changed, and it changed because he asked for it to.
	 *
	 *  ── THE SLIDER STOPS, AND WHY THEY ARE NOT `0` AND `1` ──────────────────────────────
	 *
	 *  ⛔ `0` would make σ INFINITE — an opaque white screen at every distance including the
	 *  camera. ⛔ `1` makes σ ZERO — no fog at all, i.e. a 50-gold card that does nothing. Both
	 *  are the FOG-§7b failure shape (a game-breaking value sitting on the tunable's own slider
	 *  stop), so the same ruling is applied to the same class of problem: the stops are moved to
	 *  the tightest and thinnest values that are still a FOG. ⭐ AND — exactly as FOG-§7b insists
	 *  — `ClampMin`/`ClampMax` constrain the editor SPINNER AND NOTHING ELSE. An `.ini`, a
	 *  Blueprint default or a line of C++ can still write `0`; the guard in `FogDensityAt` is the
	 *  half that protects the GAME, and it degrades to CLEAR, never to a whiteout.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "0.001", ClampMax = "0.99"))
	float FogTransmittanceAtCeiling = 0.02f; // FOG-§9.4: his "98% obscured at 20 feet" ⇒ 2% gets through
};

/**
 *  The pure fog rules. ⛔ EVERYTHING HERE IS PURE: every input is a parameter, every output is a
 *  return value. No UWorld, no AActor, no UObject, no allocation, no logging, no RNG, no clock
 *  read, no state. Purity is not style here — it is what lets Tests/SiegeFogTest.cpp assert the
 *  whole rule headlessly, and it is what keeps the M8 declaration above checkable.
 *
 *  Not a UObject / not reflected: a plain static library, so there is no BeginPlay, no GC
 *  surface, and NO Build.cs change (same module, Core only). Precedents: FSiegeCombatStatics
 *  (SiegeCombatStatics.h:23), FSiegeStuckStatics (SiegeStuckStatics.h:165).
 *
 *  ⛔ EVERY FUNCTION BELOW IS TOTAL. There is no input — not NaN, not infinity, not a negative
 *  distance, not an inverted tuning band, not a zero-width band — that divides by zero, returns
 *  NaN, or asserts. TASK-838 will call these from inside the acquisition funnel that every
 *  attack in the game routes through; a NaN escaping into that path would take out combat
 *  globally. ⭐ The degenerate cases all fail toward NO FOG (see each function), because fog is
 *  the NEW mechanic and a broken fog must never blind the army.
 */
class GITCLAUDEUNREALTEST_API FSiegeFogStatics
{
public:

	/**
	 *  ╔═══════════════════════════════════════════════════════════════════════════════════╗
	 *  ║ ⛔⛔⛔ THIS IS **THE VISUAL'S CURVE ONLY**. ⛔ NOTHING MECHANICAL MAY CONSULT IT,   ║
	 *  ║ ⛔ EVER. A GAMEPLAY SITE THAT CALLS `FogDensityAt` IS AN **AUTOMATIC QA FAIL**.    ║
	 *  ╚═══════════════════════════════════════════════════════════════════════════════════╝
	 *
	 *  (Demoted 2026-09-04 by TASK-981; law FOG-§9.2. ⛔ Enforced, not merely written: a
	 *  source-text census in `SiegeFogClampTest.cpp` pins the tree-wide call count and goes RED
	 *  the moment a shipping file outside this pair names it.)
	 *
	 *  ⭐ WHY THE BAN EXISTS AND IS NOT BUREAUCRACY: this curve is ASYMPTOTIC. It never reaches
	 *  1, so "is it opaque here?" has no true answer and a mechanic built on it would leave a
	 *  permanent sliver of visibility — 2% at the ceiling, which at Longbowman range is a LETHAL
	 *  SHOT. ⛔ The mechanical question is answered by `EffectiveVisionRadius` /
	 *  `IsVisibleThroughFog`, which are a HARD CUT, and by nothing else.
	 *
	 *  PURE. The fog's VISUAL obscuration at DistanceUU from the viewer, on [0, 1]:
	 *  0 = perfectly clear, 1 = fully obscured.
	 *
	 *  ── THE SHAPE: BEER-LAMBERT EXTINCTION (FOG-§9.2 — ⛔ REPLACES the retired `t²` ramp) ──
	 *
	 *      σ                = −ln(FogTransmittanceAtCeiling) / FogVisionCeilingUU
	 *      Transmittance(d) = exp(−σ·d)
	 *      density(d)       = 1 − exp(−σ·d)
	 *
	 *      d = 0        ->  EXACTLY 0     (clear at the camera — the one exact value left)
	 *      d = 152.4    ->  0.6239
	 *      d = 304.8    ->  0.8586        ⭐ his "86% at 10 feet"
	 *      d = 609.6    ->  0.9800        ⭐ his "98% obscured at 20 feet"
	 *      d -> ∞       ->  approaches 1, ⛔ never a hard cut
	 *
	 *  ⛔⛔ THERE IS NO CLEAR BUBBLE AND NO KNEE AT THE ONSET ANY MORE. `FogVisionOnsetUU` is
	 *  NOT READ BY THIS FUNCTION. The old curve returned exactly 0 there; he ruled 86%. See the
	 *  file header for the supersession — ⛔ the old shape was superseded, not bugged.
	 *
	 *  ⚠️ THIS IS THE DESIGN CURVE, ⛔ NOT A MATERIAL PARAMETER. TASK-841 owns the visual and
	 *  will map this onto the vendor pack's own noise/wind/shaft features; "as realistic as
	 *  possible" is achieved there, by instrument.
	 *
	 *  ⛔ TOTALITY — every degenerate input fails toward **CLEAR**, never toward a whiteout, and
	 *  every branch is reachable and tested:
	 *    - a non-finite DistanceUU, ceiling or transmittance returns 0 (clear);
	 *    - ⛔⛔ a ZERO or NEGATIVE ceiling returns 0 — and this one is NEW: the ceiling is σ's
	 *      DENOMINATOR under Beer-Lambert, which it was not under the `t²` ramp (that divided by
	 *      `ceiling − onset`). ⛔ Without this branch a `0` ceiling is a divide by zero;
	 *    - a transmittance outside the open interval (0, 1) returns 0 — `<= 0` would make σ
	 *      infinite and paint an opaque screen from a single bad float, and `>= 1` is "no fog";
	 *    - d <= 0 returns EXACTLY 0, so the camera is bit-identically clear and a negative
	 *      distance cannot produce the negative density that `1 − exp(+x)` would give;
	 *    - the result is clamped to [0, 1] belt-and-braces.
	 *
	 *  @param DistanceUU distance from the viewer in Unreal units (centimetres)
	 *  @param Tuning     the ceiling (σ's anchor) and the transmittance at it
	 *  @return           obscuration on [0, 1]; 0 clear, 1 fully obscured
	 */
	static float FogDensityAt(float DistanceUU, const FSiegeFogTuning& Tuning);

	/**
	 *  PURE. ⭐⭐ THE SEAM TASK-838 CONSUMES, and the only one it needs.
	 *
	 *  Returns the radius an acquisition may actually SEE to: RequestedRadiusUU when fog is
	 *  inactive, and min(RequestedRadiusUU, FogVisionCeilingUU) when it is active.
	 *
	 *  ⛔⛔ WHEN bFogActive IS FALSE THIS RETURNS RequestedRadiusUU BIT-IDENTICALLY. No clamp,
	 *  no sanitising, no rounding, not one ulp of drift. ⭐ That is what makes it safe for
	 *  TASK-838 to call UNCONDITIONALLY at the funnel: with fog off the game is byte-for-byte
	 *  the game that shipped, which is the property that keeps a fog regression attributable to
	 *  fog.
	 *
	 *  ⭐⛔ THE NAME IS LOAD-BEARING: ***VISION*** RADIUS. ⛔ A BLAST RADIUS IS NOT A VISION
	 *  RADIUS AND MUST NEVER BE PASSED HERE. Two measured findings for TASK-838, because
	 *  `GatherHostileAgents` is about to become the chokepoint for consumers that are NOT acts
	 *  of seeing:
	 *    • ⚠️ ALMOST every AoE radius in the shipped game is smaller than the ceiling — Sapper 250,
	 *      BombTower 250, Wizard 250, Fireball 300, FrostNova 350, BattleCry 400 (measured from
	 *      Docs/Data/cards.csv). So a blind clamp inside the funnel is numerically inert on them
	 *      ⛔ BY COINCIDENCE OF TODAY'S DATA, not by design.
	 *    • ⛔⛔ CORRECTED BY TASK-838, AND THE WORD "ALMOST" IS THE CORRECTION: this bullet
	 *      originally read "EVERY", and that is ⛔ FALSE. ⛔ **`Lightning` ships `AoERadius = 700`**
	 *      — 90.4 uu ABOVE the ceiling, in shipped data, TODAY. The six-card enumeration above
	 *      missed it. ⚠️ It was ALSO the number every downstream spec proposed as its "synthetic"
	 *      over-the-ceiling value, so a test written to that letter would have been drawn from live
	 *      data. ⭐ Nothing is broken by it: `Lightning` is `ResolveTopTargetsDamage`, a FOG-§7
	 *      ROW 3 directed-spell reticle that hands the funnel no vision query, so it is exempt
	 *      STRUCTURALLY rather than numerically — which is the strongest possible vindication of
	 *      making the exemption structural in the first place.
	 *    • ⛔⛔ AND ONE CONSUMER ALREADY EXCEEDS IT TODAY: `ASpellLineSweep::LineRange = 900.f`
	 *      (SpellLineSweep.h:103). A blind clamp inside the funnel WOULD cut the hero-line
	 *      spell by 32.3% under fog — a live behaviour change on a surface Jonathan's sentence
	 *      never mentions. ⇒ ⛔ TASK-838 must apply this to ACQUISITION/VISION radii only and
	 *      say so at the funnel; it is not a hypothetical.
	 *  ⚖️ WITCH-§2 already ruled the same distinction for the other card in this batch — "a
	 *  blast is not an act of seeing". Fog inherits that reasoning rather than re-litigating it.
	 *
	 *  ⛔ THERE IS NO FLOOR AND ⛔ NONE MAY BE INVENTED. A Cleric's 400 stays 400 and a melee
	 *  unit's 120 stays 120 — short ranges are passed through untouched, ⛔ never RAISED to the
	 *  ceiling. `min`, not `clamp`. (FOG-§2 is disclosure; softening the number with a hidden
	 *  floor would be adjusting his ruling while appearing to honour it.)
	 *
	 *  ⛔ TOTALITY: a non-finite ceiling, a ZERO OR negative ceiling, or a non-finite
	 *  RequestedRadiusUU returns RequestedRadiusUU unchanged — a broken tuning must never blind
	 *  the whole army.
	 *  ⚖️⛔⛔ `<= 0.f`, ⛔ NOT `< 0.f` — `FOG-§7b` half (b), ruled 2026-09-03 and shipped by
	 *  TASK-838. Zero was the ONE member of the degenerate class that fell through a strict
	 *  comparison, and it is the member the editor could reach: `min(Range, 0) == 0` for every
	 *  acquisition ⇒ ALL COMBAT STOPS WHILE FOG IS UP. ⭐ This is not a new rule — it is this
	 *  module's OWN totality law (degenerate ⇒ NO FOG, ⛔ never no vision) applied to the input
	 *  that was missing from it. ⛔ Half (a) raised the ceiling's `ClampMin` to the onset's own
	 *  `304.8`; that constrains the editor spinner and NOTHING else, so THIS half is the one
	 *  that protects the game from an `.ini`, a Blueprint default or a line of C++.
	 *
	 *  @param RequestedRadiusUU the site's own acquisition range (AttackRange, tower Range, ...)
	 *  @param bFogActive        is fog active right now (TASK-839's ONE replicated scalar)
	 *  @param Tuning            the ceiling (the only member this path reads), the onset and the
	 *                           transmittance
	 *  @return                  the radius to compare against; == RequestedRadiusUU when clear
	 */
	static float EffectiveVisionRadius(float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning);

	/**
	 *  PURE. The visibility predicate: can a viewer whose own acquisition range is
	 *  RequestedRadiusUU see/engage a target at DistanceUU, given the fog state.
	 *
	 *  Defined as, and ONLY as:  DistanceUU <= EffectiveVisionRadius(...)
	 *
	 *  ⭐⛔ THE COMPARISON IS INCLUSIVE (`<=`) ON PURPOSE, AND IT IS MEASURED, NOT PREFERRED:
	 *  the shipped acquisition idiom is inclusive at every site — SummonedUnit.cpp:1644, :1785,
	 *  :1851, :1956, :2409 all read `GetDistanceToTarget(...) <= AttackRange`. ⇒ the fog clamp
	 *  replaces the OPERAND of that comparison and ⛔ never its shape. An exclusive fog predicate
	 *  would make the fog boundary the only exclusive range boundary in the game, for no reason
	 *  a player could ever perceive.
	 *
	 *  ⚠️⚠️ SO THE PICTURE AND THE MECHANIC DISAGREE AT THE CEILING, AND THE DISAGREEMENT GOT
	 *  BIGGER ON 2026-09-04 — ⛔ IT IS **HIS RULING**, NOT AN OVERSIGHT (TASK-981, FOG-§9.2):
	 *
	 *      at d == 609.6 exactly,   FogDensityAt  returns 0.98  — you can JUST BARELY make
	 *                                                              something out
	 *                               this predicate returns TRUE  — still in range, by one float
	 *      one uu past the ceiling, FogDensityAt  returns ~0.98  — visually indistinguishable
	 *                               this predicate returns FALSE — ⛔ ZERO acquisition
	 *
	 *  ⛔⛔ NOBODY "HARMONISES" THESE. He chose the look (a soft, physical extinction that never
	 *  quite closes); the HARD CUT is what makes the card assertable and non-lethal, because an
	 *  asymptotic mechanic leaves 2% visibility forever and 2% at Longbowman range is a LETHAL
	 *  SHOT. ⭐ Under the retired `t²` curve these two happened to agree at the ceiling (density
	 *  read exactly 1.0 there); that agreement was a property of a curve he has since overruled,
	 *  ⛔ and it was never the reason either function is shaped the way it is.
	 *
	 *  ⛔⛔ WHAT THIS DOES ⛔ NOT COVER — ACQUISITION ONLY, ⛔ NEVER A SHOT ALREADY IN FLIGHT.
	 *  His sentence is "ranged units will not be able to FIRE beyond this range" — that is the
	 *  decision to fire. It says nothing about an arrow already in the air, and there is a
	 *  shipped precedent directly on point: WITCH-§2 rules already-locked projectiles UNAFFECTED
	 *  because Projectile.cpp:352 is target-locked at FIRE time. Fog adopts the same rule, for
	 *  the same reason, in the same batch — and it is also the physical answer, since fog does
	 *  not stop an arrow. ⭐ The residual is tiny and is declared rather than hidden: because
	 *  firing is already clamped to the ceiling, an in-flight arrow can only ever be travelling
	 *  ~609.6 uu plus target drift. ⇒ a volley fired the instant before the fog lands still
	 *  connects. That is a declared consequence, ⛔ not a bug to be patched later.
	 *
	 *  ⛔ TOTALITY: a non-finite DistanceUU returns false (out of range) rather than propagating
	 *  a NaN comparison, which is the one case where the "fail toward no fog" default is wrong —
	 *  a garbage DISTANCE is a broken target, not a broken tuning, and acquiring it would push
	 *  the NaN into a move order.
	 *
	 *  @param DistanceUU        distance from viewer to target in Unreal units
	 *  @param RequestedRadiusUU the viewer's own acquisition range, pre-fog
	 *  @param bFogActive        is fog active right now
	 *  @param Tuning            the ceiling (the only member this path reads), the onset and the
	 *                           transmittance
	 *  @return                  true when the target is within the fogged effective radius
	 */
	static bool IsVisibleThroughFog(float DistanceUU, float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning);
};
