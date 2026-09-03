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
 *  ⛔ WHAT THIS FILE IS, AND — MORE IMPORTANTLY — WHAT IT IS NOT
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  It is the PURE RULES ONLY: the falloff shape from the onset to the ceiling, and the
 *  visibility predicate the acquisition range clamp consumes. It is floats in, floats out.
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
	 *  ⭐ THE CONSEQUENCE, so a retune is made with open eyes: this is where the fog BECOMES
	 *  VISIBLE, and everything closer than it is perfectly clear. Measured against the shipped
	 *  hero capsule (`InitCapsuleSize(42, 96)` ⇒ 192 uu tall, FOG-§2): 304.8 / 192 = 1.59 hero
	 *  body-lengths. ⚠️ Note that this — NOT the ceiling — is the "roughly one-and-a-half body
	 *  lengths" figure; the brief attached it to the wrong end of his own sentence and FOG-§2
	 *  corrects it. Raising this number shortens the ramp and makes the fog arrive as a wall;
	 *  lowering it fogs the melee band, which is currently exempt.
	 *
	 *  0 is legal and means "fog begins at the camera". Values >= the ceiling degenerate the
	 *  ramp to a hard step at the ceiling — total, documented, and asserted (FogDensityAt).
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
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "304.8"))
	float FogVisionCeilingUU = 609.6f; // FOG-§1: 20 ft × 30.48 cm/ft ⛔ ClampMin = the onset (FOG-§7b(a))

	/**
	 *  ⭐ DECLARED ADDITION OVER THE SPEC, ⛔ NOT SMUGGLED IN (the SC-§15 / FSiegeStuckTuning
	 *  "manager addition, declared not silent" idiom). TASK-837 pinned TWO distance constants;
	 *  this is a THIRD tunable and it is a SHAPE, not a distance — it cannot carry the 30× trap,
	 *  and FOG-§1's "no second literal" law is about 304.8 / 609.6, which it does not touch.
	 *
	 *  ⭐ WHY IT EXISTS: TASK-837 asked me to RULE on "linear or curved". A ruling that ships as
	 *  a hardcoded `2` is a ruling somebody has to recompile to disagree with. Shipped as a
	 *  tunable, "make it linear" is a one-word retune to 1.0 — which is exactly the argument
	 *  J-F2 made for the two distances, applied to the third number the feature needs.
	 *
	 *  THE RULING: 2.0 — a QUADRATIC EASE-IN. Both halves of his sentence point at it:
	 *    • "a LIGHT amount of fog STARTING at about 10 feet" — an ease-in leaves the onset with
	 *      zero slope, so the fog fades in from nothing. A LINEAR ramp is already at 10% density
	 *      one-tenth of the way into the band; the quadratic is at 1%. Linear puts a visible
	 *      EDGE where he asked for a light haze.
	 *    • "it gets THICKER AND THICKER" — that is an accelerating rate, not a constant one. A
	 *      linear ramp gets thicker at a fixed rate and is more naturally described as "it gets
	 *      thicker"; the doubled comparative is the acceleration.
	 *  ⇒ the curve gains 0.25 density over the first half of the band and 0.75 over the second,
	 *  and it is steepest exactly at the ceiling, where his sentence slams shut.
	 *
	 *  ⛔ NOT smoothstep: smoothstep eases in AND OUT, so it flattens as it approaches the
	 *  ceiling — it would arrive at "cannot see anything" gently, which is the one place his
	 *  sentence is absolute. ⛔ NOT Beer-Lambert (physical extinction): that curve is CONCAVE —
	 *  fastest at the start, decelerating — i.e. the exact opposite of "thicker and thicker",
	 *  and it is asymptotic, so it can never reach the state he named. (His rule is already
	 *  non-physical anyway: real homogeneous fog has no clear bubble, and he asked for one.)
	 *
	 *  Legal range is any value > 0. ⛔ 0, negative and non-finite values fall back to LINEAR
	 *  (1.0) rather than producing Pow(0, 0) == 1, which would render a wall of fog at the
	 *  onset. That fallback is total, documented, and asserted.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "0.01"))
	float FogDensityExponent = 2.f; // 2 = quadratic ease-in; 1 = linear
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
	 *  PURE. The fog's visual density at DistanceUU from the viewer, on [0, 1]:
	 *  0 = perfectly clear, 1 = effectively opaque ("cannot see anything").
	 *
	 *  ── THE SHAPE (his sentence, as arithmetic) ─────────────────────────────────────────
	 *
	 *      d <= FogVisionOnsetUU                 ->  EXACTLY 0        (the clear bubble)
	 *      onset < d < ceiling                   ->  t^Exponent, where t = (d - onset)
	 *                                                              / (ceiling - onset)
	 *      d >= FogVisionCeilingUU               ->  EXACTLY 1        (⛔ a HARD CUT)
	 *
	 *  ⭐⛔ THE CEILING IS A HARD CUT, ⛔ NOT AN ASYMPTOTE, and that is a ruling with three
	 *  reasons behind it:
	 *    1. His sentence is absolute — "they really cannot see ANYTHING beyond 20 feet". An
	 *       asymptote that reads 0.97 at the ceiling still leaves 3% visibility, and at
	 *       Longbowman range 3% visibility is still a lethal shot. An asymptote makes his
	 *       ceiling a suggestion.
	 *    2. The gameplay clamp is binary by nature — a target is on the acquisition list or it
	 *       is not (TASK-838's min(Range, ceiling)). If the density were asymptotic while the
	 *       acquisition were hard, the picture and the mechanic would disagree at exactly the
	 *       boundary the player is judging the card by.
	 *    3. A hard cut is assertable EXACTLY (density(ceiling) == 1.0, to the bit). An asymptote
	 *       can only ever be asserted to a tolerance and can never be shown to have reached the
	 *       state his sentence names.
	 *
	 *  ⚠️ THIS IS THE DESIGN CURVE, ⛔ NOT A MATERIAL PARAMETER. TASK-841 owns the visual and
	 *  will map this onto the vendor pack's own noise/wind/shaft features; "as realistic as
	 *  possible" is achieved there, by instrument, ⛔ not by making this function physical.
	 *
	 *  ⛔ TOTALITY, every branch reachable and tested:
	 *    - a non-finite DistanceUU returns 0 (clear) — garbage never blinds anybody;
	 *    - a non-finite onset or ceiling returns 0 (clear) — a broken tuning yields;
	 *    - ceiling <= onset (inverted or zero-width band) is the continuous limit of the ramp
	 *      as its width goes to zero, i.e. a HARD STEP at the ceiling. No divide by zero;
	 *    - a non-positive or non-finite exponent falls back to linear (see the tunable's note);
	 *    - the result is clamped to [0, 1] belt-and-braces, though the branches above already
	 *      guarantee it.
	 *
	 *  @param DistanceUU distance from the viewer in Unreal units (centimetres)
	 *  @param Tuning     the onset, the ceiling and the falloff exponent
	 *  @return           density on [0, 1]; 0 clear, 1 effectively opaque
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
	 *  @param Tuning            the onset, the ceiling and the falloff exponent
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
	 *  ⚠️ SO THE TWO FUNCTIONS MEET AT THE CEILING WITH DIFFERENT ANSWERS, AND IT IS DELIBERATE:
	 *  at d == 609.6 exactly, FogDensityAt returns 1.0 ("effectively opaque" — the renderer's
	 *  end state) while this returns TRUE ("still in range" — the combat gate's last metre).
	 *  They are different consumers asking different questions at a single float of measure
	 *  zero; the alternative is to break the project-wide `<=` idiom to tidy up a boundary
	 *  nobody can see. ⛔ Recorded here so it reads as a choice rather than an oversight.
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
	 *  @param Tuning            the onset, the ceiling and the falloff exponent
	 *  @return                  true when the target is within the fogged effective radius
	 */
	static bool IsVisibleThroughFog(float DistanceUU, float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning);
};
