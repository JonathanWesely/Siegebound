// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// FSlateBrush is a BY-VALUE member below (the elevation background's brush, TASK-684), so the
// complete type is required here — a forward declaration cannot size a member.
#include "Styling/SlateBrush.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "WarMapWidget.generated.h"

class AActor;
class APlayerController;
class UButton;
class USiegeAssistantSnapshot;
/**
 *  ⭐ TASK-745 (`MARK-§5`) — the MARK STORE, forward-declared ONLY. `SiegeMapMarkSubsystem.h`
 *  is included in the .cpp and ⛔ never here: this header is already the heaviest surface in
 *  the war-map lane, and the store is a `ULocalPlayerSubsystem` whose header drags the local
 *  player in. ⛔ `FSiegeMapMark` itself is not needed at this scope AT ALL — `MakeMarkPickSymbol`
 *  takes an `int32` and `FSiegeWarMapMarkCircle` below stores an `int32` — so `SiegeMapMark.h`
 *  is a .cpp include too. (TASK-744 owns both files; this file only CONSUMES their pinned API.)
 */
class USiegeMapMarkSubsystem;
class USiegeScatterConfig;
class UTextBlock;
class UTexture2D;
class UWorld;

/**
 *  ⚖️ A SEPARATE CATEGORY FROM `LogSiegeAssistant`, AND THE SEPARATION IS THE POINT
 *  RATHER THAN TIDINESS. The war map is a DISPLAY that happens to sit next to the
 *  assistant; it is not part of the prompt lane. Keeping its diagnostics on their own
 *  category is what makes "the map never touched the assistant" GREPPABLE — TASK-565
 *  re-runs exactly that kind of sweep and expects ZERO coupling. A shared category
 *  would put map noise inside the one log a reviewer reads to audit the airlock.
 *  ⚠️ DECLARED ADDITION over the TASK-560 `names:` block (`SC-§15`).
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeWarMap, Log, All);

/**
 *  ⭐⛔ THE MAP'S **ONLY** OUTBOUND CHANNEL, AND IT CARRIES A SYMBOL — NEVER A NUMBER.
 *
 *  `PlaceSymbol` is one of the seven `PlaceVocabulary` symbols, taken verbatim from
 *  `USiegeAssistantSnapshot::GetPlaceNames()`. ⛔ No coordinate, no dot, no count and no
 *  marker geometry rides on this delegate, and nothing else leaves this class at all
 *  (`WR-§6`). The binder (TASK-563) hands the symbol to the console's input-insert seam
 *  (TASK-561); THE PLAYER STILL PRESSES ENTER HIMSELF, so the assistant's contract,
 *  grammar, schema and Zone A are all byte-untouched.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWarMapPlacePicked, FName, PlaceSymbol);

/** Fires on every open/close transition. TASK-563 binds this to restore its cursor/input posture — the `OnConsoleOpenChanged` shape, cloned. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWarMapOpenChanged, bool, bOpen);

/**
 *  ⛔ A BUTTON CLICK, NOT A REQUEST — AND THE NAME IS DELIBERATE (`WR-§7`).
 *
 *  This widget never requests, prices, validates or spends anything. It reports that the
 *  player clicked a button on it. TASK-563's controller decides whether that click is a
 *  reveal request, what it costs, whether the player can afford it, and whether any gold
 *  moves — all on the AUTHORITY, through the shipped `ASiegePlayerState::SpendGold`.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarMapRevealButtonClicked);

/** Defined below `FSiegeWarMapProjection`, which takes it by reference. Forward-declared here so the parameter type is never an elaborated-type-specifier inside a parameter list (legal, but the kind of subtlety a reader has to stop and verify). */
struct FSiegeWarMapMarker;

/** TASK-745's player-drawn circle, resolved into widget-local space. Same forward-declaration reason as `FSiegeWarMapMarker` above. */
struct FSiegeWarMapMarkCircle;

/**
 *  ═══ THE WAR MAP'S PURE GEOMETRY (TASK-560; CONVENTIONS `WR-§6`) ═══
 *
 *  ⛔ PURE. No `UWorld`, no `AActor`, no `UObject`, no engine subsystem, no state — values
 *  in, values out. ⭐ That is what lets TASK-564 test the WHOLE projection chain HEADLESSLY,
 *  which is the property `WR-§6` asks for when it says "make the projection a pure, testable
 *  free function".
 *
 *  Not a UObject / not reflected: a plain static library, no `Build.cs` change (`Core`
 *  already covers `FVector2D`). Shape precedents in this module: `FSiegeCombatStatics`,
 *  `FSiegeAssistantRegionStatics`, `FSiegeKeyboardLayoutStatics`, `FSiegeStuckStatics`.
 *
 *  ⛔⛔ THERE IS NOT ONE HAND-TYPED ARENA DIMENSION IN THIS FILE PAIR. Every extent arrives
 *  as a PARAMETER, sourced by the widget from `USiegeScatterConfig::ArenaHalfExtent` — the
 *  single owner (`SC-§34`'s structural escape, and `WR-§6`'s projection row).
 *
 *  📌 M8: adds no replicated property, no new replicated class, no new relevancy tier.
 */
struct GITCLAUDEUNREALTEST_API FSiegeWarMapProjection
{
	/**
	 *  ⛔ THE ZERO-DIVIDE FLOOR, AND IT IS COPIED RATHER THAN INVENTED. `WR-§6` requires
	 *  "never a silent zero-divide"; the shipped answer to the identical question already
	 *  exists at `ABattlefieldScatter` (`FMath::Max(ScatterConfig->ArenaHalfExtent.X, 1.f)`,
	 *  BattlefieldScatter.cpp:566 and :1352). Same number, same reason, so the two lanes
	 *  cannot disagree about what a degenerate arena means.
	 */
	static constexpr float MinArenaHalfExtentUu = 1.f;

	/**
	 *  WORLD `(X, Y)` → NORMALISED MAP UV, CLAMPED TO `[0,1]²`.
	 *
	 *  ── ORIENTATION, PINNED SO A LATER READER DOES NOT RE-DECIDE IT (TASK-692, `WM-§7`) ──
	 *    UV.X = 0 at world X = −HalfX  ⇒  world **+X grows RIGHT** on screen.
	 *    UV.Y = 0 at world Y = −HalfY  ⇒  world **+Y grows DOWN** on screen.
	 *
	 *  ⚠️ BOTH AXES ARE THE SAME AFFINE FORM — ⛔ THERE IS NO Y INVERSION, AND THAT IS THE
	 *  CORRECTION JONATHAN'S FIRST MAP TEST BOUGHT (`WM-§7`): Unreal's world frame is
	 *  LEFT-HANDED (X forward, Y right, Z up — at identity yaw `GetRightVector()` IS +Y), so
	 *  on a top-down map that draws +X to the RIGHT, world +Y physically lies 90° CLOCKWISE
	 *  from +X seen from above — toward the map's BOTTOM. Slate's downward local Y therefore
	 *  already runs the CORRECT way; the pre-692 "flip" that inverted Y here is what rendered
	 *  the battlefield mirrored about the castle lane (what lay on the player's left drew on
	 *  his right). The long arena axis is X (`ArenaHalfExtent` ships `(26000, 12000)`), which
	 *  is also the castle-to-castle axis, so X-horizontal puts the two castles left and
	 *  right — the reading a player expects of a battlefield map.
	 *
	 *  ⚠️ CLAMPED, NOT DROPPED. A unit that has wandered past the configured arena bound
	 *  pins to the map edge rather than vanishing or drawing outside the panel. A vanished
	 *  ally is a lie; an edge-pinned one is visibly at the edge.
	 *
	 *  @param WorldXY          world-space X and Y in uu. Z is IGNORED — this is a top-down display.
	 *  @param ArenaHalfExtent  `USiegeScatterConfig::ArenaHalfExtent`. Each axis is floored at
	 *                          `MinArenaHalfExtentUu` internally, so `(0,0)` is safe and yields
	 *                          a degenerate-but-finite map instead of a NaN.
	 */
	static FVector2D WorldToMapUV(const FVector2D& WorldXY, const FVector2D& ArenaHalfExtent);

	/**
	 *  THE DRAWABLE MAP RECT INSIDE A PANEL, ASPECT-PRESERVED AND CENTRED.
	 *
	 *  ⚠️ ASPECT IS PRESERVED ON PURPOSE. The arena is ~2.17:1 and a 16:9 panel is ~1.78:1;
	 *  stretching UV across the raw panel would make the map lie about relative distance —
	 *  two dots equally far apart in the world would read as different distances depending on
	 *  which way they were separated. The map is letterboxed instead: the largest rect of the
	 *  ARENA'S aspect that fits inside (panel − padding), centred.
	 *
	 *  ⚠️ DEGENERATE INPUTS FAIL CLOSED, NEVER NEGATIVE: a panel smaller than twice the
	 *  padding yields a zero-size rect at the panel centre, which draws nothing and hit-tests
	 *  nothing. ⛔ It never returns a negative size (which would invert every hit rect).
	 *
	 *  @param PanelLocalSize   the widget's local size (`FGeometry::GetLocalSize()`).
	 *  @param PaddingPx        inset on all four sides, in local px. Negatives are treated as 0.
	 *  @param ArenaHalfExtent  the arena extent; drives the target aspect only.
	 *  @param OutRectOrigin    top-left of the drawable rect, in widget-local px.
	 *  @param OutRectSize      size of the drawable rect, in widget-local px. Never negative.
	 */
	static void ComputeMapRectLocal(
		const FVector2D& PanelLocalSize,
		float PaddingPx,
		const FVector2D& ArenaHalfExtent,
		FVector2D& OutRectOrigin,
		FVector2D& OutRectSize);

	/** Normalised UV → widget-local px inside a rect produced by `ComputeMapRectLocal`. Pure, trivial, and separate so a test can pin each step of the chain independently. */
	static FVector2D MapUVToLocal(const FVector2D& MapUV, const FVector2D& RectOrigin, const FVector2D& RectSize);

	/**
	 *  NORMALISED MAP UV → WORLD `(X, Y)` — the EXACT INVERSE of `WorldToMapUV` on `[0,1]²`.
	 *
	 *  ⚠️ DECLARED ADDITION (`SC-§15`), AND IT LIVES **HERE** RATHER THAN INLINE IN THE BAKE
	 *  LOOP FOR THE SINGLE-OWNER REASON: the elevation bake (TASK-684, `WM-§2`) must generate
	 *  one world sample point per texel, i.e. it needs the world↔UV mapping RUN BACKWARDS.
	 *  Writing `(2u−1)·HalfX` inline in `WarMapWidget.cpp` would be a SECOND, unshared copy of
	 *  the orientation contract — exactly the "re-derive the transform" drift this struct
	 *  exists to make impossible. Both directions now live in the one owner, and the round
	 *  trip `WorldToMapUV(MapUVToWorld(UV)) == UV` is pinned by a headless test.
	 *
	 *  ⚠️ SAME ORIENTATION, SAME ZERO-DIVIDE FLOOR, ⛔ NO CLAMP: the forward map clamps
	 *  because out-of-arena ACTORS exist and must pin to the edge; the inverse is only ever
	 *  fed texel centres in `(0,1)`, and an unclamped inverse is what keeps the round trip
	 *  byte-exact there. Out-of-range UVs extrapolate linearly — callers own their range.
	 */
	static FVector2D MapUVToWorld(const FVector2D& MapUV, const FVector2D& ArenaHalfExtent);

	/**
	 *  ⭐ THE HIT TEST, AND IT IS A **WIDGET RECT** TEST — ⛔ NEVER A WORLD RADIUS (`WR-§6`).
	 *
	 *  ⚖️ THIS IS THE CLAUSE THAT KEEPS `AS-§21.4` HONOURED RATHER THAN ARGUED AROUND. A
	 *  world radius around a place symbol would be a SEMANTIC claim the model reasons over,
	 *  and `AS-§21.4` reserves that number for Jonathan. A pixel rect on a panel is a UI
	 *  affordance the player feels with the mouse; it can never mis-select a unit, never
	 *  reach a prompt, and never mean anything in world space.
	 *
	 *  ⚠️ LAST MATCH WINS ON OVERLAP, and that is deliberate: markers are painted in
	 *  vocabulary order, so the LAST one in the array is the one drawn ON TOP. Picking the
	 *  first would let the player click a marker and get the symbol of one hidden beneath it.
	 *
	 *  @return index into `Markers`, or `INDEX_NONE` when the point is on empty map.
	 */
	static int32 FindMarkerIndexAtLocal(
		const TArray<FSiegeWarMapMarker>& Markers,
		const FVector2D& LocalPoint);

	//~ ---------------------------------------------------------------------
	//~ ═══ MAP MARKS — the player's numbered circles (TASK-745; `MARK-§`) ═══
	//~
	//~ ⭐ EVERY RULE THE MARK FEATURE HAS THAT CAN BE STATED AS ARITHMETIC LIVES
	//~ HERE, AS A PURE STATIC, AND THAT IS `W4-R1`'s LAW APPLIED AT AUTHORING TIME
	//~ RATHER THAN AFTER A RED GATE: "when a spec says *and the tests assert X*, it
	//~ must ALSO say WHERE X is readable from — a testability obligation without a
	//~ testability seam is an unfunded mandate."
	//~
	//~ ⇒ The four assertions TASK-745's spec names — the symbol, the shared rect,
	//~ right-click-on-empty-does-nothing, wheel-outside-a-mark-does-nothing — are
	//~ all readable from functions on this struct plus `UWarMapWidget::
	//~ MakeMarkPickSymbol`, with ⛔ no world, ⛔ no Slate application and ⛔ no
	//~ subsystem. The widget's handlers are then thin: they LOOK UP and they ACT;
	//~ they do not DECIDE.
	//~ ---------------------------------------------------------------------

	/**
	 *  WIDGET-LOCAL PX → NORMALISED MAP UV — the EXACT INVERSE of `MapUVToLocal`, and it
	 *  ⛔ REFUSES rather than extrapolating.
	 *
	 *  ⚠️⚠️ THE REFUSAL IS THE POINT AND IT IS A CORRECTNESS GUARD, ⛔ NOT TIDINESS. The map
	 *  rect is LETTERBOXED inside the panel (`ComputeMapRectLocal` preserves the arena aspect),
	 *  so a large part of the widget is NOT map at all. A click there is not a position on the
	 *  battlefield, and clamping it to the rim would place a mark on ground the player did not
	 *  point at — a mark whose centre `ResolvePlace` will later answer with. ⚖️ The forward map
	 *  CLAMPS because out-of-arena ACTORS exist and must pin visibly to the edge; this
	 *  direction REFUSES because an out-of-rect CLICK is not a place at all. The two are not
	 *  inconsistent: one is displaying something real, the other is inventing something.
	 *
	 *  ⚠️ THE CONTAINMENT TEST IS ON THE **RECT**, ⛔ not on the normalised value, and the
	 *  boundary is INCLUSIVE (the shipped `IsPointInZone` idiom) — so the map's outermost pixel
	 *  row and column are map rather than chrome, and a point sitting exactly on the rect's far
	 *  edge cannot be refused by a one-ulp division overshoot. The .cpp carries the arithmetic.
	 *
	 *  @return false — and `OutMapUV` untouched — for a point outside the rect, or a degenerate
	 *          (zero/negative) rect. ⛔ Never a divide by zero.
	 */
	static bool LocalToMapUV(
		const FVector2D& LocalPoint,
		const FVector2D& RectOrigin,
		const FVector2D& RectSize,
		FVector2D& OutMapUV);

	/**
	 *  ⭐⭐ WORLD uu → WIDGET-LOCAL PX, FOR A **RADIUS**, AND THE WHOLE `MARK-§4`/REGISTRY
	 *  RECONCILIATION LIVES AT THIS ONE FUNCTION. Read it before changing either side.
	 *
	 *  TWO LAWS TOUCH HERE AND BOTH ARE OBEYED, WHICH IS ONLY POSSIBLE BECAUSE OF THIS SEAM:
	 *    • The PINNED CROSS-TASK REGISTRY stores `FSiegeMapMark::RadiusUU` — **world uu** —
	 *      and TASK-744/746 compile against that field character-for-character. ⛔ It may not
	 *      be re-typed into pixels to suit this widget.
	 *    • `MARK-§4` requires the WHEEL's tunables to be **widget-space**, separately named,
	 *      and ⛔ NOT the world-space `GroupRadiusWheelStep`/`Min`/`Max` numbers.
	 *  ⇒ The STORE is world-space; the INTERACTION is pixel-space; this function is the only
	 *  crossing, and its inverse below is the only crossing back.
	 *
	 *  ⭐ AND THE WORLD-SPACE STORE IS THE RIGHT SIDE OF THAT SEAM ON ITS OWN MERITS: a mark
	 *  denotes GROUND. A radius stored in pixels would denote a different amount of ground on a
	 *  different monitor, a different window size, or after a resize — so `circle_1` would
	 *  quietly mean a different place than the one the player drew. That is the same class of
	 *  failure `M-1`'s no-renumbering rule exists to prevent, arriving through geometry instead
	 *  of through bookkeeping.
	 *
	 *  ⚠️ THE SCALE IS A SINGLE SCALAR AND THAT IS PROVEN, ⛔ NOT ASSUMED. `ComputeMapRectLocal`
	 *  builds a rect of EXACTLY the arena's aspect, so `RectSize.X / (2·HalfX)` and
	 *  `RectSize.Y / (2·HalfY)` are equal by construction ⇒ a circle in the world is a circle
	 *  on the map and never an ellipse. `Siegebound.WarMap.MarkRadiusScaleIsUniformOnBothAxes`
	 *  asserts it, so a future edit that broke the aspect preservation would be caught HERE
	 *  rather than shipping subtly-oval marks.
	 *
	 *  Degenerate inputs return 0 — a zero-radius circle draws nothing and hit-tests nothing,
	 *  the `ComputeMapRectLocal` fail-closed direction. ⛔ Never negative, ⛔ never NaN.
	 */
	static float MapWorldRadiusToLocalPx(
		float RadiusUU,
		const FVector2D& ArenaHalfExtent,
		const FVector2D& RectSize);

	/** The EXACT inverse of `MapWorldRadiusToLocalPx`, same floor, same fail-closed 0. The wheel steps in px and writes uu, so the round trip is pinned by test. */
	static float MapLocalPxToWorldRadius(
		float RadiusPx,
		const FVector2D& ArenaHalfExtent,
		const FVector2D& RectSize);

	/**
	 *  ⭐ THE MARK HIT TEST — A **RADIAL** TEST, ⛔ NOT A RECT, AND THE DIFFERENCE IS THE
	 *  `WR-§6` RENDERING RULING RATHER THAN A PREFERENCE: a mark is DRAWN as a circle, so it
	 *  must be HIT as a circle. A rect around a drawn circle would give the player four corners
	 *  that delete a mark he is visibly not pointing at — "what you see and what you click" in
	 *  disagreement, which is the exact failure that ruling exists to make impossible.
	 *
	 *  ⚠️ THE WHOLE DISC IS LIVE, ⛔ not just the ring stroke. The ring is how the mark is DRAWN;
	 *  the mark IS the area it encloses, and asking the player to hit a 3-px stroke with a
	 *  mouse would be a worse map, not a more honest one.
	 *
	 *  ⚠️ LAST MATCH WINS ON OVERLAP — the `FindMarkerIndexAtLocal` doctrine, verbatim and for
	 *  the identical reason: circles are painted in store order, so the LAST one is the one
	 *  drawn on top, and picking the first would hand the player a mark hidden beneath another.
	 *
	 *  ⚠️ BOUNDARY INCLUSIVE (`<=`), the shipped `IsPointInZone` idiom, so "on the edge" means
	 *  the same thing everywhere in this codebase.
	 *
	 *  @return index into `Circles`, or `INDEX_NONE` — which is what "the cursor is on empty
	 *          map" means to BOTH the right-click delete and the wheel resize, and is therefore
	 *          the readable form of *"a right-click on empty map does nothing"* and *"the wheel
	 *          outside a mark does nothing"*.
	 */
	static int32 FindMarkIndexAtLocal(
		const TArray<FSiegeWarMapMarkCircle>& Circles,
		const FVector2D& LocalPoint);

	/**
	 *  ONE WHEEL NOTCH → the new mark radius in LOCAL PX, clamped to `[MinPx, MaxPx]`.
	 *
	 *  ⚠️ SIGN, ⛔ NOT MAGNITUDE. `FPointerEvent::GetWheelDelta()` is platform- and
	 *  driver-dependent in magnitude (a free-spinning wheel or a trackpad can deliver
	 *  fractional or multi-unit deltas), so one EVENT is one STEP in the delta's direction. ⇒
	 *  the felt speed is `StepPx`, a designer number, and never the mouse driver's.
	 *
	 *  ⭐ A ZERO DELTA RETURNS THE INPUT UNCHANGED, and that is an assertion rather than an
	 *  accident: it is half of *"the wheel does nothing"* (the other half is `INDEX_NONE`
	 *  above), and a resize that fired on a zero delta would make the map twitch on every
	 *  gesture event the platform decided to synthesize.
	 *
	 *  Degenerate tunables fail closed: a non-positive `StepPx` returns the input; `MinPx` is
	 *  floored at 1 px and `MaxPx` at `MinPx`, so an inverted pair collapses to a fixed size
	 *  instead of producing a negative radius that would invert the hit test.
	 */
	static float StepMarkRadiusPx(
		float CurrentRadiusPx,
		float WheelDelta,
		float StepPx,
		float MinPx,
		float MaxPx);

	/**
	 *  ⭐ THE CENTRING RULE, AS ARITHMETIC — the top-left at which a text block of
	 *  `MeasuredTextSize` is CENTRED on `Centre`.
	 *
	 *  ⚖️ IT IS A NAMED FUNCTION FOR EXACTLY ONE REASON, AND IT IS `W4-R1`'s: Jonathan's
	 *  sentence is *"it gets its own number in the middle of it"*, so "in the middle" is a
	 *  contract, and a contract inlined into a `MakeText` call inside a `const` paint pass is
	 *  unreadable by any test. ⚠️ Note the CONTRAST with the seven place markers' labels, which
	 *  are deliberately LEFT-ANCHORED beside their glyph because centring them "needs the font
	 *  measure service" — this feature pays that cost because the spec demands the centre,
	 *  and the measure itself comes from Slate's own service at paint (see `NativePaint`).
	 */
	static FVector2D CentreTextTopLeft(const FVector2D& Centre, const FVector2D& MeasuredTextSize);

	/**
	 *  ⭐ THE *"LEGIBLE AT ANY RADIUS"* RULE, AS ARITHMETIC — the number's point size for a
	 *  circle of `RadiusPx`: `Radius × RadiusFraction`, CLAMPED to `[MinSize, MaxSize]`.
	 *
	 *  ⚖️ IT IS A SEAM FOR `W4-R1`'s REASON, AND THE OBLIGATION IS SPELLED OUT IN TASK-745's
	 *  OWN SPEC — *"how the number is centred and stays legible at any radius"*. Inlined in the
	 *  `const` painter it would be a claim nobody could read; here, one test can sweep the whole
	 *  radius range and assert the size never leaves its window at either end.
	 *
	 *  ⚠️ THE CLAMP IS THE WHOLE POINT, ⛔ NOT A SAFETY NET — the same relationship
	 *  `HeightToBrightness`'s clamp has to the elevation ramp. Without the floor a
	 *  minimum-radius circle would carry an unreadable smudge instead of the name the player
	 *  speaks to his commander; without the ceiling a maximum-radius one would stamp a digit
	 *  across the battlefield and bury the dots the map exists to show.
	 *
	 *  Total for any input: a non-positive radius returns `MinSize`, `MinSize` is floored at
	 *  1 pt, and `MaxSize` is floored at `MinSize` — so an inverted or zeroed pair collapses to
	 *  a fixed readable size rather than to nothing.
	 */
	static float MarkNumberFontSizePx(float RadiusPx, float RadiusFraction, float MinSize, float MaxSize);
};

/**
 *  ONE PLACE MARKER, RESOLVED INTO WIDGET-LOCAL SPACE.
 *
 *  ⭐⛔ THE SINGLE SOURCE OF TRUTH SHARED BY THE PAINTER AND THE HIT TEST. `NativePaint` and
 *  `NativeOnMouseButtonDown` both build this array from the SAME function against the SAME
 *  geometry, so what you can SEE and what you can CLICK physically cannot disagree
 *  (`WR-§6`'s rendering row, and the reason that row exists).
 *
 *  Plain struct, not a `USTRUCT`: it is never stored on the widget, never replicated and
 *  never seen by Blueprint — it is built per paint and per click and discarded.
 */
struct FSiegeWarMapMarker
{
	/** The canonical `PlaceVocabulary` symbol, verbatim from `USiegeAssistantSnapshot::GetPlaceNames()`. ⛔ This file never spells one itself. */
	FName PlaceSymbol = NAME_None;

	/** Marker centre in widget-local px. */
	FVector2D LocalCentre = FVector2D::ZeroVector;

	/** HALF the hit rect, in widget-local px. ⛔ A WIDGET RECT — never a world radius. */
	FVector2D LocalHitHalfSize = FVector2D::ZeroVector;
};

/**
 *  ONE PLAYER-DRAWN MAP MARK, RESOLVED INTO WIDGET-LOCAL SPACE (TASK-745; `MARK-§`).
 *
 *  ⭐⛔ THE SINGLE SOURCE OF TRUTH SHARED BY THE PAINTER, THE CLICK, THE RIGHT-CLICK AND THE
 *  WHEEL. All four build this array from the SAME function (`UWarMapWidget::BuildMarkCircles`)
 *  against the SAME geometry, so what you can SEE, what you can NAME, what you can DELETE and
 *  what you can RESIZE physically cannot disagree — the `FSiegeWarMapMarker` doctrine above,
 *  extended to a feature with four verbs instead of one.
 *
 *  Plain struct, not a `USTRUCT`: never stored on the widget, never replicated, never seen by
 *  Blueprint — built per paint and per input event, then discarded. ⛔ The DURABLE state is
 *  `USiegeMapMarkSubsystem`'s, in world uu; this is its projection for one frame.
 */
struct FSiegeWarMapMarkCircle
{
	/**
	 *  The mark's PERMANENT IDENTITY, `1..MaxMapMarks`, copied verbatim from
	 *  `FSiegeMapMark::Number`.
	 *
	 *  ⛔⛔ ⛔ NOT AN INDEX INTO THIS ARRAY AND ⛔ NOT A POSITION IN ANY LIST — `M-1`. Deleting
	 *  circle 2 of 3 LEAVES THE HOLE, so `Circles[1].Number` is `3` afterwards, and every verb
	 *  in this file addresses the store BY NUMBER rather than by index for exactly that reason.
	 *  The decisive argument is the airlock, ⛔ not ergonomics: the map writes `circle_2` into
	 *  the console's input box and THE PLAYER sends it himself, so there is a window of
	 *  arbitrary length in which a renumber would silently redirect an order he already
	 *  composed.
	 */
	int32 Number = 0;

	/** Circle centre in widget-local px, from the shipped `WorldToMapUV` → `MapUVToLocal` chain — byte-identical to the dots' and the markers', so a mark and a dot can never disagree about where the arena is. */
	FVector2D LocalCentre = FVector2D::ZeroVector;

	/** Drawn (and hit) radius in widget-local px, from `MapWorldRadiusToLocalPx`. ⛔ A UI affordance in PIXELS; the DURABLE radius is the store's world-uu one, and this is that value seen through this frame's map scale. */
	float LocalRadiusPx = 0.f;
};

/**
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *  THE BATTLEFIELD WAR MAP (TASK-560; CONVENTIONS `WR-§6`, `WR-§9`)
 *  `UWarMapWidget`  ↔  `/Game/UI/WBP_WarMap`   (the `U<Name>Widget` ↔ `WBP_<Name>` law)
 *  ═══════════════════════════════════════════════════════════════════════════════════════
 *
 *  Jonathan's directive, verbatim, because it IS the spec: *"there is a map of the entire
 *  battlefield that you walk up to and make it appear on your entire screen, and then click
 *  on different locations to easily communicate about certain points of interest with the AI
 *  powered NPC… The map stays at the castle and it updates with dots that show ally
 *  locations. You can pay 30 gold to reveal all enemy locations and then the red dots will
 *  appear on the map, but the enemy locations go away as soon as you close the map."*
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  1. ⛔⛔ THE ONE RULE THAT SHAPES EVERY LINE BELOW: **THIS WIDGET IS A DISPLAY.**
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔ **NO DOT, COORDINATE, POSITION, COUNT OR MARKER GEOMETRY FROM THIS CLASS EVER ENTERS
 *  ANY PROMPT ZONE.** ✅ The ONLY thing it emits is a PLACE SYMBOL STRING on
 *  `OnPlacePicked` — which the binder hands to the console's input box, and which THE PLAYER
 *  STILL SENDS HIMSELF.
 *
 *  ⇒ ⭐ **THIS FEATURE SPENDS ZERO PROMPT CHARACTERS.** Zone A stays byte-frozen at its
 *  named, dated baseline of 5658 chars (2026-08-05) and TASK-564 asserts that it did not
 *  move. ⛔ ASSERT IN CHARS/BYTES, NEVER IN TOKENS: `AS-§12g` pins every token figure
 *  STALE-PENDING-RE-MEASUREMENT and the shipped `zoneA_tok` has never been printed, so no
 *  comment, handoff or report in this batch may quote or derive one.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  2. ⭐⭐ CLICK → SYMBOL. ⛔ NEVER CLICK → COORDINATE.
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  An arbitrary click on a battlefield map is a CONTINUOUS WORLD COORDINATE, and the shipped
 *  vocabulary has NO PRIMITIVE FOR ONE — `SiegeAssistantCommand.h` is explicit that *"every
 *  field is a SYMBOL, never a coordinate… the model never sees a number that means a
 *  position."*
 *
 *  ⇒ ✅ **ONLY THE SEVEN `PlaceVocabulary` MARKERS ARE HIT-TESTABLE**, drawn at their
 *  `USiegeAssistantSnapshot::ResolvePlace` positions.
 *
 *  ⚠️⚠️ **AMENDED BY TASK-745 (`MARK-§`), AND THE AMENDMENT IS DECLARED RATHER THAN SLIPPED IN
 *  (`SC-§15`): `WR-§9` OUTCOME 1 — *"a click on empty map area does nothing"* — IS ⛔ NO LONGER
 *  TRUE, BY JONATHAN'S OWN DIRECTIVE.** His words: *"click on anywhere on the map, to create a
 *  circle on the map at that location… Every time you make a new circle, it gets its own number
 *  in the middle of it."* ⇒ **an empty-map LEFT click now PLACES A NUMBERED MARK** (§8 below).
 *  ⭐ **THE AIRLOCK IS UNCHANGED AND THAT IS WHY THE AMENDMENT IS CHEAP:** the click still emits
 *  a **SYMBOL** (`circle_1`) and never a coordinate — `MARK-§1`'s *"`WR-§6`'s click→symbol trick,
 *  applied a second time and for the same reason."* ⛔ The shipped no-op arm SURVIVES VERBATIM
 *  wherever the mark store is unreachable, so the retired behaviour is a degrade path rather
 *  than deleted code. ⛔ A click on empty map still does nothing on a RIGHT button.
 *
 *  ⛔ THE INSERTED TEXT IS THE LITERAL SYMBOL (`ancient_ground_near`), not English, and the
 *  reason is measured rather than aesthetic: `DEV-01` recorded the natural-English *"to the
 *  nearest ancient ground"* resolving to `nearest_mine`. The symbol is the exact token the
 *  model must emit, and it is verifiable by string equality in a unit test (`WR-§9`
 *  outcome 2). ⚠️ An English rendering is FLAGGED to Jonathan and re-opens `DEV-01`.
 *
 *  ⛔ NO NEW PLACE SYMBOL, GRID CELL, SNAP RADIUS, COORDINATE FIELD OR EIGHTH `who` SHAPE
 *  APPEARS HERE. This file does not spell a single place symbol: the marker list is READ
 *  from `GetPlaceNames()`, so a vocabulary that never grows cannot be grown by this file
 *  either, and a vocabulary that DOES grow picks this map up for free.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  3. ⛔⛔ THE SNAPSHOT HAZARD — AND THE FINDING IT PRODUCED (`WR-§6`, `SC-§15`)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  `ResolvePlace` lives on `USiegeAssistantSnapshot`, which the assistant captures ONCE PER
 *  TYPED SENTENCE to BUILD THE PROMPT. ⛔ **OPENING A UI PANEL MUST NEVER TRIGGER A
 *  `Capture()` AS A SIDE EFFECT** — a survey the player did not ask for would answer a
 *  different question from the one the model was asked, on a class whose own header says in
 *  terms "⛔ Do NOT re-Capture() from the executor".
 *
 *  ✅ A READ-ONLY ROUTE EXISTS AND IS THE ONE TAKEN: `GetOwningPlayer()` →
 *  `ASiegePlayerController::GetAssistantComponent()` → `USiegeAssistantComponent::
 *  GetTurnSnapshot()` (public, `const`, returns the EXISTING object). ⛔ `Capture()` is not
 *  called, referenced or reachable from this file.
 *
 *  ⚠️⚠️ **THE RESIDUAL, REPORTED RATHER THAN CODED AROUND — AND ITS MECHANISM WAS
 *  RE-DIAGNOSED, SO READ THE CORRECTED STORY, NOT THE 560-ERA ONE (TASK-691, ruling
 *  `W691-3`; corrected here by TASK-685's rider).** `GetTurnSnapshot()` is in fact
 *  **NON-NULL from match start**: `USiegeAssistantComponent::BeginPlay()` calls
 *  `EnsureSnapshot()`, and has since the assistant first landed (TASK-447, `cd5f4ed`).
 *  What is empty is the OBJECT — `PlaceNames` has exactly ONE append site, `Capture()`'s
 *  resolved-slot loop, reachable only from the turn path. ⇒ **A player who opens the war
 *  map before ever typing into the console sees ally dots but NO PLACE MARKERS — via the
 *  snapshot-present-but-never-captured route, not a null pointer.** The map is otherwise
 *  fully functional and self-heals the moment one sentence is sent; TASK-580 (in flight,
 *  same wave) seeds one REAL at-rest capture per match so the first open is populated.
 *
 *  ⛔ THIS FILE DOES NOT FIX THAT, AND THE REFUSAL IS THE POINT. Every fix crosses a
 *  boundary this task may not cross: forcing a `Capture()` is the exact side effect `WR-§6`
 *  forbids; capturing into a second, map-owned snapshot builds the parallel survey `§4`
 *  rejects on sight; and priming the component at `BeginPlay` is an edit to
 *  `SiegeAssistantComponent.{h,cpp}`, which TASK-560's `names:` block lists as ⛔ NOT
 *  TOUCHED. ⇒ It is escalated in `handoffs/TASK-560-programmer.md` with the three candidate
 *  owners named. Degradation is logged ONCE, at Log, so a playtest report of "the map has no
 *  markers" lands on a line that already explains itself.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  3b. ⚠️ TASK-579 — THE FIRST OPEN IS NOW **LEGIBLE**, AND IT IS ⛔ STILL NOT FIXED
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔⛔ **READ THE DISTINCTION BEFORE READING THE CODE: TASK-579 MAKES THE EMPTY STATE
 *  EXPLAIN ITSELF. IT DOES ⛔ NOT POPULATE IT.** A first open still has no markers (the
 *  snapshot EXISTS from BeginPlay but lists no places until the first real capture — the
 *  corrected §3 story), and ⛔ nothing here calls `Capture()` or `EnsureSnapshot()`.
 *  What changed is that the player is TOLD, in the status line the map already owns, why the
 *  map has nothing to click and what single action fixes it. ⭐ **TASK-685 (`W691-3`)
 *  RE-POINTED BOTH DISCRIMINATORS from "snapshot null" to "NO PLACE RESOLVED"**
 *  (`GetPlaceNames().Num() == 0`; a null snapshot short-circuits into the same arm) —
 *  TASK-691 proved the pointer test aimed at a state that never occurs, so "War map" and
 *  the click-a-marked-place hint were rendering over a marker-less map (VID-003).
 *
 *  ⚖️ **AND THE GAP IS RECORDED HONESTLY RATHER THAN DRESSED UP: `WR-§9` row 12 is the ONE
 *  row on that list explicitly labelled a KNOWN GAP WE CHOSE NOT TO CLOSE — ⛔ NOT a designed
 *  outcome.** The real repair is **TASK-580**, it belongs in `USiegeAssistantComponent`, and
 *  it is deliberately held out of this batch: editing that file here would convert the
 *  batch's strongest proof — *"we did not touch the airlock"* — into a weaker one — *"we
 *  touched it and checked"* — in exchange for markers on one screen.
 *
 *  ⭐ **THE LINE IS A LATCH, NOT A POLL, AND IT MUST DISAPPEAR — a stale hint is its own
 *  defect.** `bShowingNoSnapshotHint` is true EXACTLY WHEN the hint is the line currently on
 *  screen; the map's existing refresh timer clears it the moment the snapshot lists at least
 *  one resolved place (the `W691-3` re-point — the retire condition matches the arming
 *  condition), and any other line (a picked symbol, the empty-click hint) clears it
 *  immediately. ⛔ No new timer, ⛔ no tick, ⛔ no per-frame log and ⛔ no `Warning` — a status
 *  line the player reads IS the whole mechanism.
 *
 *  ✅ **AND WHAT THE FIRST OPEN ALREADY DOES CORRECTLY, CONFIRMED AT THE CODE RATHER THAN
 *  ASSUMED (TASK-579 spec item 4):** the ALLY dots (`RefreshAllyDots` — world actors + the
 *  owning `ASiegePlayerState`), the ENEMY dots (`EnemyDotsWorldXY`, painted straight from the
 *  RPC payload) and the 30-GOLD REVEAL (`HandleRevealButtonClicked` broadcasts; TASK-563's
 *  authority path never reads a snapshot) are ⛔ NONE of them gated on the snapshot. **Only
 *  the markers are.**
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  4. ⭐ RENDERING — C++ SLATE, NOT UMG CHILD WIDGETS (`WR-§6`'s rendering row)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  The markers and the dots are drawn in `NativePaint` with `FSlateDrawElement`. ⚖️ That is
 *  a deliberate RISK REDUCTION rather than a style: it dodges the MCP widget-tree authoring
 *  limit, dodges the duplicate-and-reparent corruption class outright (a duplicated +
 *  reparented WidgetBlueprint has silently broken RUNTIME repaint on this project before and
 *  cost ~9 wasted fixes), and makes marker geometry a SINGLE SOURCE OF TRUTH shared by the
 *  painter and the hit test.
 *
 *  ⚠️ **DECLARED SPEC DEPARTURE, NAMED RATHER THAN SILENT (`SC-§15`): the task spec says
 *  `NativeOnPaint`; THE UE 5.8 VIRTUAL IS `NativePaint`.** Verified against the engine on
 *  this machine — `Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h:1592` declares
 *  `virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
 *  FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const`, and there is no
 *  `NativeOnPaint` symbol anywhere in that header (the `NativeOn…` prefix belongs to the
 *  INPUT events). Overriding the spelled name would have compiled as a NEW function that the
 *  engine never calls, and the map would have rendered nothing while every readback looked
 *  correct — so the departure is the fix, not a liberty.
 *
 *  ⭐ `WBP_WarMap` therefore supplies ONLY a background panel, the reveal button, the close
 *  button and a status line — all `BindWidgetOptional`, all genuinely optional. ⛔ IT IS
 *  BUILT FRESH (TASK-568), ⛔ NEVER duplicate-and-reparent; `WBP_SessionMenu` is the standing
 *  proof a fresh build is achievable, and this class clones its optional-child contract.
 *
 *  ✅ **AND THE MAP WORKS WITH NO BLUEPRINT AT ALL.** `SObjectWidget::OnPaint` routes
 *  `NativePaint` regardless of what the widget tree contains (SObjectWidget.cpp:146), so a
 *  `CreateAndAddToViewport` fallback onto this C++ class alone still paints every marker and
 *  every dot, and still hit-tests them. The BP adds chrome; it is not load-bearing.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  5. ⛔ WHAT THIS CLASS REFUSES TO DO
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  - ⛔ **IT NEVER PAUSES.** `WR-§6` rules it, and it is M8-safe (a listen-server client
 *    cannot pause a host) and consistent with the shipped console. Opening the map is a real
 *    tactical cost, which is the point of it living in the castle (`WR-§9` outcome 3).
 *  - ⛔ **IT NEVER REQUESTS, PRICES, VALIDATES OR SPENDS GOLD.** It holds the dot array it is
 *    handed and CLEARS it on close, always. TASK-563 owns `EnemyRevealCost`, the authority
 *    check, `SpendGold`, the net-zero refusal and both RPCs (`WR-§7`).
 *  - ⛔ **IT NEVER GATES THE CONSOLE.** No proximity check, no NPC reference and no range
 *    condition exists in this file. *"The console still works anywhere"* is a RULING
 *    (`WR-§5`, `WR-§9` outcome 7); the proximity gate is the MAP's alone and lives in
 *    TASK-563.
 *  - ⛔ **IT NEVER RE-DERIVES A PLACE POSITION.** `ResolvePlace` is the single owner. There
 *    is no `TActorIterator` for places anywhere here; the only actor iteration in this file
 *    is the ALLY DOT sweep, which asks a different question (where are my units) that no
 *    shipped finder answers.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  6. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  ⛔ This class adds **no replicated property, no new replicated class, no new relevancy
 *  tier and no RPC.** A `UUserWidget` is CLIENT-LOCAL by construction — nothing here crosses
 *  the wire. ✅ Ally dots need nothing new: own-team actors are already relevant to their own
 *  client. ⚠️ The TWO RPCs `WR-§8` declares for this batch (`ServerRequestEnemyReveal` /
 *  `ClientReceiveEnemyReveal`) belong to TASK-563's `ASiegePlayerController`, because gold is
 *  authority-owned. *"There is nothing to declare"* only counts when it is stated.
 *
 *  ⚠️ AND THE HONEST LIMITATION, RECORDED RATHER THAN HIDDEN: on a listen server the client
 *  ALREADY holds the enemy actors under Tier-B relevancy, so the paid reveal is an
 *  ECONOMY/UI gate, ⛔ NOT an anti-cheat boundary and ⛔ not concealment. This widget simply
 *  declines to draw what it was not handed.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  7. ⚠️ FLAGGED AND UNRULED — ⛔ NOT DECIDED HERE
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  - **D6 frozen vs live enemy dots.** This class HOLDS what it is given and clears on
 *    close, which IS the frozen default (`WR-§7`) — and it is agnostic: if D6 flips to live,
 *    TASK-563 simply re-pushes on an interval and not one line here changes.
 *  - **D7 map background art.** ⛔ NOT THIS TASK. Nothing here draws a backdrop; the WBP's
 *    optional background panel is the art seam.
 *  - **D8 whether the `SM_POI_01..04` props become nameable places.** ⭐⛔ **CLOSED
 *    2026-09-01 by `MARK-§0`, AND CLOSED *AGAINST* THE SHAPE THIS BULLET DESCRIBES.** His
 *    answer arrived sideways and is better: the nameable places are ⛔ NOT the decorative
 *    landmarks — they are **player-defined, drawn at runtime, and numbered by the player
 *    himself** (§8 below). ⇒ ⛔ **`SM_POI_01..04` STAY SCENERY with no symbol and no
 *    `ResolvePlace` entry — refused on the record, not merely unbuilt** — and this file still
 *    contains no place literal of any kind.
 *  - **D9 whether the map pauses.** `WR-§6` rules NO PAUSE and this class obeys that ruling;
 *    it is recorded here as ruled-by-law rather than decided-by-programmer.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *  8. ⭐⭐ MAP MARKS — THE PLAYER'S NUMBERED CIRCLES (TASK-745; `MARK-§0..§6`)
 *  ───────────────────────────────────────────────────────────────────────────────────────
 *
 *  Jonathan's directive, verbatim, because it IS the spec: *"click on anywhere on the map, to
 *  create a circle on the map at that location. you can hover the mouse over it and use the
 *  mouse wheel scroll to make that circle larger or smaller. You can right click to delete
 *  that circle. Every time you make a new circle, it gets its own number in the middle of
 *  it… I can make 3 different circles and then tell the commander something like 'move all
 *  units to hold 1' or 'move all units to ambush 2'."*
 *
 *  ⭐⭐ **THIS IS ⛔ NOT A UI FEATURE. IT IS A NEW REFERENT CLASS IN THE AI COMMAND GRAMMAR**
 *  (`MARK-§0`) — and this widget is the half of it the player touches. The other half
 *  (publishing the symbol into the per-match place list and answering for it in
 *  `ResolvePlace`) is TASK-746's, in `SiegeAssistantSnapshot.{h,cpp}`, which ⛔ this file does
 *  not touch. The STORE is TASK-744's `USiegeMapMarkSubsystem`, which ⛔ this file does not
 *  define — it consumes the pinned registry and nothing else.
 *
 *  ── (a) ⛔⛔ THE AIRLOCK IS THE FIRST THING, NOT THE LAST, AND IT IS **UNCHANGED** ────────
 *
 *  ⛔ **NO COORDINATE, RADIUS, DOT, COUNT OR MARKER GEOMETRY ENTERS ANY PROMPT ZONE.** ✅ The
 *  map writes ONLY the symbol string — `circle_1`, from `FSiegeMapMark::MakeSymbol` — onto the
 *  SAME `OnPlacePicked` delegate the seven place markers already use, and the binder hands it
 *  to the console's input box where **THE PLAYER STILL PRESSES ENTER HIMSELF**.
 *
 *  ⚠️⚠️ **THE ONE THING THAT IS GENUINELY NEW, STATED LOUDLY BECAUSE A REVIEWER WILL AND
 *  SHOULD STOP ON IT: THIS FILE NOW COMPUTES A WORLD COORDINATE.** A placement click is run
 *  back through `LocalToMapUV` → `MapUVToWorld` and stored as `FSiegeMapMark::WorldXY`.
 *  ⇒ **Three properties make that safe, and all three are structural rather than disciplinary:**
 *    1. ⛔ **It goes into a CLIENT-LOCAL `ULocalPlayerSubsystem` and nowhere else.** Nothing in
 *       this file prints it, serializes it, replicates it or hands it to a delegate. The map's
 *       ENTIRE outbound surface is still `FOnWarMapPlacePicked(FName)`.
 *    2. ✅ **`MARK-§1` sanctions exactly this shape by name:** *"a mark's centre is resolved on
 *       the GAME side by `ResolvePlace`, exactly as `nearest_mine` resolves to a world position
 *       today without ever printing one."* The model sees a SYMBOL; the executor resolves it.
 *    3. ⛔⛔ **IT DOES NOT COME FROM THE ELEVATION BAKE — `WM-§8d`'s FIREWALL, HONOURED
 *       EXACTLY.** It comes from `FSiegeWarMapProjection`'s exact X/Y inverse, and a
 *       `FVector2D` **has no Z at all**, so the 1,000-uu-clamped display buffer this class also
 *       owns is STRUCTURALLY unreachable from a mark. ⇒ the fake-instrument class `SHIP-§9`
 *       exists for cannot arise here, and it cannot arise later either.
 *
 *  ── (b) ⛔ THE NUMBER IS THE PLAYER'S; THE SYMBOL IS THE MODEL'S (`MARK-§2`) ──────────────
 *
 *  The map draws **`1`** in the circle's centre and inserts **`circle_1`** into the input box.
 *  ⛔ **NEITHER MAY EVER BE SHOWN IN THE OTHER'S PLACE.** The symbol is not a bare digit
 *  because Zone A already ships `COUNT = 1 to 30`, so a bare `1` in a `where` field is a token
 *  the model has been taught means a QUANTITY — and this project's entire measured failure
 *  history is *valid-shaped-wrong-command*. `MakeMarkPickSymbol` is the only place this file
 *  turns a number into a symbol, and it delegates to the ONE seam TASK-744 owns.
 *
 *  ── (c) ⭐ THE FOUR VERBS, AND THE PRECEDENCE BETWEEN THEM ────────────────────────────────
 *
 *    LEFT click  on a place MARKER  ⇒ insert that marker's symbol   (⛔ SHIPPED, UNCHANGED)
 *    LEFT click  on a MARK          ⇒ insert `circle_N`             (⭐ the point of the feature)
 *    LEFT click  on empty map       ⇒ PLACE a mark at the lowest free number
 *    RIGHT click on a MARK          ⇒ DELETE it (the hole stays — `M-1`)
 *    RIGHT click on empty map       ⇒ ⛔ NOTHING (absorbed, as every button already is)
 *    WHEEL       over a MARK        ⇒ resize it
 *    WHEEL       anywhere else      ⇒ ⛔ NOTHING (absorbed — see (e))
 *
 *  ⛔⛔ **MARKERS OUTRANK MARKS, IN BOTH THE HIT TEST AND THE PAINT ORDER, AND THE TWO ORDERS
 *  ARE THE SAME ORDER ON PURPOSE.** TASK-745's spec lists *"the seven place markers' hit-testing
 *  and symbol insertion"* as a shipped thing it may not disturb — and a mark is an arbitrarily
 *  large disc the player can drop anywhere, so if marks won, drawing one big circle over
 *  `own_castle` would silently make that marker unclickable forever. ⇒ markers are tested
 *  first and drawn last (topmost), exactly as before.
 *  ⚠️ **THE CONSEQUENCE, DECLARED RATHER THAN DISCOVERED:** you cannot place a mark INSIDE an
 *  existing mark — that click names the outer one instead. Delete it, or place elsewhere. It
 *  is the same designed outcome as last-match-wins on overlap, and it goes on the playtest
 *  sheet rather than being coded around.
 *
 *  ── (d) ⭐ THE VISUAL IDENTITY — ⛔ DELIBERATELY UNLIKE THE ORDER ZONES ────────────────────
 *
 *  ⚠️⚠️ **THE GAME NOW HAS *TWO* DIFFERENT THINGS CALLED "CIRCLES", BOTH RESIZED BY THE MOUSE
 *  WHEEL** (`MARK-§4`): the shipped **group-order pick zones** — transient, filled, world-space
 *  ground DECALS that ISSUE AN ORDER the moment you confirm — and these **map marks** —
 *  persistent, hollow, map-space RINGS that issue nothing and merely NAME ground.
 *
 *  ⇒ **THREE INDEPENDENT TELLS, so no single one has to carry it:**
 *    1. **Hollow vs filled.** A mark is a stroked RING; an order zone is a filled decal.
 *    2. **A NUMBER IN THE MIDDLE.** No order zone has ever carried a number. It is the tell
 *       Jonathan himself named, and it is the one that is unmistakable at a glance.
 *    3. **MAGENTA, the exact complement of the map's green** — a hue that appears nowhere else
 *       in this game's tactical palette (the team palette is blue/red, the field is green, the
 *       mine and marker glyphs are gold). ⇒ nothing on this map or in the world reads as
 *       "magenta ring" except a mark.
 *  ⛔ **TASK-751 owns reconciling the two for the player in the controls help;** this section
 *  exists so that when it does, the marks are already visually unambiguous.
 *
 *  ── (e) ⛔ THE WHEEL LAW (`MARK-§4`) — AN AMENDMENT, AND ONE THIS FILE MUST NOT EXCEED ─────
 *
 *  ✅ **THE CONTROLLER'S POLL AND THIS WIDGET'S EVENT ARE STRUCTURALLY DIFFERENT MECHANISMS IN
 *  DIFFERENT LANES, VERIFIED AT THE SOURCE RATHER THAN ASSUMED:** the controller's is a **POLL** —
 *  `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)` inside `ApplyGroupPickWheel`, whose ONLY
 *  call site is `ASiegePlayerController::PlayerTick`'s `GroupPickStage != EGroupPickStage::None`
 *  branch (`SiegePlayerController.cpp:647-657`, `:2843-2876`). This one is a **SLATE EVENT** on
 *  a focused widget. ⇒ ⛔ **NOT ONE LINE OF THE CONTROLLER IS TOUCHED, no symbol is shared, and
 *  its "inert outside the pick flow" property is LITERALLY unchanged.**
 *
 *  ⭐ TASK-1592 (2026-09-29), comment only: this paragraph used to open "THE TWO CONSUMERS ARE
 *  STRUCTURALLY DIFFERENT MECHANISMS", a count that went stale when `STACK-§4` amended
 *  `MARK-§4` by name, to three. `MARK-§4` now reads "the wheel has exactly THREE consumers —
 *  (1) the controller's group-pick poll · (2) `UWarMapWidget` while the map is open and the
 *  cursor is over it · (3) the controller's PLACEMENT-mode footprint poll
 *  (`ApplyPlacementFootprintWheel`, `STACK-§4`)". Consumers 1 and 3 are both controller polls,
 *  in sibling branches of `PlayerTick` that never run in the same frame, so the contrast drawn
 *  here holds for both of them against this widget's Slate event, and "inert outside the pick
 *  flow" is true of consumer 1's poll only.
 *
 *  ⚠️ **AND THE ABSORB IS A DECISION, DECLARED:** while the map is OPEN this widget returns
 *  `Handled` for EVERY wheel event, including ones over empty map. ⚖️ **That is the same
 *  fail-safe direction — and the same argument — as the shipped `NativeOnMouseButtonDown`,
 *  which already absorbs every mouse BUTTON while the map is up: the map fills the screen, so
 *  letting input fall through would drive something the player cannot see. `MARK-§4`'s own
 *  wording contemplates it (*"`UWarMapWidget` while the map is open and the cursor is over
 *  it"*), and it adds ⛔ NO third consumer.** ⛔ **INERT still means INERT: a wheel event that
 *  is not over a mark changes NO state anywhere** — it is swallowed, not acted on. And when
 *  the map is CLOSED this widget is `Collapsed`, so the event never reaches it at all and the
 *  controller's path is byte-for-byte what it has always been.
 *
 *  ── (f) 📌 M8 (`MARK-§6`) ─────────────────────────────────────────────────────────────────
 *
 *  ⛔ **NO replicated property, ⛔ NO RPC, ⛔ NO class-tier change.** Marks are client-local
 *  display plus client-local symbol resolution, and `ULocalPlayerSubsystem` makes `M-2`
 *  (per-player) STRUCTURAL rather than disciplinary. ⚠️ `M-3` (⛔ not enemy-visible) is what
 *  keeps that free, and it is **deliberately the OPPOSITE of `GHOST-§ G-4`** — ⛔ do not "make
 *  them consistent"; both are Jonathan's own words on their own features.
 *
 *  ⛔ **MARKS PERSIST ACROSS MAP CLOSE/REOPEN** (`M-4`) — so `CloseMap` clears the enemy
 *  reveal, the ally dots and the POI census, and ⛔ deliberately NOT the marks. ⛔ Do NOT copy
 *  `WR-§7`'s clear-on-close rule onto them: that rule governs PURCHASED enemy intel and its
 *  whole point is that the snapshot goes stale. A mark is the player's own note about his own
 *  ground and does not decay. Clearing on match reset is the STORE's job (`ClearMarks`).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UWarMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  ⚠️ THE SOFT DEFAULTS ARE SET **HERE**, ⛔ NOT IN `NativeConstruct`, AND THE DIFFERENCE
	 *  IS THE WHOLE POINT OF AN `EditDefaultsOnly` PROPERTY. `NativeConstruct` runs on every
	 *  widget instance AFTER the class defaults are applied, so assigning `ArenaConfigAsset`
	 *  there would silently STOMP whatever a designer set on `WBP_WarMap` — the property
	 *  would appear editable and be inert. The shipped soft-ref idiom
	 *  (`ACastle`, `AAncientGround`, `UCombatantHealthBarComponent`) sets these in the
	 *  constructor for exactly this reason.
	 */
	UWarMapWidget(const FObjectInitializer& ObjectInitializer);

	//~ ---------------------------------------------------------------------
	//~ Outbound seams. TASK-563's controller binds these.
	//~ ---------------------------------------------------------------------

	/** ⭐ THE MAP'S ONLY CHANNEL TO THE AI, AND IT CARRIES A SYMBOL. See the delegate's own comment. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|WarMap")
	FOnWarMapPlacePicked OnPlacePicked;

	/** Open/close transitions, for the posture owner. Mirrors `OnConsoleOpenChanged`. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|WarMap")
	FOnWarMapOpenChanged OnMapOpenChanged;

	/** The player clicked the reveal button. ⛔ A click, not a request — this widget prices nothing (`WR-§7`). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|WarMap")
	FOnWarMapRevealButtonClicked OnRevealButtonClicked;

	//~ ---------------------------------------------------------------------
	//~ Open / close. ⛔ Never touches the input mode, the cursor or pause —
	//~ the posture owner is the controller (TASK-563), exactly as it is for
	//~ the console.
	//~ ---------------------------------------------------------------------

	/**
	 *  Shows the map full-screen and makes it hit-testable.
	 *
	 *  ⚠️ `Visible`, NOT the console's `SelfHitTestInvisible`, AND THE ONE-WORD DIFFERENCE IS
	 *  LOAD-BEARING. `SelfHitTestInvisible` means "my children can be clicked, I cannot" —
	 *  which is right for the console (it never hit-tests itself) and would silently delete
	 *  this entire feature, because `NativeOnMouseButtonDown` would never fire and every
	 *  marker would be inert while looking perfectly painted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|WarMap")
	void OpenMap();

	/**
	 *  Hides the map and ⛔ **DISCARDS THE ENEMY REVEAL, ALWAYS** (`WR-§7`, `WR-§9`
	 *  outcome 4): re-opening shows no red dots until the player pays again, even one second
	 *  after paying. That is the mechanic, not a bug.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|WarMap")
	void CloseMap();

	/** Open when closed, close when open. The natural binding for the `IA_WarMap` toggle (TASK-563). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|WarMap")
	void ToggleMap();

	UFUNCTION(BlueprintPure, Category = "Siegebound|WarMap")
	bool IsMapOpen() const { return bMapOpen; }

	//~ ---------------------------------------------------------------------
	//~ The enemy reveal. ⛔ THIS WIDGET IS THE SINK, NEVER THE SOURCE.
	//~ ---------------------------------------------------------------------

	/**
	 *  Accepts a paid reveal's dot list from TASK-563's `ClientReceiveEnemyReveal`.
	 *
	 *  ⛔⛔ **THE PARAMETER IS WORLD-SPACE `(X, Y)` IN UU — NOT MAP UV, NOT SCREEN PIXELS.**
	 *  Stated this hard because the RPC's `TArray<FVector2D>` cannot tell the two apart and a
	 *  silent mismatch would put every red dot in a plausible wrong place. ⚖️ World-space is
	 *  the right side of that seam: the projection then has exactly ONE owner
	 *  (`FSiegeWarMapProjection`), shared byte-for-byte with the ally dots, so the two dot
	 *  colours can never disagree about where the arena is.
	 *
	 *  ⛔ Accepting the array is NOT the same act as requesting it. Nothing here checks a
	 *  cost, a balance or an authority; a caller that hands this widget dots the player never
	 *  paid for has a defect in TASK-563, and this class is deliberately not the place that
	 *  would catch it (a second economy check is a second economy rule to get wrong).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|WarMap")
	void ReceiveEnemyReveal(const TArray<FVector2D>& EnemyWorldXY);

	/** Drops the reveal snapshot. Idempotent. Called by `CloseMap` unconditionally, and reachable for a match reset. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|WarMap")
	void ClearEnemyReveal();

	/** Red dots currently held. 0 ⇒ no reveal is active. Diagnostics and TASK-564; ⛔ never printed into a prompt. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|WarMap")
	int32 GetEnemyRevealDotCount() const { return EnemyDotsWorldXY.Num(); }

	/** Blue dots at the last refresh. Diagnostics and TASK-564; ⛔ never printed into a prompt. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|WarMap")
	int32 GetAllyDotCount() const { return AllyDotsWorldXY.Num(); }

	/**
	 *  Marks the local player currently holds (TASK-745). 0 ⇒ none placed, or ⛔ no store is
	 *  reachable at all (no owning local player — the headless case).
	 *
	 *  ⛔ A READ-THROUGH TO `USiegeMapMarkSubsystem::GetMarks().Num()`, ⛔ NOT a second count:
	 *  this widget owns ⛔ no mark state whatsoever, and a cached count here would be a second
	 *  truth to drift. Diagnostics and tests only; ⛔ never printed into a prompt (`MARK-§5`'s
	 *  airlock row names the COUNT explicitly among the things that may not travel).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|WarMap")
	int32 GetMapMarkCount() const;

	//~ ---------------------------------------------------------------------
	//~ BlueprintImplementableEvents — FString/int32/bool params ONLY (the
	//~ widget-param law: MCP cannot author enum or struct BP params).
	//~ ---------------------------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|WarMap")
	void OnWarMapOpenStateChanged(bool bOpen);

	/** One line of STATIC CHROME. ⛔ It never names an order, a unit count or an outcome, and it never reaches a prompt. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|WarMap")
	void OnWarMapStatusLine(const FString& Line);

	/** Lets the WBP grey or label the reveal button. ⚠️ It is told WHAT IS DRAWN, never what anything cost. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|WarMap")
	void OnWarMapRevealStateChanged(bool bRevealActive, int32 DotCount);

	//~ ---------------------------------------------------------------------
	//~ The shared geometry. ⭐ ONE builder, two callers.
	//~ ---------------------------------------------------------------------

	/**
	 *  Builds the hit/paint rect for every place symbol that RESOLVES this match, in fixed
	 *  vocabulary order, from the read-only snapshot.
	 *
	 *  ⛔ THE ONLY PRODUCER OF MARKER GEOMETRY IN THIS CLASS. `NativePaint` calls it;
	 *  `NativeOnMouseButtonDown` calls it with the same geometry. ⚠️ Deliberately NOT cached:
	 *  a cache would be one more lifetime to get wrong (the snapshot is re-`Capture()`d out
	 *  from under it every sentence, the hero moves every frame, and the mine is "the best
	 *  mine NOW"), for seven array lookups a frame.
	 *
	 *  Empty output — never a partial one — for: no owning controller, no assistant
	 *  component, no snapshot (defensive — §3's corrected story: the common empty case is a
	 *  present-but-never-captured snapshot, whose `GetPlaceNames()` loop simply runs zero
	 *  times), or a degenerate panel.
	 */
	void BuildMarkerRects(const FGeometry& AllottedGeometry, TArray<FSiegeWarMapMarker>& OutMarkers) const;

	/**
	 *  ⭐ THE MARK EQUIVALENT, AND THE SAME LAW: **ONE PRODUCER, FOUR CONSUMERS** (TASK-745).
	 *  `NativePaint`, `NativeOnMouseButtonDown` (both buttons), `NativeOnMouseWheel` and
	 *  `NativeOnMouseMove` all call it with the geometry they were handed, so the ring you see,
	 *  the ring you name, the ring you delete and the ring you resize are the same ring.
	 *
	 *  Projects the store's world-space marks through the SHIPPED `WorldToMapUV` →
	 *  `MapUVToLocal` chain (⛔ the transform is never re-derived) and their world radii through
	 *  `MapWorldRadiusToLocalPx`, in STORE ORDER — which is what makes `FindMarkIndexAtLocal`'s
	 *  last-match-wins rule agree with the paint order.
	 *
	 *  ⚠️ Deliberately NOT cached, the `BuildMarkerRects` argument verbatim: a cache would be one
	 *  more lifetime to get wrong (the panel resizes, the arena config can reload, the store
	 *  mutates under three different verbs) for at most nine array lookups a frame.
	 *
	 *  ⛔ Empty output — never a partial one — for: no owning local player, no mark subsystem,
	 *  or a degenerate panel. ⭐ It is `const` and it MUTATES NOTHING, which is what lets the
	 *  `const` painter share it: reading the store is a read.
	 */
	void BuildMarkCircles(const FGeometry& AllottedGeometry, TArray<FSiegeWarMapMarkCircle>& OutCircles) const;

	//~ ---------------------------------------------------------------------
	//~ ═══ THE ELEVATION BACKGROUND (TASK-684; CONVENTIONS `WM-§2`) ═══
	//~ A RUNTIME one-time trace bake per match — ⛔ never an offline asset,
	//~ because the hills are a scatter layer re-rolled per match seed, so a
	//~ baked picture of last match's hills is a lie about this match's.
	//~ Lazy on the first map open; cached until the field re-rolls; ZERO
	//~ per-frame cost after the bake (`NativePaint` draws one cached brush).
	//~ 📌 M8: client-local display — no replicated property, no new class,
	//~ no RPC. Nothing from this layer enters any prompt zone (`WR-§6`).
	//~ ---------------------------------------------------------------------

	/**
	 *  ⭐ THE §-PINNED PURE SEAM (`WM-§2` — the `W4-R1` lesson: a testability obligation gets
	 *  a testability seam). Maps a traced surface height to a brightness in `[0,1]`:
	 *
	 *      0 at `GroundZ` (the field floor ⇒ dark) … 1 at `GroundZ + ReliefCeiling` (light),
	 *      linear between, and ⛔ CLAMPED to exactly 1 ABOVE the ceiling — full white.
	 *
	 *  ⚠️ THE CLAMP IS THE NORMALIZATION LAW, NOT A SAFETY NET: castle shells (≈8,000 uu) and
	 *  boundary walls tower over the tallest hill crown (≈1,000 uu at max scatter scale); a
	 *  min→max normalization over RAW heights would crush every hill into one gray band. They
	 *  clamp white instead — they read as "walls", declared a designed outcome (`WM-§6`).
	 *
	 *  Below-ground hits clamp to 0; a non-positive `ReliefCeiling` is floored at 1 uu (the
	 *  `MinArenaHalfExtentUu` zero-divide doctrine) so the function is total — no NaN, ever.
	 *
	 *  ⛔ `public`, plain C++ static, ⛔ NOT a `UFUNCTION`, exactly three parameters, none
	 *  defaulted (`SC-§33`). Tested headlessly in `Tests/SiegeWarMapTest.cpp`.
	 */
	static float HeightToBrightness(float HitZ, float GroundZ, float ReliefCeiling);

	/**
	 *  ⭐ THE RAMP'S **COLOUR** SEAM (TASK-722; `WM-§8a`) — the COMPANION to
	 *  `HeightToBrightness` above, and ⛔ EMPHATICALLY NOT A CHANGE TO IT. `WM-§2` split the
	 *  ramp's SHAPE (that function) from its SCREEN MAPPING (this one) on purpose; `WM-§8a`
	 *  re-states the split, because turning the map from GRAY to GREEN had to happen without
	 *  the shape moving a single byte — and it did.
	 *
	 *  Maps a `[0,1]` brightness — exactly what `HeightToBrightness` returns — to the texel
	 *  written into the elevation texture: a per-channel lerp between the two MEASURED grass
	 *  colours (`WM-§8b`), dark green at 0, light green at 1. Out-of-range input clamps, so
	 *  the function is total.
	 *
	 *  ⚠️⚠️ IT RETURNS AN `FColor` OF **sRGB-ENCODED TEXTURE BYTES**, ⛔ not linear light —
	 *  the bake's texture is created with `SRGB = true`. The whole colour-space argument (and
	 *  the warning that this file holds `FLinearColor` constants in TWO different spaces)
	 *  lives at the constants' declaration in `WarMapWidget.cpp`. The one line that matters
	 *  at this signature: ⛔ `FLinearColor::ToFColor(true)` must NEVER be used to build this
	 *  value — it would sRGB-encode floats that are already encoded.
	 *
	 *  ⛔ DISPLAY ONLY (`WM-§8d`). Like the whole elevation layer, nothing here may ever be
	 *  read by gameplay: the bake feeding it clamps at `GroundZ + ElevationReliefCeiling`
	 *  (1,000 uu), so it answers correctly in the ordinary case and lies exactly where a
	 *  tower or a tall hill lives — `SHIP-§9`'s class. `HIGH-§` reads `GetActorLocation().Z`
	 *  from live actors and nothing else.
	 *
	 *  ⛔ `public`, plain C++ static, ⛔ NOT a `UFUNCTION` (a texel is not a Blueprint API),
	 *  exactly ONE parameter, ⛔ none defaulted (`SC-§33`). It is a seam for the same reason
	 *  `HeightToBrightness` is one — `W4-R1`: a testability obligation gets a testability
	 *  seam — and it is pinned headlessly in `Tests/SiegeWarMapTest.cpp`.
	 */
	static FColor BrightnessToRampColor(float Brightness);

	/**
	 *  ⭐⭐ THE MARK'S OUTBOUND SYMBOL, AND THE **ONLY** PLACE THIS FILE TURNS A NUMBER INTO A
	 *  NAME (TASK-745; `MARK-§2`, `MARK-§5`).
	 *
	 *  `MakeMarkPickSymbol(1)` ⇒ `circle_1` — obtained by DELEGATING to
	 *  `FSiegeMapMark::MakeSymbol`, the ONE seam the widget and the snapshot must agree on.
	 *  ⛔ **THIS FUNCTION MUST NEVER SPELL THE SYMBOL ITSELF.** A second transcription is a
	 *  second thing to drift, and the drift would be silent in exactly the worst way: the map
	 *  would insert a name TASK-746 never published, the grammar would refuse it, and the
	 *  feature would read as "the AI ignores my circles."
	 *
	 *  ⚖️ **AND IT EXISTS AS A NAMED FUNCTION FOR ONE REASON — `W4-R1` AGAIN.** TASK-745's spec
	 *  requires a test asserting *"the symbol composed into the input box for a mark is exactly
	 *  `FSiegeMapMark::MakeSymbol(N)`"*. Inlined into `NativeOnMouseButtonDown`, that claim
	 *  would sit behind a realized Slate widget, a live local player and a populated subsystem —
	 *  i.e. unassertable, which is the unfunded-mandate defect this law was written to stop
	 *  happening a third time. As a pure static it is assertable by STRING EQUALITY with ⛔ no
	 *  world, ⛔ no widget instance and ⛔ no store.
	 *
	 *  ⛔ Returns an `FName` because that is `FOnWarMapPlacePicked`'s parameter type — the map's
	 *  entire outbound surface, unchanged by this feature. `NAME_None` for a non-positive
	 *  number, so a corrupt store can NEVER broadcast an empty symbol into the input box (the
	 *  binder already refuses `IsNone()`, and this makes the refusal reachable from both ends).
	 *
	 *  ⛔ `public`, plain C++ static, ⛔ NOT a `UFUNCTION`, exactly ONE parameter, ⛔ none
	 *  defaulted (`SC-§33`).
	 */
	static FName MakeMarkPickSymbol(int32 Number);

	/**
	 *  ⭐⭐ THE MARK RING'S TINT, READ-ONLY — AND THE ACCESSOR EXISTS SO THE LEGIBILITY CLAIM CAN
	 *  BE **MEASURED BY A TEST** RATHER THAN ASSERTED IN A COMMENT (`SC-§32`: a guardrail nobody
	 *  has watched trip is a guardrail nobody has tested).
	 *
	 *  ⛔⛔ IT RETURNS A **TRUE-LINEAR** SLATE TINT. ⛔ NOT the space the elevation ramp's
	 *  constants are in — that family is DISPLAY-ENCODED sRGB in the same `FLinearColor`
	 *  container, because it becomes the raw bytes of an `SRGB = true` texture and never passes
	 *  through Slate. ⛔ **NEVER copy a value between the two families and ⛔ never "correct" one
	 *  into the other** (`WM-§8e`): both readings compile, both render, and only one is right at
	 *  each site. The full argument lives at the constant's declaration in `WarMapWidget.cpp`.
	 *
	 *  ⭐ **AND THAT IS EXACTLY WHY THIS ACCESSOR EARNS ITS PUBLIC SURFACE.** `WM-§8e`'s named
	 *  hazard is *"a plausible-looking value in the right-looking type that is wrong in a way
	 *  review cannot see."* `Siegebound.WarMap.MarkRingColorIsLegibleAtBothRampEnds` computes
	 *  this colour's relative luminance against BOTH ends of the ramp — read through the shipped
	 *  `BrightnessToRampColor` seam, ⛔ never transcribed — and requires ≥3:1 at each. ⇒ a future
	 *  edit that sRGB-encoded this constant would raise its luminance from 0.175 to ~0.60 and
	 *  the light-end ratio would collapse to ~1.3:1, **turning the invisible colour-space error
	 *  into a red test.** ⛔ Do not weaken that assertion into a range check.
	 *
	 *  ⛔ `public`, plain C++ static, ⛔ NOT a `UFUNCTION` (a chrome tint is not a Blueprint API),
	 *  ⛔ ZERO parameters — so `SC-§33` cannot fire at all, structurally. The shape is the
	 *  `W4-R5` `GetDefaultBlueBarColor()` precedent, cloned.
	 */
	static FLinearColor GetMarkRingColor();

	//~ ---------------------------------------------------------------------
	//~ Construction. A plain static, not a UFUNCTION — the
	//~ `USiegeAssistantConsoleWidget::CreateAndAddToViewport` contract, cloned.
	//~ ---------------------------------------------------------------------

	/**
	 *  Creates the map for `OwningController` and adds it to the viewport, CLOSED.
	 *
	 *  ⚠️ **`WBP_WarMap` DOES NOT EXIST YET (TASK-568 builds it FRESH after the compile), AND
	 *  THIS PATH IS WHY THAT IS NOT A BLOCKER.** A null `MapClass` falls back to this C++
	 *  class, which paints and hit-tests on its own (see §4). The BP is passed in later with
	 *  ZERO change to this file.
	 *
	 *  @param OwningController  the local player controller. Null ⇒ no map, logged, never fatal.
	 *  @param MapClass          `/Game/UI/WBP_WarMap` once it exists; null ⇒ this class.
	 *  @param ZOrder            viewport Z order.
	 */
	static UWarMapWidget* CreateAndAddToViewport(
		APlayerController* OwningController,
		TSubclassOf<UWarMapWidget> MapClass = nullptr,
		int32 ZOrder = 0);

	//~ ---------------------------------------------------------------------
	//~ OPTIONAL children — the `USessionMenuWidget` contract, cloned verbatim.
	//~ ⛔ ALL `BindWidgetOptional`: a `WBP_WarMap` that names none of these
	//~ still compiles and still works. TASK-568 gets these exact names.
	//~ ---------------------------------------------------------------------

	/** OPTIONAL: the paid-reveal button. When bound, `OnClicked` auto-wires to broadcast `OnRevealButtonClicked`. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|WarMap", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RevealButton;

	/** OPTIONAL: the close button. When bound, `OnClicked` auto-wires to `CloseMap`. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|WarMap", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** OPTIONAL: the status line — kept updated by C++ when bound. The BIE fires either way. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|WarMap", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusTextBlock;

protected:

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 *  ⚠️ `NativePaint`, ⛔ NOT `NativeOnPaint` — the spec's spelling does not exist on
	 *  `UUserWidget` in UE 5.8. The full argument is in §4 of the class comment; it is
	 *  repeated at the declaration because this is the one signature a reviewer will check
	 *  against the task text and find "wrong".
	 */
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	/**
	 *  ⭐ HIT-TESTS AGAINST THE SAME RECTS THE PAINTER USED, from the same `BuildMarkerRects`
	 *  call shape. A hit inserts the LITERAL SYMBOL; a miss shows the hint and does nothing
	 *  else (`WR-§9` outcome 1).
	 *
	 *  ⚠️ EVERY BUTTON IS ABSORBED WHILE THE MAP IS OPEN, but only the LEFT one picks. The
	 *  map fills the screen, so letting a click fall through would issue a game order at the
	 *  world position behind the map — a defect the player would read as "the map made my
	 *  army walk somewhere". ⛔ Absorbing is the fail-safe direction.
	 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/**
	 *  ⭐ THE MARK RESIZE (TASK-745; `MARK-§4`) — **A SLATE EVENT ON A FOCUSED WIDGET, ⛔ NOT
	 *  THE CONTROLLER'S POLLED WHEEL.** §8(e) of the class comment carries the full amendment
	 *  and the source lines that prove the two mechanisms are disjoint; ⛔ `PlayerTick`'s
	 *  group-pick branch is not touched by this file and must not be.
	 *
	 *  ⚠️ THE TARGET IS COMPUTED FROM **THIS EVENT'S OWN CURSOR POSITION**, ⛔ never from the
	 *  `HoveredMarkNumber` highlight state. That state is a DISPLAY affordance which is
	 *  updated by a different event on a different cadence — resizing off it would mean a
	 *  stale hover could silently resize a circle the cursor has already left, which is the
	 *  wrong-circle failure this whole feature must not have.
	 *
	 *  ⛔ Absorbs while the map is open even when nothing is under the cursor (the
	 *  `NativeOnMouseButtonDown` fail-safe doctrine); INERT means it changes no state, ⛔ not
	 *  that it lets the event fall through to the world behind a full-screen panel.
	 */
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/**
	 *  Tracks WHICH mark the wheel would resize, for the highlight only (TASK-745).
	 *
	 *  ⚖️ IT EXISTS BECAUSE "HOVER + WHEEL" IS OTHERWISE AN INVISIBLE AFFORDANCE. Jonathan's
	 *  sentence is *"you can hover the mouse over it and use the mouse wheel scroll"* — without
	 *  a highlight the player cannot tell WHICH circle he is hovering when two overlap, and the
	 *  first thing he would learn is that the wheel resizes an unpredictable one.
	 *
	 *  ⛔ PURELY COSMETIC AND DELIBERATELY NOT LOAD-BEARING: no verb reads it, and deleting the
	 *  whole mechanism would change nothing but the picture. ⛔ It also does NOT absorb the
	 *  event — a move is not a command, and swallowing moves would break tooltips and hover on
	 *  every optional child `WBP_WarMap` supplies.
	 */
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Retires the hover highlight when the cursor leaves the map. ⛔ A highlight that outlives the cursor is a lie about what the wheel will resize. */
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget interface

	//~ ---------------------------------------------------------------------
	//~ Tunables. ⚠️ EVERY PIXEL NUMBER BELOW IS A UI AFFORDANCE, NOT A WORLD
	//~ CLAIM — the exact distinction `WR-§5` records for `InteractRadius`: a
	//~ number the player feels directly with the mouse, that can never
	//~ mis-select a unit and can never reach a prompt. ⛔ None of them is a
	//~ world radius, and `AS-§21.4` is untouched by all of them.
	//~ FLAGGED for Jonathan's feel pass.
	//~ ---------------------------------------------------------------------

	/** Ally-dot refresh period in seconds (`WR-§6`). ⛔ A TIMER, never a tick — see the .cpp for why `NativeTick` is not trustworthy on a WBP-derived class. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float AllyDotRefreshInterval = 0.25f;

	/** HALF the clickable rect around a marker, in local px. Generous on purpose: a marker the player cannot reliably hit reads as a broken map. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "4.0", ClampMax = "128.0"))
	float MarkerHitHalfSizePx = 18.f;

	/** Inset from the widget edge to the drawable map rect, in local px, so an edge marker is not clipped by the WBP's panel border. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "0.0", ClampMax = "512.0"))
	float MapPaddingPx = 48.f;

	/**
	 *  HALF the drawn ALLY dot, in local px (the quad painter's half-size — a "radius" in
	 *  the square-dot sense every dot on this map already uses).
	 *
	 *  ⭐ TASK-685 (`WM-§3`): PROMOTED from the file-local `DotDrawHalfSizePx = 3` literal
	 *  and BUMPED 3 → 5 for contrast at map scale against TASK-684's dark elevation
	 *  background — a 6-px square was a live "I see nothing" candidate VID-003 could not
	 *  rule out. ⚠️ ENEMY dots deliberately keep the shipped literal: `WM-§3` scopes the
	 *  legibility bump to Jonathan's blue dots, and the `WR-§7` reveal lane is not this
	 *  wave's to restyle (flagged in the handoff — making them ride a tunable is one line).
	 *  Still a UI affordance in PIXELS, ⛔ never a world radius (`AS-§21.4` untouched).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "1.0", ClampMax = "32.0"))
	float AllyDotRadius = 5.f;

	/**
	 *  Elevation-bake sample columns (world X). 130 × 60 = 7,800 down-traces ONCE per match
	 *  (`WM-§2`'s pinned grid: ≈400-uu cells over the shipped 52,000 × 24,000 arena) — a
	 *  one-shot sub-frame cost class, ⛔ zero per-frame. The sampled rect comes from
	 *  `ResolveArenaHalfExtent()` (asset → CDO), ⛔ never a hand-typed size (`SC-§34`).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "2", ClampMax = "1024"))
	int32 ElevationGridX = 130;

	/** Elevation-bake sample rows (world Y). See `ElevationGridX`. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "2", ClampMax = "1024"))
	int32 ElevationGridY = 60;

	/**
	 *  Relief band height (uu) mapped to the full dark→light ramp; every hit above
	 *  `GroundZ + this` clamps to full white (`HeightToBrightness` — the `WM-§2`
	 *  normalization law that stops castles erasing hills).
	 *
	 *  ⚠️ THE DEFAULT IS **MEASURED, NOT GUESSED** (`WM-§2` demands the derivation):
	 *  tallest hill mesh = `SM_Hill_02`, authored height **400 uu** (CONVENTIONS "Climbable
	 *  terrain (M6.6)" mesh table: knoll 250 / hill 400 / ridge 350, base pivot z_min = 0)
	 *  × the shipped `DA_BattlefieldScatter` Hills-layer `ScaleRange` max **2.5**
	 *  (`handoffs/TASK-251.md`: 0.9–1.3 → 0.4–2.5, uniform scale by the scatter law)
	 *  = **1,000 uu** — the tallest possible hill crown above the floor. Cross-check: the
	 *  W1-PREP nav-Z cap law caps hill scale so the tallest crown stays under the ±1,200
	 *  NavMeshBoundsVolume Z; 1,000 < 1,200 ✓ consistent. ⇒ the tallest hill maps to ≈1.0
	 *  (light) and every hill face lands inside the ramp, exactly the `WM-§2` intent.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap", meta = (ClampMin = "1.0"))
	float ElevationReliefCeiling = 1000.f;

	/**
	 *  `DA_BattlefieldScatter` — the SINGLE OWNER of the arena extent (`WR-§6`, `SC-§34`).
	 *
	 *  ⚠️ THE ASSET, NOT THE HEADER DEFAULT, IS THE AUTHORITY: `WR-§2` row 4 records that the
	 *  saved DataAsset OVERRIDES the C++ default, so reading the CDO alone would silently
	 *  draw a different arena from the one the game generated. ⛔ Absent or unloadable ⇒ a
	 *  NAMED fallback (`ResolveArenaHalfExtent`) plus a single log — ⛔ never a hand-typed
	 *  size and ⛔ never a silent zero-divide.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap")
	TSoftObjectPtr<USiegeScatterConfig> ArenaConfigAsset;

	//~ ---------------------------------------------------------------------
	//~ ═══ MAP MARK TUNABLES (TASK-745; `MARK-§4`, `HIGH-§1`) ═══
	//~
	//~ ⛔⛔ EVERY NUMBER BELOW IS IN **WIDGET/MAP PIXELS**, AND THAT IS `MARK-§4`'s EXPLICIT
	//~ INSTRUCTION: *"REUSE THE SHIPPED TUNABLES' SHAPE, ⛔ NOT THEIR VALUES … ⛔ do NOT reuse
	//~ `GroupRadiusWheelStep`/`Min`/`Max` numerically — they are world-space and would be
	//~ meaningless."* The controller's 100 / 200 / 5000 uu are ⛔ NOT referenced, ⛔ not
	//~ converted and ⛔ not echoed anywhere in this file.
	//~
	//~ ⭐ AND EACH CARRIES ITS CONSEQUENCE BESIDE IT — `HIGH-§1`'s law: a tunable whose comment
	//~ says only what it is, and never what happens when it moves, is a dial nobody can turn.
	//~
	//~ ⚠️ Pixel tunables against a world-space store is a DELIBERATE seam, not an oversight —
	//~ `FSiegeWarMapProjection::MapWorldRadiusToLocalPx` carries the whole argument.
	//~ ---------------------------------------------------------------------

	/**
	 *  The radius a NEW circle is born at, in map px.
	 *
	 *  ⚠️ CONSEQUENCE: too small and the centred number stops fitting inside its own ring on
	 *  the very first click, which is the first thing Jonathan will see; too large and one
	 *  click covers a quarter of the field, hiding the markers under it (which stay clickable,
	 *  but stop being visible). 40 px is ~4× the 24-px POI glyph — unmistakably a region rather
	 *  than a point — and comfortably fits a two-digit-wide font box at the default size.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "8.0", ClampMax = "512.0"))
	float MarkDefaultRadiusPx = 40.f;

	/**
	 *  One wheel notch, in map px.
	 *
	 *  ⚠️ CONSEQUENCE: this is the FELT SPEED of the resize and it is the only thing that
	 *  decides it — the driver's raw delta magnitude is deliberately discarded
	 *  (`StepMarkRadiusPx` uses the SIGN only), so a free-spinning wheel and a trackpad behave
	 *  the same. Raise it and the circle snaps between sizes; lower it and crossing the full
	 *  12 → 240 px range takes 76 notches instead of 38.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "0.5", ClampMax = "64.0"))
	float MarkWheelStepPx = 6.f;

	/**
	 *  Smallest a circle can be scrolled to, in map px.
	 *
	 *  ⚠️ CONSEQUENCE: this is the floor on BOTH legibility and clickability at once. Below
	 *  ~12 px the smallest permitted number font (10 pt) no longer fits inside the ring, and
	 *  the disc becomes harder to hit with a mouse than the 18-px place markers beside it — at
	 *  which point right-click-to-delete starts feeling broken. ⛔ Never let it reach 0: a
	 *  zero-radius mark would be invisible AND un-deletable, i.e. a permanent ghost entry
	 *  holding a number the player cannot reclaim.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "4.0", ClampMax = "128.0"))
	float MarkMinRadiusPx = 12.f;

	/**
	 *  Largest a circle can be scrolled to, in map px.
	 *
	 *  ⚠️ CONSEQUENCE: the cap on how much of the battlefield one mark may claim. Above ~240 px
	 *  on a 1080p panel a single circle spans most of the drawn map, which makes every LATER
	 *  empty-map click land inside it — and an empty-map click that lands inside a mark NAMES
	 *  that mark instead of placing a new one (§8(c)). ⇒ raising this too far quietly costs the
	 *  player the ability to place more circles.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "16.0", ClampMax = "2048.0"))
	float MarkMaxRadiusPx = 240.f;

	/**
	 *  Ring stroke thickness in px.
	 *
	 *  ⚠️ CONSEQUENCE: legibility against a 14.74:1 background span. Thinner than ~2 px and the
	 *  ring becomes a hairline that disappears over the ramp's light end; thicker than ~5 px
	 *  and the stroke starts covering the ground it is supposed to be circling. ⭐ The ring is
	 *  drawn TWICE — a dark rim underneath, the magenta on top — so the effective ink is about
	 *  double this; that pairing, ⛔ not the colour alone, is what makes it read at both ends
	 *  (the file's own measured finding: the overlays that fail 3:1 are exactly the ones drawn
	 *  with no outline).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "1.0", ClampMax = "12.0"))
	float MarkRingThicknessPx = 3.f;

	/**
	 *  The number's font size as a FRACTION of the circle's radius — this is what makes
	 *  *"legible at ANY radius"* true rather than true-at-one-size.
	 *
	 *  ⚠️ CONSEQUENCE: the number's visual weight inside its ring. At 0.6 a 40-px circle gets a
	 *  24-pt digit — big enough to read at a glance, small enough that the glyph never touches
	 *  the stroke. Push it past ~0.8 and the digit collides with its own ring at every size;
	 *  drop it below ~0.3 and a big circle gets a digit that looks like a speck at its centre.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MarkNumberFontRadiusFraction = 0.6f;

	/**
	 *  Floor on the number's font size, in pt.
	 *
	 *  ⚠️ CONSEQUENCE: it is the reason a circle scrolled all the way down still SAYS something.
	 *  Without a floor the digit would shrink with the ring until it was an unreadable smudge —
	 *  and the number is the whole feature: it is the name the player speaks to the AI. ⛔ Below
	 *  ~9 pt an outlined glyph loses its counter (the hole in a `9`) and stops being a digit.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "6.0", ClampMax = "64.0"))
	float MarkNumberFontMinSize = 10.f;

	/**
	 *  Ceiling on the number's font size, in pt.
	 *
	 *  ⚠️ CONSEQUENCE: it stops a maximally-scrolled circle from stamping a 144-pt digit across
	 *  the battlefield and burying the dots the map exists to show. ⛔ Must stay above
	 *  `MarkNumberFontMinSize` or the two clamps cross and every number renders at the ceiling.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|Marks", meta = (ClampMin = "8.0", ClampMax = "128.0"))
	float MarkNumberFontMaxSize = 28.f;

	//~ ---------------------------------------------------------------------
	//~ ═══ THE POI ICON LAYER — textures (TASK-685; CONVENTIONS `WM-§1`) ═══
	//~ ⛔⛔ DISPLAY ONLY, restated at the members so the boundary cannot be
	//~ missed: icons are NOT clickable, add NO place symbol, NO grid cell, NO
	//~ snap radius and NO coordinate field. Only the seven `ResolvePlace`
	//~ markers hit-test (`WR-§6` stands byte-untouched by this layer).
	//~ All three textures are WHITE-on-transparent by authored law — every
	//~ colour arrives at DRAW TIME (castles via the `W4-R5` team accessors,
	//~ mine/ancient-ground via one named constant each in the .cpp).
	//~ Soft-referenced with the ATorch TWO-NULL-PATHS idiom, per asset:
	//~   • CLEARED (IsNull)      ⇒ SILENT opt-out — that POI draws the shipped
	//~     dot primitive in its own tint (a designer choice, not a fault).
	//~   • SET but UNRESOLVABLE  ⇒ same dot-primitive fallback + ONE log line
	//~     (never a crash, never silence — the spec's degrade clause).
	//~ Defaults are set in the CONSTRUCTOR, never NativeConstruct — the
	//~ ArenaConfigAsset stomp argument above applies verbatim.
	//~ ---------------------------------------------------------------------

	/** Mine icon — `/Game/UI/WarMap/T_WarMap_Icon_Mine` (TASK-683's pickaxe glyph). Marks the GOLD-NODE mines only (J4: `ADeepMine` is a player-built `ABuilding`, never iconed). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|PoiIcons")
	TSoftObjectPtr<UTexture2D> MineIconTexture;

	/** Ancient-ground icon — `/Game/UI/WarMap/T_WarMap_Icon_AncientGround` (the rune ring). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|PoiIcons")
	TSoftObjectPtr<UTexture2D> AncientGroundIconTexture;

	/** Castle icon — `/Game/UI/WarMap/T_WarMap_Icon_Castle` (the keep). Tinted PER CASTLE TEAM at draw via the `W4-R5` accessors — the one POI whose tint is not a named constant. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|WarMap|PoiIcons")
	TSoftObjectPtr<UTexture2D> CastleIconTexture;

private:

	/**
	 *  ⛔ READ-ONLY. Returns the EXISTING per-turn snapshot or null — ⛔ it never allocates
	 *  one, never captures one and never asks anybody else to (`WR-§6`'s snapshot-hazard row;
	 *  §3 of the class comment for the residual this leaves).
	 */
	const USiegeAssistantSnapshot* GetReadOnlySnapshot() const;

	/**
	 *  The arena extent, from the DataAsset when it loads and from
	 *  `USiegeScatterConfig`'s own CDO when it does not.
	 *
	 *  ⛔ THE FALLBACK IS THE CLASS DEFAULT OBJECT, NOT A TRANSCRIBED NUMBER, AND THAT IS A
	 *  DELIBERATE UPGRADE OVER THE SPEC'S "named fallback constant" (`SC-§15`, declared).
	 *  `SC-§34`'s structural escape says it outright — *"derive at runtime from the asset
	 *  instead of transcribing a number"* — and a transcribed `(26000, 12000)` here is
	 *  precisely the stale-derived-constant defect that clause exists to prevent: it would
	 *  compile, pass every test, and quietly draw the wrong battlefield the day the arena
	 *  changes size.
	 */
	FVector2D ResolveArenaHalfExtent() const;

	//~ ---------------------------------------------------------------------
	//~ ═══ MAP MARK internals (TASK-745; `MARK-§`) ═══
	//~ ⛔ THIS WIDGET OWNS NO MARK STATE. Every function below either READS the
	//~ store or calls exactly one of its five pinned methods; the durable data
	//~ lives in `USiegeMapMarkSubsystem` (TASK-744) and nowhere else, so there
	//~ is no second copy to go stale across an open, a resize or a match reset.
	//~ ---------------------------------------------------------------------

	/**
	 *  The local player's mark store, or null.
	 *
	 *  ⛔ A LOOKUP, ⛔ NEVER A CREATE: `ULocalPlayer::GetSubsystem<T>()` returns the collection's
	 *  existing instance and allocates nothing, so a widget with no owning local player (the
	 *  headless test case, and a viewport teardown) degrades to "no marks" rather than
	 *  manufacturing a store nobody owns.
	 *
	 *  ⚠️ `const` AND YET IT RETURNS A MUTABLE POINTER, which is correct and is the same shape
	 *  `GetOwningLocalPlayer() const` itself has: constness here is about THIS WIDGET, and this
	 *  widget holds no mark state to protect. ⭐ `NativePaint`'s guarantee that painting cannot
	 *  change what is painted is untouched, because the painter's only use of this is
	 *  `BuildMarkCircles`, which calls `GetMarks()` — a `const` read.
	 */
	USiegeMapMarkSubsystem* GetMarkSubsystem() const;

	/**
	 *  PLACE a mark at a local point, or explain why not. Returns true when the click was
	 *  CONSUMED by the mark lane (placed, or refused with a status line naming the cap).
	 *
	 *  ⛔ FALSE MEANS "THE MARK LANE IS NOT AVAILABLE HERE", AND THE CALLER THEN RUNS THE
	 *  SHIPPED EMPTY-CLICK ARM VERBATIM — that is how `WR-§9` outcome 1's retired behaviour
	 *  survives as a real degrade path instead of being deleted. False for: no store reachable,
	 *  or a point outside the drawn map rect (the letterbox — `LocalToMapUV` refuses, and its
	 *  comment says why clamping there would be a lie).
	 *
	 *  ⚠️ AT THE CAP IT REFUSES **LOUDLY** — the shipped refusal doctrine, `M-5`: a status line
	 *  naming how many circles exist and what to do about it, ⛔ never a silent no-op. The
	 *  number in that line is `GetMarks().Num()` read at the moment of refusal — ⛔ not a
	 *  transcribed 9 — so it cannot disagree with the store's own cap.
	 */
	bool TryPlaceMarkAtLocal(const FGeometry& InGeometry, const FVector2D& LocalPoint);

	/** DELETE the mark under a local point. Returns true when one was deleted. ⛔ The number is NOT reused by a renumber — the hole stays (`M-1`); the store's allocator hands it out again only when it is the LOWEST FREE one. */
	bool TryDeleteMarkAtLocal(const FGeometry& InGeometry, const FVector2D& LocalPoint);

	/** RESIZE the mark under a local point by one wheel notch. Returns true when one was resized. ⛔ A miss changes nothing, anywhere — that is the readable half of "the wheel outside a mark does nothing". */
	bool TryResizeMarkAtLocal(const FGeometry& InGeometry, const FVector2D& LocalPoint, float WheelDelta);

	/** Own-team `ASummonedUnit` + `AHeroCharacter` positions, refreshed on the timer while open. Cleared on close. */
	void RefreshAllyDots();

	/**
	 *  ⭐ THE ONE THING THE REFRESH TIMER CALLS: the ally sweep, then the POI icon census
	 *  (TASK-685), then the hint latch (TASK-579, re-pointed by TASK-685), then the
	 *  elevation cache's staleness check (TASK-684).
	 *
	 *  ⚖️ ONE TIMER, FOUR JOBS, AND THE ALTERNATIVE IS WORSE. A second timer for a bool that
	 *  flips ONCE per open — or a third for a cache that re-bakes once per MATCH — would be
	 *  another lifetime to start, stop, clear on close and clear again on teardown — the exact
	 *  bookkeeping `NativeDestruct` already carries a double-clear for. ⛔ And the riders cost
	 *  little-to-nothing at steady state: `UpdateNoSnapshotHint` early-outs on its bool BEFORE
	 *  it reads anything, `EnsureElevationBake` early-outs on two pointer checks, and the POI
	 *  census is the ally sweep's own cost class (a handful of world actors, 4×/s, open only).
	 */
	void HandleMapRefreshTimer();

	/** Starts/stops the refresh timer with the map. ⛔ Nothing iterates actors while the map is closed. */
	void SetAllyRefreshTimerEnabled(bool bEnabled);

	/**
	 *  Pushes one chrome line to `StatusTextBlock` when bound, and to `OnWarMapStatusLine` always.
	 *
	 *  ⚠️ IT ALSO CLEARS `bShowingNoSnapshotHint`, AND THAT IS THE INVARIANT RATHER THAN A SIDE
	 *  EFFECT (TASK-579): the latch means *"the hint is the line ON SCREEN RIGHT NOW"*, so every
	 *  line that replaces it must retire it. ⭐ ONE writer for `false` (here) and ONE for `true`
	 *  (`ShowNoSnapshotHint`, which re-arms AFTER calling this) ⇒ the flag physically cannot
	 *  claim the hint is showing when a picked symbol overwrote it.
	 */
	void SetStatusLine(const FString& Line);

	/**
	 *  Shows the first-open explanation and arms the latch. ⛔ It explains; it does ⛔ NOT fix
	 *  (see §3b — the repair is TASK-580's and lives in `USiegeAssistantComponent`).
	 */
	void ShowNoSnapshotHint();

	/**
	 *  ⛔ THE HINT MUST DISAPPEAR — A STALE HINT IS ITS OWN DEFECT (TASK-579 spec item 5).
	 *
	 *  Runs on the map's existing refresh timer while the map is open. One-way: it only ever
	 *  RETIRES the hint, and only when the hint is what is on screen AND the snapshot now
	 *  lists at least one resolved place (the `W691-3` re-point — the retire condition
	 *  mirrors the arming condition, so the hint and the marker layer cannot disagree) — so
	 *  it can never stomp a symbol the player just clicked, and never re-arms mid-open.
	 *
	 *  ⚠️ Bound above by the refresh interval (0.25 s by default), ⛔ not instantaneous — stated
	 *  rather than implied. The MARKERS themselves appear on the very next paint; only the line
	 *  of text lags, by at most one interval.
	 */
	void UpdateNoSnapshotHint();

	//~ ---------------------------------------------------------------------
	//~ Elevation bake internals (TASK-684; `WM-§2`). ⛔ Every writer below is a
	//~ NON-const path (`OpenMap` / the refresh timer) — `NativePaint`'s const
	//~ guarantee that painting cannot change what is painted stays intact: the
	//~ painter only READS `ElevationTexture`/`ElevationBrush`.
	//~ ---------------------------------------------------------------------

	/**
	 *  The lazy gate: (re)bakes when — and only when — the cache is stale. Steady state
	 *  costs TWO pointer checks; a valid cache is never re-traced.
	 *
	 *  ⚖️ THE STALENESS KEY, DIAGNOSED AT SOURCE RATHER THAN COPIED FROM THE SPEC'S
	 *  SUGGESTION (`SC-§20` — the spec itself says the `TWeakObjectPtr<UWorld>` key is "the
	 *  suggested shape, not a spec"): **a world key ALONE is provably insufficient here,
	 *  because Play Again is an IN-PLACE reset** — `ASiegeGameMode::PlayAgain` drives
	 *  `ClearScatter()` + `GenerateScatter()` on the SAME `UWorld` (no travel), so the hills
	 *  re-roll while the world pointer never changes. The world key is kept (level travel,
	 *  stale-world safety) and a SCATTER-LIFECYCLE SENTINEL is layered on top: `ClearScatter`
	 *  **DESTROYS** the scatter-spawned mine + ancient-ground actors and every generate
	 *  spawns FRESH ones (BattlefieldScatter.h — `SpawnedMines`/the ancient-ground pair,
	 *  "destroyed, never pooled"), so a weak pointer to one of them goes invalid EXACTLY
	 *  when the field re-rolled. Verified at source; no scatter file is touched for this.
	 */
	void EnsureElevationBake();

	/**
	 *  The one-shot 7,800-trace capture → ONE transient `UTexture2D`. Never called directly —
	 *  `EnsureElevationBake` owns the cache discipline. Returns false (and leaves the shipped
	 *  flat background standing) on: no world, degenerate rect, texture-allocation failure,
	 *  or ZERO trace hits. ⛔ Never a crash; failure logs ONCE (`LogSiegeWarMap`, Log).
	 */
	bool BakeElevationTexture();

	//~ ---------------------------------------------------------------------
	//~ POI icon internals (TASK-685; `WM-§1`). ⛔ DISPLAY ONLY: nothing below
	//~ is hit-testable, nothing enters the marker array, and no position read
	//~ here ever leaves this widget or reaches a prompt zone (`WR-§6`). Every
	//~ writer is a NON-const path (`OpenMap` / the refresh timer); the painter
	//~ only READS the arrays/brushes, so `NativePaint`'s const guarantee that
	//~ painting cannot change what is painted stays intact.
	//~ ---------------------------------------------------------------------

	/**
	 *  Loads any still-unresolved icon soft ref and configures its brush. `OpenMap` path
	 *  (never paint — it loads). Idempotent: an already-resolved icon is never re-loaded.
	 *  The SET-but-unresolvable family logs ONCE (`bWarnedPoiIconUnresolved`); a CLEARED
	 *  soft ref is a silent designer opt-out (the ATorch two-null-paths law) — both degrade
	 *  to the shipped dot primitive in that POI's tint at paint.
	 */
	void ResolvePoiIconTextures();

	/**
	 *  The POI census: world `(X, Y)` of the live gold-node mines (⛔ skipping depleted ones
	 *  — `AGoldNode::IsDepleted`, diagnosed at source; ⛔ `ADeepMine` is an `ABuilding`, so
	 *  the `AGoldNode` iterator excludes player-built DeepMines BY CONSTRUCTION, J4), both
	 *  ancient grounds, and the standing castles split BY TEAM for the `W4-R5` tint.
	 *
	 *  READ-ONLY over live world actors — the ally-dot precedent `WM-§1` names (that layer
	 *  already iterates world actors and is not snapshot-gated; neither is this one).
	 *  Runs on the same refresh timer as the dots, plus the `OpenMap` seed; never while the
	 *  map is closed. ⛔ NOT a place iterator: no `PlaceVocabulary` symbol is involved, no
	 *  `ResolvePlace` answer is duplicated — this asks "where are the world's POIs", never
	 *  "where is place symbol X" (`WR-§6`: `ResolvePlace` stays the single owner of
	 *  place → position).
	 */
	void RefreshPoiIcons();

	/** Resolved mine icon. ⛔ `Transient` `UPROPERTY` hard ref on purpose: the class's ownership of the loaded asset for GC — the brush below is non-reflected and invisible to GC (the `ElevationTexture` idiom). Null ⇒ cleared, unresolvable, or not yet opened ⇒ dot-primitive fallback. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MineIconResolvedTexture;

	/** Resolved ancient-ground icon. Same contract as `MineIconResolvedTexture`. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> AncientGroundIconResolvedTexture;

	/** Resolved castle icon. Same contract as `MineIconResolvedTexture`. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CastleIconResolvedTexture;

	/** Wraps `MineIconResolvedTexture` for `FSlateDrawElement::MakeBox`. Configured at resolve; only READ under `NativePaint` (the `ElevationBrush` discipline). */
	FSlateBrush MineIconBrush;

	/** Wraps `AncientGroundIconResolvedTexture`. */
	FSlateBrush AncientGroundIconBrush;

	/** Wraps `CastleIconResolvedTexture`. Tinted per element at draw (blue/red by castle team), never on the brush. */
	FSlateBrush CastleIconBrush;

	/** Gold-node mine positions in WORLD `(X, Y)` — the census's mine list. Projected at paint, never stored projected (the `AllyDotsWorldXY` doctrine). Cleared on close/teardown. */
	TArray<FVector2D> MinePoiWorldXY;

	/** Ancient-ground positions in WORLD `(X, Y)`. Same lifecycle as `MinePoiWorldXY`. */
	TArray<FVector2D> AncientGroundPoiWorldXY;

	/** BLUE-team standing castles in WORLD `(X, Y)`. Split by team so the paint loop pairs each list with one `W4-R5` accessor and carries no per-element branch. */
	TArray<FVector2D> BlueCastlePoiWorldXY;

	/** RED-team standing castles in WORLD `(X, Y)`. See `BlueCastlePoiWorldXY`. */
	TArray<FVector2D> RedCastlePoiWorldXY;

	/** One-shot log latch for the unresolvable-icon line. ⛔ NOT `mutable` — every writer is the non-const `OpenMap` path (the `bWarnedElevationBakeFailed` discipline, not the paint-path latches'). */
	bool bWarnedPoiIconUnresolved = false;

	/** Auto-wire target for `RevealButton`. Broadcasts and nothing else — ⛔ it prices nothing. */
	UFUNCTION()
	void HandleRevealButtonClicked();

	/** Auto-wire target for `CloseButton`. */
	UFUNCTION()
	void HandleCloseButtonClicked();

	/** True while the map is on screen and hit-testable. */
	bool bMapOpen = false;

	/**
	 *  TASK-745 — the mark NUMBER the cursor is currently over, or 0 for none.
	 *
	 *  ⛔ A **NUMBER**, ⛔ NOT AN INDEX, for `M-1`'s reason: indices shift when a mark is
	 *  deleted and numbers never do, so an index held across a delete would highlight the wrong
	 *  circle for exactly as long as the cursor stayed still.
	 *
	 *  ⛔ COSMETIC ONLY. ⛔ NOT `mutable` — every writer (`NativeOnMouseMove`,
	 *  `NativeOnMouseLeave`, `CloseMap`, the delete path) is a non-const path, so
	 *  `NativePaint`'s const guarantee is untouched; the painter only READS it. ⚠️ And ⛔ no
	 *  verb reads it: the wheel recomputes its target from its own event's cursor position, so
	 *  a stale value here can make the picture wrong for one frame and can ⛔ never resize,
	 *  delete or name the wrong circle.
	 */
	int32 HoveredMarkNumber = 0;

	/**
	 *  TASK-579 — true EXACTLY WHEN the first-open explanation is the status line on screen.
	 *
	 *  ⛔ NOT `mutable`, and the difference from the two log latches below is deliberate: this
	 *  one is written only by non-const paths (`OpenMap`, `SetStatusLine`, the timer, the
	 *  click), so `NativePaint`'s const guarantee — that painting cannot change what is
	 *  painted — is untouched by it.
	 *
	 *  ⚠️ `OpenMap` RE-EVALUATES IT UNCONDITIONALLY, which is why `CloseMap` deliberately does
	 *  not reset it: the only reader is `UpdateNoSnapshotHint`, the only caller of that is the
	 *  refresh timer, and the timer is cleared on close. A value left standing between close
	 *  and re-open is observed by nothing and is overwritten before it could be.
	 */
	bool bShowingNoSnapshotHint = false;

	/**
	 *  ⛔ ONE-SHOT LOG LATCHES. Both conditions are per-frame-reachable, so an unlatched
	 *  warning would fill the log at 60 Hz and bury the line that matters — the shipped
	 *  `WarnedRosterKindsPrinted` idiom.
	 *
	 *  ⚠️ `mutable` BECAUSE THE READERS ARE `const` AND MUST STAY THAT WAY. `NativePaint` is
	 *  a `const` override, so `BuildMarkerRects`, `GetReadOnlySnapshot` and
	 *  `ResolveArenaHalfExtent` are all const beneath it. ⛔ These two bools are the ONLY
	 *  mutable state in the class, they are pure log bookkeeping, and nothing observable
	 *  depends on them — dropping the `const` to avoid `mutable` would have cost the
	 *  compiler-enforced guarantee that painting cannot change what is painted.
	 */
	mutable bool bWarnedNoSnapshot = false;
	mutable bool bWarnedNoArenaConfig = false;

	/**
	 *  The baked elevation layer (TASK-684, `WM-§2`). ⛔ TRANSIENT AND `UPROPERTY` ON
	 *  PURPOSE: `UTexture2D::CreateTransient` textures are unreferenced by any asset, so this
	 *  pointer is the ONLY thing keeping the bake alive across GC — `ElevationBrush` below is
	 *  NOT a reflected member and its resource-object reference is invisible to the GC.
	 *  Null ⇒ no bake exists (never opened / bake failed) and the painter draws exactly the
	 *  pre-TASK-684 background.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ElevationTexture;

	/** Wraps `ElevationTexture` for `FSlateDrawElement::MakeBox`. Configured at bake; only READ under `NativePaint`. */
	FSlateBrush ElevationBrush;

	/** Cache key 1: the world the bake was traced in. Stale/different world ⇒ re-bake (level travel; the `SC-§20` suggested shape). */
	TWeakObjectPtr<UWorld> ElevationBakedWorld;

	/**
	 *  Cache key 2: a scatter-spawned per-generate actor (a mine, else an ancient ground)
	 *  captured at bake time. `ClearScatter` destroys these and every generate spawns fresh
	 *  ones ⇒ this going invalid means THE FIELD RE-ROLLED (Play Again's in-place reset —
	 *  the case the world key cannot see; `EnsureElevationBake`'s comment has the diagnosis).
	 *  ⛔ IDENTITY ONLY: no position, name or state is ever read off it.
	 */
	TWeakObjectPtr<const AActor> ElevationGenerationSentinel;

	/**
	 *  True when a sentinel was CAPTURED at bake time. When false (a field with no scatter
	 *  actors at all — debug fields; a client whose deterministic re-generate has not run
	 *  yet), the refresh-timer path watches for one APPEARING and re-bakes then, so an
	 *  early-open client still picks the hills up within one refresh interval. On the shipped
	 *  field the economy law guarantees mines exist, so this is effectively always true.
	 */
	bool bElevationSentinelArmed = false;

	/** One failed bake attempt per open, MAX — stops a pathological world (zero trace hits) from re-tracing 7,800 rays on every timer tick. Reset by `OpenMap`. */
	bool bElevationBakeFailedThisOpen = false;

	/** One-shot log latch for the failed-bake line. ⛔ NOT `mutable` — every writer is non-const (unlike the two paint-path latches above). */
	bool bWarnedElevationBakeFailed = false;

	/** Ally positions in WORLD `(X, Y)`. Projected at paint, never stored projected — a resize must not need a re-survey. */
	TArray<FVector2D> AllyDotsWorldXY;

	/** The paid reveal's FROZEN snapshot, in WORLD `(X, Y)`. ⛔ Does not track; ⛔ cleared on close, always (`WR-§7`). */
	TArray<FVector2D> EnemyDotsWorldXY;

	/** The ally refresh timer. Cleared in `CloseMap` and again in `NativeDestruct`. */
	FTimerHandle AllyDotTimerHandle;
};
