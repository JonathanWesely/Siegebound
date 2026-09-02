// Copyright Epic Games, Inc. All Rights Reserved.

#include "WarMapWidget.h"

// FCollisionObjectQueryParams / FCollisionQueryParams for the elevation bake (TASK-684) —
// included by name rather than inherited through Engine/World.h (the complete-type-include law).
#include "CollisionQueryParams.h"
#include "Components/Button.h"
// UPrimitiveComponent::IsVisible - the bake's invisible-proxy rejection (TASK-684); included
// by name, never inherited (the complete-type-include law).
#include "Components/PrimitiveComponent.h"
#include "Components/TextBlock.h"
// ⚠️ EngineUtils.h is TActorIterator's home. THREE iteration families live in this file, and
// ALL are comment-audited: (1) the ALLY DOT sweep (RefreshAllyDots); (2) the elevation bake's
// SCATTER-GENERATION SENTINEL probe (TASK-684), which reads actor IDENTITY ONLY — never a
// position; (3) the POI ICON census (RefreshPoiIcons, TASK-685), which reads world POSITIONS
// off AGoldNode / AAncientGround / ACastle for the DISPLAY-ONLY icon layer (WM-§1 — the
// ally-dot precedent, extended to the field's fixtures). ⛔ There is still no PLACE iterator
// here and there must never be one: USiegeAssistantSnapshot::ResolvePlace is the SINGLE OWNER
// of place → position, and a second search would be a second answer that drifts (WR-§6). The
// census asks "where are the world's mines/grounds/castles", never "where is place symbol X".
#include "EngineUtils.h"
// FHitResult's own home since UE5 - the bake fills a TArray<FHitResult> (TASK-684).
#include "Engine/HitResult.h"
// ULocalPlayer::GetSubsystem<T>() - the mark store's ONLY reachable route from a widget
// (TASK-745). Included by name, never inherited through UserWidget.h (the complete-type law).
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
// ⚠️ REQUIRED, NOT INHERITED — CHECKED, NOT ASSUMED. NativeOnMouseButtonDown compares against
// EKeys::LeftMouseButton, and EKeys lives here; UserWidget.h brings FPointerEvent but not the
// key table. The console widget's header carries the same include for the same reason.
#include "InputCoreTypes.h"
// FTexturePlatformData / the mip bulk data the bake writes (TASK-684) — same
// included-by-name discipline as CollisionQueryParams.h above.
#include "TextureResource.h"
#include "TimerManager.h"

// ── Slate, for the C++ painter (WR-§6's rendering row) ──
// ⚠️ Both modules are ALREADY public dependencies of this module and neither is added by
// this task: GitClaudeUnrealTest.Build.cs lists "Slate" and "SlateCore" with a standing
// comment explaining that SlateCore is not implied by Slate. ⛔ No Build.cs change is owed.
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
// ⭐ TASK-745 - THE NUMBER IS CENTRED, AND CENTRING NEEDS A MEASUREMENT. All three headers are
// SlateCore (already a public dependency; ⛔ no Build.cs change is owed) and all three are
// included BY NAME rather than inherited:
//   • SlateApplicationBase.h - FSlateApplicationBase::Get()/IsInitialized(). ⚠️ The BASE, ⛔ not
//     FSlateApplication: the base lives in SlateCore beside everything else this painter uses,
//     it exposes the renderer, and it is the narrower dependency of the two.
//   • SlateRenderer.h        - FSlateRenderer::GetFontMeasureService().
//   • FontMeasure.h          - FSlateFontMeasure::Measure(), whose result the pure
//                              FSiegeWarMapProjection::CentreTextTopLeft turns into a top-left.
#include "Application/SlateApplicationBase.h"
#include "Fonts/FontMeasure.h"
// FFontOutlineSettings is passed BY VALUE into FCoreStyle::GetDefaultFontStyle's third
// parameter (CoreStyle.h:51), so the complete type is required. CoreStyle.h already pulls it in
// transitively; named here anyway, per this file's standing complete-type-include law.
#include "Fonts/SlateFontInfo.h"
#include "Rendering/SlateRenderer.h"

#include "Siegebound/AncientGround.h"               // TASK-684 SENTINEL fallback (identity only) + TASK-685 POI icon census (position, display-only)
#include "Siegebound/Castle.h"                      // TASK-685 POI icon census - position + GetTeamId() for the W4-R5 tint + IsCastleDestroyed(); display-only (WM-§1)
#include "Siegebound/CombatantHealthBarComponent.h" // the SHIPPED team palette - GetDefaultBlueBarColor() / GetDefaultRedBarColor()
#include "Siegebound/GoldNode.h"                    // TASK-684 SENTINEL (identity only) + TASK-685 POI icon census (position + IsDepleted(), display-only)
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/ScatterConfig.h"               // USiegeScatterConfig::ArenaHalfExtent - the single owner
#include "Siegebound/SiegeAssistantComponent.h"     // GetTurnSnapshot() - READ-ONLY (WR-§6 snapshot hazard)
#include "Siegebound/SiegeAssistantSnapshot.h"      // GetPlaceNames() / ResolvePlace()
#include "Siegebound/SiegeMapMark.h"                // TASK-744 - FSiegeMapMark + the MakeSymbol seam (MARK-§5)
#include "Siegebound/SiegeMapMarkSubsystem.h"       // TASK-744 - the store; consumed through the PINNED registry ONLY
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SummonedUnit.h"

DEFINE_LOG_CATEGORY(LogSiegeWarMap);

namespace SiegeWarMap
{
	/**
	 *  Static CHROME. ⛔ These are the ONLY player-visible words this file authors, and none
	 *  of them names an order, a unit, a count or an outcome — the console's §3 discipline,
	 *  applied to a display that has no reason to say anything else.
	 *
	 *  ⛔ AND NONE OF THEM IS A PROMPT STRING. Nothing in this file is ever serialized into
	 *  any zone; Zone A stays byte-frozen at 5658 chars and TASK-564 asserts it (WR-§6).
	 */
	static const TCHAR* EmptyClickHintText = TEXT("Click a marked place to add its name to the console.");

	/** Shown once on open, before anything is clicked. */
	static const TCHAR* OpenStatusText = TEXT("War map");

	/**
	 *  ⭐⛔ TASK-579's STRING, RE-AIMED BY TASK-685 (ruling W691-3) AT THE STATE THAT ACTUALLY
	 *  SHIPS. The NAME is kept historical (TASK-579 called the state "no snapshot") so the
	 *  579/580/691 paper trail still greps; the COMMENT below is the truth.
	 *
	 *  Shown when the map is open and the snapshot lists NO RESOLVED PLACE - which TASK-691
	 *  proved is every match's opening state: the old narration here ("ResolvePlace lives on
	 *  a snapshot the assistant allocates on its turn path, so it does not exist until the
	 *  player's first sentence") was FALSE at every anchor since TASK-447 (cd5f4ed) -
	 *  USiegeAssistantComponent::BeginPlay() calls EnsureSnapshot(), so the OBJECT exists
	 *  from match start; what is missing is its CONTENT (PlaceNames' one append site is
	 *  Capture()'s resolved-slot loop, reachable only from the turn path). A null snapshot
	 *  (no controller/component route) shows this same line - same remedy either way.
	 *
	 *  ⛔ THIS LINE EXPLAINS THE EMPTY STATE. IT DOES NOT POPULATE IT. The repair is TASK-580's
	 *  and belongs in USiegeAssistantComponent (WarMapWidget.h §3b).
	 *
	 *  ── FOUR DECISIONS INSIDE ~150 CHARACTERS, EACH ONE DELIBERATE ──
	 *
	 *  (1) ⛔ IT NAMES NO KEY. The console's key lives in IMC_Hero, a BINARY asset no file-only
	 *      task can read, and the KEYBOARD-LAYOUT batch remaps bindings POSITIONALLY - so a
	 *      transcribed letter would be a guess that is additionally WRONG for a Dvorak player.
	 *      A UI string asserting an unverified fact is the stale-derived-constant class
	 *      (SC-§34) wearing player-facing words. The console widget's shipped "Press Z to
	 *      accept" precedent exists, and it is deliberately NOT followed here for that reason.
	 *  (2) ✅ IT STATES THE REMEDY IS REACHABLE WITHOUT CLOSING THE MAP. TASK-563 shipped that
	 *      (D-1: the map and the console are NOT mutually exclusive), and it matters
	 *      MECHANICALLY, not just for comfort: closing the map DISCARDS a paid 30-gold reveal
	 *      (WR-§7), so "go close the map and come back" would be advice that costs gold.
	 *  (3) ⛔ IT NEVER SAYS "SNAPSHOT", "CAPTURE", "ASSISTANT" OR "TURN". The player has no
	 *      model of any of them; he has a commander he can talk to.
	 *  (4) ⛔ PURE ASCII, matching every other player-facing string in this file. Non-ASCII
	 *      inside TEXT() compiles fine here (12 such literals ship in SiegeAssistantComponent.cpp
	 *      alone), but this string is RENDERED by Slate's default font, and a glyph the font
	 *      lacks is a visible defect rather than a build one.
	 *
	 *  ⛔ AND IT IS CHROME, NOT A PROMPT. Like every other string in this namespace it is never
	 *  serialized into any zone; Zone A stays byte-frozen at 5658 chars (WR-§6).
	 */
	static const TCHAR* NoSnapshotStatusText = TEXT("No places surveyed yet - send your commander one order in the console and the markers appear. The console opens over this map; no need to close it.");

	/**
	 *  MARKER CHROME COLOUR - a PLACE is not a TEAM, so this is deliberately NOT drawn from
	 *  the team palette.
	 *
	 *  ⛔ AND THIS IS NOT THE "never a new hardcoded colour" VIOLATION IT MIGHT LOOK LIKE ON
	 *  A FAST READ. That rule (WR-§6's ally-dot row) binds the TEAM tints - the dots - which
	 *  ARE read from the shipped palette below and are never re-typed here. A marker glyph is
	 *  widget chrome in exactly the sense the console's BackdropColor / TranscriptColor /
	 *  StatusColor are, and it belongs to the same shipped precedent.
	 *
	 *  Legibility is law here, not decoration: this project has already shipped a white fill
	 *  on a white track (the M5.5 health-bar defect), so the glyph is a bright warm neutral
	 *  over a near-black outline and must read over ANY background WBP_WarMap supplies.
	 */
	static const FLinearColor MarkerColor        = FLinearColor(1.00f, 0.86f, 0.45f, 1.00f);
	static const FLinearColor MarkerOutlineColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.90f);
	static const FLinearColor MarkerLabelColor   = FLinearColor(0.97f, 0.94f, 0.86f, 1.00f);

	/** Half the drawn glyph, in local px. ⚠️ SMALLER than MarkerHitHalfSizePx on purpose: the click target is generous, the ink is not. */
	static constexpr float MarkerDrawHalfSizePx = 7.f;

	/**
	 *  Half the drawn ENEMY dot, in local px. ⚠️ TASK-685 (WM-§3) PROMOTED the ALLY dot's
	 *  half-size off this literal to the EditDefaultsOnly AllyDotRadius tunable (bumped for
	 *  contrast against the dark elevation background); the enemy dots deliberately keep the
	 *  shipped size - the WR-§7 reveal lane is not this wave's to restyle (flagged in the
	 *  TASK-685 handoff; making them ride a tunable is one line if Jonathan asks).
	 */
	static constexpr float DotDrawHalfSizePx = 3.f;

	// ── The POI icon layer (TASK-685; WM-§1) ────────────────────────────────────

	/**
	 *  Half the drawn POI icon, in local px - 24 px total, the exact size TASK-683's three
	 *  glyphs were legibility-gated at on a contact sheet over a dark map ground. Bigger
	 *  than a unit dot (a fixture outranks a soldier), smaller than a marker's generous
	 *  36-px hit rect (the SEVEN markers stay the map's primary affordance). Widget chrome
	 *  in the MarkerDrawHalfSizePx sense - a UI affordance, never a world radius.
	 */
	static constexpr float PoiIconDrawHalfSizePx = 12.f;

	/**
	 *  MINE / ANCIENT-GROUND ICON TINTS - ONE named neutral constant per icon (WM-§1's own
	 *  clause: castles ride the W4-R5 team accessors at draw; the team-less POIs each get a
	 *  named constant here). ⛔ NOT the "never a new hardcoded colour" violation - the same
	 *  MarkerColor argument above: a mine is not a team, so the team palette is exactly the
	 *  WRONG owner for its tint, and these are widget chrome in the shipped
	 *  BackdropColor/TranscriptColor precedent. The textures themselves are PURE WHITE on
	 *  every texel (TASK-683's authored law), so a draw-time tint multiplies cleanly with
	 *  zero fringe.
	 *
	 *  ⚠️⚠️ COLOUR SPACE - THESE TWO ARE **TRUE LINEAR**, ⛔ NOT the space the elevation
	 *  ramp's constants below are in, AND THAT DIFFERENCE IS INVISIBLE IN THE TYPE NAME.
	 *  An overlay tint reaches the screen through SLATE, which sRGB-ENCODES it AT DRAW TIME
	 *  (PackVertexColor -> FLinearColor::ToFColor(bSRGBVertexColor), and
	 *  FSlateRHIRenderingPolicy::IsVertexColorInLinearSpace() returns false, so the encode
	 *  always happens). ⇒ what these literals mean is LINEAR LIGHT, and the screen shows
	 *  srgb_encode(them).
	 *  ⛔⛔ ElevationGrassDark / ElevationGrassLight below are the OPPOSITE - DISPLAY-ENCODED
	 *  sRGB values in the same FLinearColor type - because they never pass through Slate at
	 *  all; they become the raw BYTES of a texture created with SRGB = true. ⇒ THIS FILE
	 *  HOLDS FLinearColor CONSTANTS IN TWO DIFFERENT COLOUR SPACES, A FEW HUNDRED LINES
	 *  APART. ⛔ Never copy a value between the two families, and ⛔ never "correct" one into
	 *  the other: BOTH readings compile, BOTH render, and only one is right at each site.
	 *
	 *  ⭐ WHY AncientGroundIconTint MOVED (TASK-721/722, WM-§8c). It was (0.62, 0.93, 0.66)
	 *  "pale verdant" - a PALE GREEN picked against a GRAYSCALE background. Measured against
	 *  the new green ramp's light end it was 1.03:1 contrast at dE 36.7: the only overlay in
	 *  the file that collides on LUMINANCE AND HUE AT ONCE. Deep emerald (0.04, 0.22, 0.09)
	 *  measures 3.79:1 against the ramp's dark end and 3.89:1 against its light end, versus a
	 *  3.84:1 theoretical ceiling for ANY flat colour over this ramp's 14.74:1 span.
	 *  ⚖️ WM-§8c's ruling, applied: THE ICON YIELDS, ⛔ the ramp never comes off the measured
	 *  grass Jonathan asked for. Only the tint's VALUE moved - it is still verdant, and it is
	 *  still the furthest of every passing candidate from BOTH team hues (dE 89.8 from blue,
	 *  100.9 from red), which is exactly what the first paragraph says this constant is for.
	 *
	 *  🚩 KNOWN, MEASURED, ⛔ DELIBERATELY NOT REPAIRED HERE (TASK-721 flagged these and did
	 *  not fix them; neither did TASK-722). MineIconTint (1.09:1) and MarkerLabelColor
	 *  (1.15:1) also sit under the 3:1 gate at the ramp's light end - but they were ALREADY
	 *  under it against the OLD gray ramp (1.33:1 and 1.06:1), so ⛔ the green did not break
	 *  them. ⭐ THE STRUCTURAL FINDING: the three overlays that fail are EXACTLY the three
	 *  drawn with NO OUTLINE. The marker GLYPH is fine (12.22:1) because it gets
	 *  MarkerOutlineColor; its LABEL, drawn by MakeText a few lines later, gets none.
	 *  ⚖️ No flat colour can clear 3:1 against BOTH ends of a 14.74:1 background by luminance
	 *  alone, so the repair for those two is an OUTLINE, ⛔ not another re-tint - and it is
	 *  not this task's to make.
	 */
	static const FLinearColor MineIconTint          = FLinearColor(1.00f, 0.72f, 0.18f, 1.00f); // TRUE LINEAR (Slate tint) - gold - what a gold-node mine yields; reads beside the warm marker family, apart from BOTH team hues
	static const FLinearColor AncientGroundIconTint = FLinearColor(0.04f, 0.22f, 0.09f, 1.00f); // TRUE LINEAR (Slate tint) - deep emerald - still verdant and still mystic-neutral, at a value that clears 3:1 on BOTH ramp ends

	/** Gap between a marker's right edge and its label. */
	static constexpr float MarkerLabelGapPx = 6.f;

	static constexpr float MarkerLabelFontSize = 11.f;

	// ── MAP MARKS - the player's numbered circles (TASK-745; MARK-§) ────────────

	/**
	 *  ⭐⭐ THE MARK RING'S COLOUR - THE ONE NEW COLOUR CONSTANT THIS WHOLE FEATURE ADDS.
	 *
	 *  ⚠️⚠️ COLOUR SPACE: **TRUE LINEAR**, because it is a SLATE TINT and Slate sRGB-ENCODES a
	 *  tint at draw time. ⛔ It is the OPPOSITE space from ElevationGrassDark/Light below, which
	 *  are DISPLAY-ENCODED sRGB in the same FLinearColor container because they become the raw
	 *  bytes of an SRGB=true texture and never pass through Slate at all. ⛔⛔ NEVER copy a value
	 *  between the two families and ⛔ never "correct" one into the other: BOTH readings compile,
	 *  BOTH render, and only one is right at each site (WM-§8e). The full argument is at the POI
	 *  tint block above; this line restates the SPACE because WM-§8e's closing clause is
	 *  absolute - "a colour constant whose space is not written beside it is not shippable."
	 *
	 *  ── WHY MAGENTA, AND ⛔ NOT BY EYE (WM-§8b's method, applied to an overlay) ──
	 *
	 *  ⭐ FIVE THINGS WERE ALREADY ON THIS MAP AND ⛔ NONE OF THEM MAY BE COLLIDED WITH: the
	 *  GREEN elevation ramp (the whole background), BLUE ally dots + blue castle icons
	 *  (GetDefaultBlueBarColor), RED enemy dots + red castle icons (GetDefaultRedBarColor), GOLD
	 *  mine icons and GOLD place-marker glyphs, and DEEP EMERALD ancient grounds (itself just
	 *  re-tinted by TASK-722 to survive the green). ⇒ ⭐ MAGENTA IS THE ONE STRONG HUE THE
	 *  TACTICAL PALETTE HAS NOT SPENT, and it is the EXACT COMPLEMENT of the map's green - the
	 *  maximum possible hue separation from the background, at both of its ends at once. It also
	 *  reads correctly as what a mark IS: a player's annotation over the game's own colours,
	 *  rather than another faction, fixture or resource.
	 *
	 *  ── THE LUMINANCE, DERIVED ⛔ NOT PICKED, AGAINST THE RAMP THIS FILE ALREADY OWNS ──
	 *
	 *  The ramp spans Y = 0.008651 (dark) to Y = 0.814602 (light) in relative luminance, i.e.
	 *  the 14.74:1 span WM-§8c records. For a FLAT overlay the two contrast ratios move in
	 *  opposite directions, so the best any flat colour can do is where they meet:
	 *      (Y+0.05)² = (0.814602+0.05)·(0.008651+0.05)  ⇒  Y = 0.17519, ratio 3.84:1
	 *  - which is exactly the 3.84:1 theoretical ceiling WM-§8c states. ⇒ this constant is
	 *  TUNED TO THAT OPTIMUM rather than to a look:
	 *      Y = 0.2126·0.52 + 0.7152·0.035 + 0.0722·0.54 = 0.17457
	 *      vs the ramp's DARK end : (0.17457+0.05)/(0.008651+0.05) = 3.83:1  ✅
	 *      vs the ramp's LIGHT end: (0.814602+0.05)/(0.17457+0.05) = 3.85:1  ✅
	 *  ⭐ Within 0.3% of the ceiling on BOTH ends - i.e. it clears the 3:1 gate everywhere on a
	 *  background no flat colour can clear by more than 3.84:1. As display bytes, for the reader
	 *  who wants them: (191, 53, 194).
	 *
	 *  ⚠️ AND THE RING IS STILL DRAWN OVER A DARK RIM, BECAUSE 3.84:1 IS A CEILING AND NOT A
	 *  COMFORT. This file's own measured finding is that the three overlays failing the gate are
	 *  EXACTLY the three drawn with NO OUTLINE, and that "no flat colour can clear 3:1 against
	 *  BOTH ends of a 14.74:1 background by luminance alone, so the repair is an OUTLINE, ⛔ not
	 *  another re-tint." ⇒ the mark ring reuses the SHIPPED MarkerOutlineColor as a rim (the
	 *  same pairing that puts the marker glyph at 12.22:1) and the centred number carries a font
	 *  OUTLINE in that same colour. ⛔ NO new rim colour is typed: reusing the proven one is one
	 *  fewer constant, in a file where a constant's SPACE is the thing that goes wrong.
	 *
	 *  ⚠️ DISTINCTNESS FROM THE FOUR NEIGHBOURS, SPOT-CHECKED IN LINEAR CHANNELS RATHER THAN
	 *  ASSERTED: vs BLUE (0.05, 0.30, 1.00) the red channel differs by 0.47; vs RED (1.00, 0.10,
	 *  0.05) the blue channel differs by 0.49 - that gap IS the difference between magenta and
	 *  red and it is the largest single-channel gap in the set; vs GOLD (1.00, 0.72, 0.18) the
	 *  green channel differs by 0.69; vs DEEP EMERALD (0.04, 0.22, 0.09) every channel differs.
	 *  ⚠️ HONEST LIMIT: this is a channel-distance check plus the luminance derivation above -
	 *  ⛔ it is NOT a colour-blind simulation, and a deuteranope may read magenta and red closer
	 *  together than the numbers suggest. The SHAPE tells carry it in that case: a mark is a
	 *  HOLLOW RING WITH A NUMBER IN IT, an enemy dot is a 6-px filled square that only exists
	 *  during a paid reveal. Flagged in the handoff rather than claimed as solved.
	 */
	static const FLinearColor MarkRingColor = FLinearColor(0.52f, 0.035f, 0.54f, 1.00f); // TRUE LINEAR (Slate tint) - magenta, the exact complement of the map's green; tuned to the 3.84:1 flat-colour ceiling so it clears 3:1 at BOTH ramp ends

	/**
	 *  Outline thickness for the centred number, in slate units (px at 1.0 font scale).
	 *
	 *  ⚠️ 2, ⛔ not 1: a 1-px outline on a 10-pt digit is eaten by antialiasing against the
	 *  ramp's light end, which is the one place the off-white fill has no contrast of its own.
	 */
	static constexpr int32 MarkNumberOutlineSizePx = 2;

	/** Extra ring thickness while a mark is hovered - the affordance that says "the wheel will resize THIS one". Cosmetic; no verb reads the hover state. */
	static constexpr float MarkHoverThicknessBoostPx = 2.f;

	/**
	 *  Ring tessellation bounds. Segments scale with radius so a 12-px circle costs 24 points
	 *  and a 240-px one costs 96 - ⛔ never a fixed count, which would either facet visibly at
	 *  the top of the range or waste points at the bottom. At the cap of nine marks the whole
	 *  layer is at most 9 × 2 rings × 97 points per frame, which is the same cost class as the
	 *  POI icon loop beside it.
	 */
	static constexpr int32 MarkRingSegmentsMin = 24;
	static constexpr int32 MarkRingSegmentsMax = 96;
	static constexpr float MarkRingSegmentsPerPx = 0.4f;

	/**
	 *  MARK CHROME. ⛔ The ONLY player-visible words TASK-745 authors, kept in this namespace
	 *  with the rest so the file's whole player-facing surface stays auditable in one place.
	 *  ⛔ PURE ASCII, the NoSnapshotStatusText rule (4): non-ASCII inside TEXT() compiles, but
	 *  these strings are RENDERED by Slate's default font and a glyph the font lacks is a
	 *  visible defect rather than a build one. ⛔ None of them is a prompt string; nothing in
	 *  this namespace is ever serialized into any zone (WR-§6), and Zone A stays byte-frozen at
	 *  5658 chars.
	 *
	 *  ⚠️ THEY ARE FUNCTIONS RATHER THAN `const TCHAR*` CONSTANTS, AND THAT IS A HARD LANGUAGE
	 *  CONSTRAINT, ⛔ NOT A STYLE CHOICE: `FString::Printf` takes a `UE::Core::
	 *  TCheckedFormatString`, whose only constructor is `TCheckedFormatStringPrivate(const
	 *  CharType (&Fmt)[N])` and is `consteval` under `UE_VALIDATE_FORMAT_STRINGS`
	 *  (`String/FormatStringSan.h:486-520`). ⇒ a `const TCHAR*` format ⛔ WILL NOT COMPILE, and
	 *  the format must be a literal ARRAY at the call site. Wrapping each line in a tiny builder
	 *  keeps the words here AND the literal where Printf needs it.
	 */

	/**
	 *  ⛔ THE PLACED LINE NAMES THE **NUMBER**, ⛔ NEVER THE SYMBOL (MARK-§2): the player sees
	 *  `1`, the model sees `circle_1`, and neither may be shown in the other's place. It also
	 *  teaches all three remaining verbs in one sentence, because a circle you cannot resize,
	 *  delete or name is a circle that looks broken.
	 */
	static FString MakeMarkPlacedStatusLine(int32 Number)
	{
		return FString::Printf(
			TEXT("Circle %d placed. Scroll on it to resize, right-click to delete, click it to name it to your commander."),
			Number);
	}

	/** ⛔ THE REFUSAL IS LOUD AND IT NAMES THE CAP (M-5, the shipped refusal doctrine) - ⛔ never a silent no-op. The count is read from the store at the moment of refusal, so it cannot disagree with MaxMapMarks. */
	static FString MakeMarkCapRefusedStatusLine(int32 ExistingCount)
	{
		return FString::Printf(
			TEXT("Map circle limit reached - you already have %d. Right-click one to delete it before placing another."),
			ExistingCount);
	}

	/** ⛔ THE DELETED LINE NAMES THE NUMBER THAT WENT, AND SAYS THE HOLE STAYS (M-1). A player who deletes 2 of 3 and sees 1 and 3 remain must be told that is deliberate, or he reports it as a renumbering bug - and the "fix" a future reader would reach for is the renumber that silently redirects an order already sitting unsent in his input box. */
	static FString MakeMarkDeletedStatusLine(int32 Number)
	{
		return FString::Printf(
			TEXT("Circle %d deleted. The other numbers do not change - your commander still knows them by the same names."),
			Number);
	}

	/**
	 *  ⛔ ONE WHITE BRUSH FOR EVERY QUAD, TINTED PER ELEMENT. FSlateColorBrush carries no
	 *  texture resource (Brushes/SlateColorBrush.h), so Slate batches these as flat colour -
	 *  there is no asset to resolve, nothing to load, and nothing to go missing when
	 *  WBP_WarMap does not exist yet.
	 *
	 *  A function-local static: initialised on first paint, never re-created, and never a
	 *  file-scope static whose construction order could matter.
	 */
	static const FSlateBrush& GetQuadBrush()
	{
		static const FSlateColorBrush QuadBrush(FLinearColor::White);
		return QuadBrush;
	}

	// ── The elevation background (TASK-684; WM-§2) ──────────────────────────────

	/**
	 *  ⭐⭐ THE ELEVATION RAMP'S TWO ENDS (TASK-722; WM-§8a/§8b). Dark green at brightness 0,
	 *  light green at 1. They REPLACE the shipped `ElevationFloorLuma = 0.10f` scalar, which
	 *  is retired rather than left behind: a dead luma constant would imply this ramp is
	 *  still gray. (The 0.10 is not lost - it is literally the factor ElevationGrassDark is
	 *  the chromaticity scaled by.)
	 *
	 *  ⚠️⚠️ COLOUR SPACE - THESE ARE **DISPLAY-ENCODED (sRGB) VALUES** IN AN `FLinearColor`
	 *  CONTAINER. ⛔⛔ THEY ARE NOT LINEAR, DESPITE THE TYPE'S NAME, AND THAT IS CORRECT.
	 *  ⛔ WHY, and why "fixing" it would ship a wrong-looking map that reviews as right:
	 *  these two colours never reach Slate. They are turned into the raw BYTES of a texture
	 *  created with `Texture->SRGB = true` (BakeElevationTexture, below), and THE BYTES OF AN
	 *  sRGB TEXTURE ARE sRGB-ENCODED VALUES. The scalar they replaced lived in exactly this
	 *  space for exactly this reason. ⇒ Typing the TRUE-LINEAR equivalents of these same two
	 *  colours - (0.00516, 0.01002, 0.00515) and (0.34934, 1.00000, 0.34770) - would drop the
	 *  light end from byte 255 to 160 and the dark end from 26 to 3: a visibly much darker
	 *  map, from literals that look perfectly plausible in a diff. ⛔ Do not convert these.
	 *  ⛔⛔ AND THE OVERLAY TINTS ABOVE (MarkerColor, MineIconTint, AncientGroundIconTint, the
	 *  team accessors) ARE THE OPPOSITE - TRUE LINEAR - because Slate sRGB-encodes a tint at
	 *  draw time. TWO SPACES, ONE TYPE, ONE FILE. The full argument is at the POI-tint block
	 *  above; ⛔ never copy a value between the two families.
	 *
	 *  ⭐ MEASURED, ⛔ NEVER PICKED BY EYE (WM-§8b). Sampled from
	 *  /Game/Materials/Instances/MI_BattlefieldGround - the material actually on ArenaGround,
	 *  ⛔ not a swatch - and cross-checked byte-identical against /Game/Materials/M_HillGrass,
	 *  the terrain at the ramp's HIGH end, so field and hills are the same green and there is
	 *  no seam. Method, sample counts and provenance: handoffs/TASK-721-artist.md §0-§1.
	 *  ⭐ BOTH ENDS ARE ONE COLOUR AT TWO LUMINANCES: the identical display-space chromaticity
	 *  C = (0.6257, 1.0000, 0.6243), scaled by 0.10 and by 1.00. Since
	 *  Lerp(C*0.10, C*1.00, B) = C * Lerp(0.10, 1.00, B), HUE AND SATURATION ARE CONSTANT AT
	 *  EVERY POINT ON THE RAMP BY CONSTRUCTION and only VALUE moves - which is the whole of
	 *  WM-§8b - while the luminance curve stays byte-identical to the gray ramp this replaced
	 *  (WM-§8a: the ramp's COLOUR changed, the relief's SHAPE did not).
	 *
	 *  ⛔ NOT pitch black at the dark end, for the reason the retired scalar gave: the flat
	 *  field floor must still read as "map" against the letterbox outside the rect, and
	 *  WM-§4 already darkens the WBP panel toward near-black - a 0.0 floor would make the two
	 *  indistinguishable. Widget CHROME in exactly the MarkerColor sense above: height is not
	 *  a team, so this is deliberately not the team palette.
	 *
	 *  As bytes, for the reader who wants them: dark (16, 26, 16), light (160, 255, 159).
	 */
	static const FLinearColor ElevationGrassDark  = FLinearColor(0.0626f, 0.1000f, 0.0624f, 1.00f);
	static const FLinearColor ElevationGrassLight = FLinearColor(0.6257f, 1.0000f, 0.6243f, 1.00f);

	/**
	 *  The trace bracket, ±uu around Z=0 — COPIED from the shipped ground-trace idiom
	 *  (ASiegeBattlefieldScatter::GroundZAt uses the identical ±50,000 bracket), not invented,
	 *  so the two lanes cannot disagree about what "the whole vertical field" means.
	 */
	static constexpr float ElevationTraceHalfHeightUu = 50000.f;

	/**
	 *  ⭐ THE SCATTER-GENERATION SENTINEL (TASK-684's staleness key 2 — the diagnosis is at
	 *  UWarMapWidget::EnsureElevationBake's declaration). Returns any scatter-spawned
	 *  PER-GENERATE actor: a mine first (the economy law guarantees the shipped field ships
	 *  them), else an ancient ground. ClearScatter DESTROYS both families and every generate
	 *  spawns fresh ones, so the returned actor's LIFETIME is a re-scatter detector.
	 *
	 *  ⛔⛔ IDENTITY ONLY. No GetActorLocation, no name, no state — nothing is read off the
	 *  actor, ever. This is NOT a place iterator (WR-§6: ResolvePlace stays the single owner
	 *  of place → position; this function answers "did the field re-roll", not "where is X"),
	 *  and nothing it touches can reach a prompt zone.
	 */
	static const AActor* FindScatterGenerationSentinel(UWorld& World)
	{
		for (TActorIterator<AGoldNode> It(&World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}

		for (TActorIterator<AAncientGround> It(&World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}

		return nullptr;
	}

	/** One tinted axis-aligned quad centred on `LocalCentre`. */
	static void PaintQuad(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& AllottedGeometry,
		const FVector2D& LocalCentre,
		float HalfSizePx,
		const FLinearColor& Tint)
	{
		const float Size = HalfSizePx * 2.f;
		const FVector2f Offset(
			static_cast<float>(LocalCentre.X) - HalfSizePx,
			static_cast<float>(LocalCentre.Y) - HalfSizePx);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			Layer,
			// ⚠️ FVector2f, NOT FVector2D, at every Slate boundary. UE 5.8's
			// FDeprecateVector2DParameter marks its double-precision constructors
			// UE_SLATE_VECTOR_DEPRECATED_DEFAULT (SlateVector2.h:499-510). The reporting
			// macro is off by default TODAY, so FVector2D would compile - and this module
			// builds warnings-as-errors, so the day it is switched on is the day every one
			// of these lines becomes a build failure. Float in, float out.
			AllottedGeometry.ToPaintGeometry(FVector2f(Size, Size), FSlateLayoutTransform(Offset)),
			&GetQuadBrush(),
			ESlateDrawEffect::None,
			Tint);
	}

	/**
	 *  One POI icon, centred on `LocalCentre`, tinted per element (TASK-685; WM-§1).
	 *
	 *  ⛔ NULL-SAFE BY THE TWO-NULL-PATHS LAW, AND THE DEGRADE IS DRAWN, NEVER SKIPPED: a
	 *  null brush (icon cleared by a designer, or set-but-unresolvable - the caller logged
	 *  that once at resolve) falls back to the SHIPPED dot primitive in the same tint at the
	 *  same footprint, so the POI is still marked and the layout reads identically either
	 *  way. Never a crash, never silence, never an invisible POI.
	 *
	 *  ⛔ No defaulted parameter (SC-§33).
	 */
	static void PaintPoiIcon(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& AllottedGeometry,
		const FVector2D& LocalCentre,
		float HalfSizePx,
		const FSlateBrush* IconBrush,
		const FLinearColor& Tint)
	{
		if (IconBrush == nullptr)
		{
			PaintQuad(OutDrawElements, Layer, AllottedGeometry, LocalCentre, HalfSizePx, Tint);
			return;
		}

		const float Size = HalfSizePx * 2.f;
		const FVector2f Offset(
			static_cast<float>(LocalCentre.X) - HalfSizePx,
			static_cast<float>(LocalCentre.Y) - HalfSizePx);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			Layer,
			// FVector2f at the Slate boundary - the standing PaintQuad discipline above.
			AllottedGeometry.ToPaintGeometry(FVector2f(Size, Size), FSlateLayoutTransform(Offset)),
			IconBrush,
			ESlateDrawEffect::None,
			Tint);
	}

	/**
	 *  ONE HOLLOW RING centred on `LocalCentre` (TASK-745).
	 *
	 *  ⭐⛔ HOLLOW IS THE DESIGN, ⛔ NOT A SHORTCUT, AND IT IS ONE OF THE THREE TELLS THAT
	 *  SEPARATE A MAP MARK FROM A GROUP-ORDER PICK ZONE (WarMapWidget.h §8(d)): the shipped
	 *  order zones are FILLED world-space ground decals that ISSUE AN ORDER; a mark is a
	 *  STROKED map-space ring that issues nothing and merely names ground. A filled disc would
	 *  additionally hide the elevation, the icons and the dots underneath it - which is the
	 *  content the player opened the map to read.
	 *
	 *  ⚠️ MakeLines takes its points in the PAINT GEOMETRY's local space, so this uses the
	 *  IDENTITY ToPaintGeometry() and builds absolute widget-local points - ⛔ unlike PaintQuad
	 *  above, which offsets the geometry instead. Both are correct; they are different Slate
	 *  primitives with different conventions, and mixing them is how a layer ends up drawn at
	 *  double its offset.
	 *
	 *  ⚠️ FVector2f AT THE SLATE BOUNDARY - the standing PaintQuad discipline, and MakeLines'
	 *  FVector2f overload takes its array BY VALUE, so the local array is handed over directly.
	 *
	 *  ⛔ Degenerate radii draw NOTHING rather than a dot or a NaN-shaped smear: the
	 *  ComputeMapRectLocal fail-closed direction, applied to a stroke. ⛔ No defaulted
	 *  parameter (SC-§33).
	 */
	static void PaintCircleRing(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& AllottedGeometry,
		const FVector2D& LocalCentre,
		float RadiusPx,
		float ThicknessPx,
		const FLinearColor& Tint)
	{
		if (!(RadiusPx > 0.f) || !(ThicknessPx > 0.f))
		{
			return;
		}

		// Segments scale with the drawn size (see MarkRingSegmentsPerPx): smooth at 240 px,
		// cheap at 12 px, and never a fixed count that is wrong at one end of the range.
		const int32 Segments = FMath::Clamp(
			FMath::CeilToInt(RadiusPx * MarkRingSegmentsPerPx),
			MarkRingSegmentsMin,
			MarkRingSegmentsMax);

		TArray<FVector2f> Points;
		Points.Reserve(Segments + 1);

		const float CentreX = static_cast<float>(LocalCentre.X);
		const float CentreY = static_cast<float>(LocalCentre.Y);

		// ⚠️ Segments + 1 POINTS, NOT Segments: MakeLines JOINS consecutive points and does not
		// close the loop itself, so the first point is repeated last. Omitting it leaves a
		// visible notch in every ring - one wrong `<` and the circle has a bite taken out of it.
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = (2.f * UE_PI * static_cast<float>(Index)) / static_cast<float>(Segments);
			Points.Emplace(
				CentreX + RadiusPx * FMath::Cos(Angle),
				CentreY + RadiusPx * FMath::Sin(Angle));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			Layer,
			AllottedGeometry.ToPaintGeometry(),
			MoveTemp(Points),
			ESlateDrawEffect::None,
			Tint,
			/*bAntialias=*/ true,
			ThicknessPx);
	}

	/**
	 *  Resolves ONE icon soft ref into its hard pointer + brush (TASK-685). A plain static
	 *  taking the members by reference - no `this`, no state of its own, three call sites.
	 *
	 *  THE TWO NULL PATHS, split exactly as ACastle::TorchClassAsset splits them:
	 *    • already resolved          ⇒ false (never re-loaded - resolve is once per match at most);
	 *    • CLEARED (IsNull)          ⇒ false, silently - the designer opt-out;
	 *    • SET but LoadSynchronous
	 *      returns null              ⇒ TRUE - the caller aggregates and logs ONCE.
	 *
	 *  @return true exactly when the ref is SET but UNRESOLVABLE (the loggable degrade).
	 */
	static bool ResolvePoiIconTexture(
		const TSoftObjectPtr<UTexture2D>& IconAsset,
		TObjectPtr<UTexture2D>& InOutResolvedTexture,
		FSlateBrush& InOutBrush)
	{
		if (InOutResolvedTexture != nullptr)
		{
			return false;
		}

		if (IconAsset.IsNull())
		{
			return false;
		}

		UTexture2D* const Loaded = IconAsset.LoadSynchronous();
		if (Loaded == nullptr)
		{
			return true;
		}

		// Publish: the Transient UPROPERTY owns the GC reference; the brush only mirrors it
		// (the ElevationTexture/ElevationBrush idiom, verbatim). Tint stays white ON THE
		// BRUSH - the per-element tint at draw is the one and only colorist (WM-§1).
		InOutResolvedTexture = Loaded;
		InOutBrush.SetResourceObject(Loaded);
		InOutBrush.ImageSize = FVector2f(
			static_cast<float>(Loaded->GetSizeX()),
			static_cast<float>(Loaded->GetSizeY()));
		InOutBrush.DrawAs = ESlateBrushDrawType::Image;
		InOutBrush.TintColor = FSlateColor(FLinearColor::White);
		return false;
	}
}

// ---------------------------------------------------------------------------
// FSiegeWarMapProjection - PURE. No world, no actor, no state (WR-§6).
// ---------------------------------------------------------------------------

FVector2D FSiegeWarMapProjection::WorldToMapUV(const FVector2D& WorldXY, const FVector2D& ArenaHalfExtent)
{
	// ⛔ THE ZERO-DIVIDE FLOOR IS APPLIED BEFORE ANY DIVISION, NOT AFTER. A configured
	// (0,0) extent is not hypothetical - ArenaHalfExtent is an EditAnywhere field on a
	// DataAsset a designer can clear - and an unguarded divide would put NaN into every
	// marker rect, which propagates into the hit test and makes every click miss.
	const double HalfX = FMath::Max(ArenaHalfExtent.X, static_cast<double>(MinArenaHalfExtentUu));
	const double HalfY = FMath::Max(ArenaHalfExtent.Y, static_cast<double>(MinArenaHalfExtentUu));

	FVector2D MapUV;

	// +X world grows RIGHT.
	MapUV.X = FMath::Clamp((WorldXY.X + HalfX) / (2.0 * HalfX), 0.0, 1.0);

	// +Y world grows DOWN (TASK-692, WM-§7): UE's world frame is left-handed (X forward,
	// Y RIGHT, Z up), so from a bird's eye with +X drawn to the right, +Y physically lies
	// toward the map's BOTTOM — the same direction Slate's local Y already grows. ⛔ NO
	// inversion: the pre-692 (HalfY - Y) flip here is what mirrored the whole field about
	// the castle lane (Jonathan's first map test). The convention is decided here and
	// NOWHERE ELSE — the ally dots, the enemy dots, the icons, the markers and the
	// elevation bake all route through this pair and cannot disagree about it.
	MapUV.Y = FMath::Clamp((WorldXY.Y + HalfY) / (2.0 * HalfY), 0.0, 1.0);

	return MapUV;
}

void FSiegeWarMapProjection::ComputeMapRectLocal(
	const FVector2D& PanelLocalSize,
	float PaddingPx,
	const FVector2D& ArenaHalfExtent,
	FVector2D& OutRectOrigin,
	FVector2D& OutRectSize)
{
	const double Padding = FMath::Max(static_cast<double>(PaddingPx), 0.0);
	const FVector2D Inner(PanelLocalSize.X - 2.0 * Padding, PanelLocalSize.Y - 2.0 * Padding);

	if (Inner.X <= 0.0 || Inner.Y <= 0.0)
	{
		// ⛔ FAIL CLOSED, NEVER NEGATIVE. A negative size would invert every hit rect and
		// make FindMarkerIndexAtLocal answer for points nowhere near a marker.
		OutRectOrigin = PanelLocalSize * 0.5;
		OutRectSize = FVector2D::ZeroVector;
		return;
	}

	const double HalfX = FMath::Max(ArenaHalfExtent.X, static_cast<double>(MinArenaHalfExtentUu));
	const double HalfY = FMath::Max(ArenaHalfExtent.Y, static_cast<double>(MinArenaHalfExtentUu));

	// Aspect of the ARENA (width / height), which the drawn rect must match exactly.
	const double ArenaAspect = HalfX / HalfY;
	const double InnerAspect = Inner.X / Inner.Y;

	if (InnerAspect > ArenaAspect)
	{
		// The panel is wider than the arena ⇒ height is the binding constraint.
		OutRectSize.Y = Inner.Y;
		OutRectSize.X = Inner.Y * ArenaAspect;
	}
	else
	{
		OutRectSize.X = Inner.X;
		OutRectSize.Y = Inner.X / ArenaAspect;
	}

	OutRectOrigin = FVector2D(
		Padding + (Inner.X - OutRectSize.X) * 0.5,
		Padding + (Inner.Y - OutRectSize.Y) * 0.5);
}

FVector2D FSiegeWarMapProjection::MapUVToLocal(const FVector2D& MapUV, const FVector2D& RectOrigin, const FVector2D& RectSize)
{
	return FVector2D(
		RectOrigin.X + MapUV.X * RectSize.X,
		RectOrigin.Y + MapUV.Y * RectSize.Y);
}

FVector2D FSiegeWarMapProjection::MapUVToWorld(const FVector2D& MapUV, const FVector2D& ArenaHalfExtent)
{
	// The SAME floor as WorldToMapUV, applied to the SAME axes, so the two directions are
	// exact inverses even for a degenerate configured extent (no NaN, no zero product drift).
	const double HalfX = FMath::Max(ArenaHalfExtent.X, static_cast<double>(MinArenaHalfExtentUu));
	const double HalfY = FMath::Max(ArenaHalfExtent.Y, static_cast<double>(MinArenaHalfExtentUu));

	// Solve WorldToMapUV's two lines for the world coordinate — the EXACT inverse,
	// re-derived for the TASK-692 convention (WM-§7):
	//   U = (X + HalfX) / (2·HalfX)  ⇒  X = (2U − 1)·HalfX
	//   V = (Y + HalfY) / (2·HalfY)  ⇒  Y = (2V − 1)·HalfY
	// Both axes are now the SAME affine form (world +Y grows DOWN on screen, no inversion
	// anywhere) — the orientation contract still has a single owner, readable in both
	// directions, and the round trip stays byte-exact on (0,1)².
	return FVector2D(
		(2.0 * MapUV.X - 1.0) * HalfX,
		(2.0 * MapUV.Y - 1.0) * HalfY);
}

int32 FSiegeWarMapProjection::FindMarkerIndexAtLocal(const TArray<FSiegeWarMapMarker>& Markers, const FVector2D& LocalPoint)
{
	// ⚠️ BACKWARDS: markers are painted in vocabulary order, so the LAST entry is the one
	// drawn ON TOP. Scanning forwards would let a click land on a marker the player cannot
	// see and hand them the wrong symbol - a silent wrong answer, which is the one failure
	// class this whole feature is shaped to avoid.
	for (int32 Index = Markers.Num() - 1; Index >= 0; --Index)
	{
		const FSiegeWarMapMarker& Marker = Markers[Index];

		// Boundary INCLUSIVE (<=), copied from the shipped IsPointInZone idiom rather than
		// re-decided, so "on the edge" means the same thing everywhere in this codebase.
		if (FMath::Abs(LocalPoint.X - Marker.LocalCentre.X) <= Marker.LocalHitHalfSize.X
			&& FMath::Abs(LocalPoint.Y - Marker.LocalCentre.Y) <= Marker.LocalHitHalfSize.Y)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

// ---------------------------------------------------------------------------
// ═══ MAP MARK GEOMETRY (TASK-745; MARK-§) - still PURE ═══
// ⛔ No world, no actor, no UObject, no state. The whole reason this block lives
// on FSiegeWarMapProjection rather than inside the widget is that the widget's
// mark handlers must be able to LOOK UP and ACT without DECIDING - so every rule
// that can be stated as arithmetic is assertable headlessly (W4-R1).
// ---------------------------------------------------------------------------

bool FSiegeWarMapProjection::LocalToMapUV(
	const FVector2D& LocalPoint,
	const FVector2D& RectOrigin,
	const FVector2D& RectSize,
	FVector2D& OutMapUV)
{
	// ⛔ THE DEGENERATE RECT IS REJECTED BEFORE ANY DIVISION, not after - the
	// MinArenaHalfExtentUu doctrine. ComputeMapRectLocal returns a ZERO size for a panel
	// smaller than twice the padding, and that value reaches here on the very first frame of a
	// map that has not been laid out yet.
	if (!(RectSize.X > 0.0) || !(RectSize.Y > 0.0))
	{
		return false;
	}

	// ⛔⛔ THE CONTAINMENT TEST IS ON THE **RECT**, ⛔ NOT ON THE NORMALISED VALUE, AND THAT IS A
	// CORRECTNESS CHOICE RATHER THAN A STYLE ONE. `(Origin + UV·Size) - Origin` is ⛔ NOT exactly
	// `UV·Size` in floating point, so a point sitting EXACTLY on the rect's right or bottom edge
	// - which is what MapUVToLocal(1,1) produces, and what a click on the map's last pixel column
	// is - can normalise to 1.0000000000000002 and be REFUSED by a naive `U > 1.0`. ⇒ testing the
	// geometry directly makes "inside the drawn map" mean exactly what it says, at the boundary
	// too, and the round trip stays exact at the corners (pinned by
	// `Siegebound.WarMap.LocalToMapUVInvertsTheRectAndRefusesOutsideIt`).
	//
	// ⛔ REFUSE, ⛔ DO NOT CLAMP. The declaration carries the argument: the forward map clamps
	// because out-of-arena ACTORS are real and must pin visibly to the rim, while an
	// out-of-rect CLICK is not a battlefield position at all - it is the letterbox, or the
	// padding, or the WBP's chrome. Clamping it would silently place a mark on ground the
	// player did not point at, and ResolvePlace would then answer an order with that ground.
	//
	// ⚠️ BOUNDARY INCLUSIVE (<=), the shipped IsPointInZone / FindMarkerIndexAtLocal idiom - so
	// the map's outermost pixel row and column are map, not chrome.
	if (LocalPoint.X < RectOrigin.X || LocalPoint.X > RectOrigin.X + RectSize.X
		|| LocalPoint.Y < RectOrigin.Y || LocalPoint.Y > RectOrigin.Y + RectSize.Y)
	{
		return false;
	}

	const double U = (LocalPoint.X - RectOrigin.X) / RectSize.X;
	const double V = (LocalPoint.Y - RectOrigin.Y) / RectSize.Y;

	// ⚠️⚠️ THIS CLAMP IS ⛔ NOT THE ONE THE PARAGRAPH ABOVE FORBIDS, AND THE DIFFERENCE MATTERS:
	// the point has ALREADY been proven inside the rect, so this removes a one-ulp overshoot from
	// the division and nothing else. The forbidden clamp is the one that would ACCEPT an
	// out-of-rect click and pin it to the rim; this one changes no click's meaning by more than a
	// floating-point epsilon, and it guarantees callers a UV that is genuinely in [0,1] - which
	// MapUVToWorld, by its own contract, does not range-check.
	OutMapUV = FVector2D(FMath::Clamp(U, 0.0, 1.0), FMath::Clamp(V, 0.0, 1.0));
	return true;
}

float FSiegeWarMapProjection::MapWorldRadiusToLocalPx(
	float RadiusUU,
	const FVector2D& ArenaHalfExtent,
	const FVector2D& RectSize)
{
	if (!(RadiusUU > 0.f) || !(RectSize.X > 0.0))
	{
		// Fail closed: a zero-radius circle draws nothing and hit-tests nothing. ⛔ Never
		// negative - a negative radius would make the radial hit test answer for every point
		// on the panel at once (its `<=` would compare against a negative bound and always
		// fail, which is the SAFE half; but a negative THICKNESS reaches Slate, which is not).
		return 0.f;
	}

	// ⭐ ONE SCALAR, AND IT IS EXACT ON BOTH AXES BY CONSTRUCTION - the declaration's proof:
	// ComputeMapRectLocal builds a rect of exactly the arena's aspect, so
	// RectSize.X/(2·HalfX) == RectSize.Y/(2·HalfY). The X axis is read because it is the arena's
	// LONG axis (26,000 of the shipped 26,000 x 12,000), i.e. the one with the most precision
	// to give. `Siegebound.WarMap.MarkRadiusScaleIsUniformOnBothAxes` asserts the equality, so
	// an edit that broke the aspect preservation is caught here rather than shipping oval marks.
	const double HalfX = FMath::Max(ArenaHalfExtent.X, static_cast<double>(MinArenaHalfExtentUu));
	const double PxPerUu = RectSize.X / (2.0 * HalfX);

	return static_cast<float>(static_cast<double>(RadiusUU) * PxPerUu);
}

float FSiegeWarMapProjection::MapLocalPxToWorldRadius(
	float RadiusPx,
	const FVector2D& ArenaHalfExtent,
	const FVector2D& RectSize)
{
	if (!(RadiusPx > 0.f) || !(RectSize.X > 0.0))
	{
		return 0.f;
	}

	// The EXACT inverse of the line above, same floor, same axis - so the wheel's px → uu →
	// px round trip is stable and a mark does not creep in size across repeated notches.
	const double HalfX = FMath::Max(ArenaHalfExtent.X, static_cast<double>(MinArenaHalfExtentUu));
	const double UuPerPx = (2.0 * HalfX) / RectSize.X;

	return static_cast<float>(static_cast<double>(RadiusPx) * UuPerPx);
}

int32 FSiegeWarMapProjection::FindMarkIndexAtLocal(
	const TArray<FSiegeWarMapMarkCircle>& Circles,
	const FVector2D& LocalPoint)
{
	// ⚠️ BACKWARDS, for the identical reason FindMarkerIndexAtLocal scans backwards: circles
	// are painted in store order, so the LAST entry is the one drawn ON TOP. Scanning forwards
	// would let the player right-click a circle he cannot see and delete it - a silent wrong
	// answer, which is the one failure class this whole feature is shaped to avoid.
	for (int32 Index = Circles.Num() - 1; Index >= 0; --Index)
	{
		const FSiegeWarMapMarkCircle& Circle = Circles[Index];

		if (!(Circle.LocalRadiusPx > 0.f))
		{
			// A degenerate circle is unhittable rather than universally hittable - fail closed.
			continue;
		}

		// ⛔ SQUARED DISTANCE, ⛔ never a Sqrt: same answer, no root, and no chance of a
		// near-boundary float wobble that would make the edge hit-test disagree with itself
		// between two frames.
		const double DeltaX = LocalPoint.X - Circle.LocalCentre.X;
		const double DeltaY = LocalPoint.Y - Circle.LocalCentre.Y;
		const double RadiusSq = static_cast<double>(Circle.LocalRadiusPx) * static_cast<double>(Circle.LocalRadiusPx);

		// Boundary INCLUSIVE (<=) - the shipped IsPointInZone / FindMarkerIndexAtLocal idiom,
		// copied rather than re-decided so "on the edge" means the same thing everywhere.
		if (DeltaX * DeltaX + DeltaY * DeltaY <= RadiusSq)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

float FSiegeWarMapProjection::StepMarkRadiusPx(
	float CurrentRadiusPx,
	float WheelDelta,
	float StepPx,
	float MinPx,
	float MaxPx)
{
	// ⛔ A ZERO DELTA CHANGES NOTHING, AND SO DOES A NON-POSITIVE STEP. This is half of "the
	// wheel does nothing" (the other half is FindMarkIndexAtLocal returning INDEX_NONE), and
	// it is checked FIRST so a degenerate tunable can never silently snap a mark to a clamp.
	if (WheelDelta == 0.f || !(StepPx > 0.f))
	{
		return CurrentRadiusPx;
	}

	// Degenerate bounds collapse to a fixed size rather than producing a negative radius: a
	// negative radius would invert every hit test that consumes it.
	const float SafeMin = FMath::Max(MinPx, 1.f);
	const float SafeMax = FMath::Max(MaxPx, SafeMin);

	// ⭐ THE **SIGN**, ⛔ NOT THE MAGNITUDE. FPointerEvent::GetWheelDelta() is
	// platform/driver-dependent in magnitude - a free-spinning wheel or a trackpad can deliver
	// fractional or multi-unit deltas - so one EVENT is one STEP in the delta's direction and
	// the felt speed is MarkWheelStepPx, a designer number, never the mouse driver's.
	const float Signed = (WheelDelta > 0.f) ? StepPx : -StepPx;

	return FMath::Clamp(CurrentRadiusPx + Signed, SafeMin, SafeMax);
}

float FSiegeWarMapProjection::MarkNumberFontSizePx(float RadiusPx, float RadiusFraction, float MinSize, float MaxSize)
{
	// Degenerate tunables collapse to a fixed READABLE size rather than to nothing: a zeroed
	// or inverted pair must never produce a 0-pt font, which renders as an absent number - i.e.
	// a circle with no name, which is the one thing this feature cannot ship.
	const float SafeMin = FMath::Max(MinSize, 1.f);
	const float SafeMax = FMath::Max(MaxSize, SafeMin);

	if (!(RadiusPx > 0.f) || !(RadiusFraction > 0.f))
	{
		return SafeMin;
	}

	// ⭐ THE SIZE FOLLOWS THE RING, CLAMPED AT BOTH ENDS - the HeightToBrightness relationship,
	// verbatim: the clamp is the NORMALIZATION LAW rather than a safety net. Floor ⇒ a circle
	// scrolled to its minimum still carries a readable digit; ceiling ⇒ one scrolled to its
	// maximum carries a digit instead of a billboard.
	return FMath::Clamp(RadiusPx * RadiusFraction, SafeMin, SafeMax);
}

FVector2D FSiegeWarMapProjection::CentreTextTopLeft(const FVector2D& Centre, const FVector2D& MeasuredTextSize)
{
	// Jonathan's contract - "it gets its own number in the middle of it" - as arithmetic, so a
	// test can read it (W4-R1). ⚠️ A NEGATIVE measured size would push the glyph the wrong way;
	// the measure service never returns one, but the max costs nothing and makes the function
	// total for a hand-built test input.
	return FVector2D(
		Centre.X - FMath::Max(MeasuredTextSize.X, 0.0) * 0.5,
		Centre.Y - FMath::Max(MeasuredTextSize.Y, 0.0) * 0.5);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

UWarMapWidget* UWarMapWidget::CreateAndAddToViewport(
	APlayerController* OwningController,
	TSubclassOf<UWarMapWidget> MapClass,
	int32 ZOrder)
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogSiegeWarMap, Warning,
			TEXT("[WarMap] CreateAndAddToViewport: no owning player controller - no map was created. Never fatal: the map is an advantage, never a requirement (WR-§5)."));
		return nullptr;
	}

	// ⚠️ `.Get()` ON BOTH ARMS IS LOAD-BEARING, NOT TIDYING - it is the fix for C2445, and it
	// is carried verbatim from USiegeAssistantConsoleWidget::CreateAndAddToViewport, which
	// paid for the diagnosis. TSubclassOf carries BOTH a non-explicit TSubclassOf(UClass*)
	// constructor AND a non-explicit operator UClass*(), so a conditional whose arms are
	// TSubclassOf<T> and UClass* has two equally good common types and the compiler must
	// refuse to choose. Collapsing both arms to UClass* removes the choice.
	const TSubclassOf<UWarMapWidget> ResolvedClass =
		MapClass ? MapClass.Get() : UWarMapWidget::StaticClass();

	UWarMapWidget* Map = CreateWidget<UWarMapWidget>(OwningController, ResolvedClass);

	if (Map == nullptr)
	{
		UE_LOG(LogSiegeWarMap, Warning,
			TEXT("[WarMap] CreateAndAddToViewport: CreateWidget returned null for class '%s' - no map. Never fatal."),
			*GetNameSafe(ResolvedClass));
		return nullptr;
	}

	// Added CLOSED - NativeConstruct collapses it - so nothing appears on screen and nothing
	// becomes hit-testable until OpenMap().
	Map->AddToViewport(ZOrder);

	UE_LOG(LogSiegeWarMap, Log,
		TEXT("[WarMap] Created (class '%s', ZOrder %d), closed. %s"),
		*GetNameSafe(ResolvedClass), ZOrder,
		// ⚠️ `.Get()` AGAIN, AND FOR THE SAME REASON AS ABOVE. Comparing a TSubclassOf<T>
		// directly against a UClass* re-opens the two-viable-conversions ambiguity the block
		// above documents; taking the raw pointer on both sides leaves nothing to choose.
		(ResolvedClass.Get() == UWarMapWidget::StaticClass())
			? TEXT("No WBP - the C++ painter still draws and hit-tests every marker and dot (TASK-568 supplies /Game/UI/WBP_WarMap).")
			: TEXT(""));

	return Map;
}

UWarMapWidget::UWarMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ⛔ THE CONSTRUCTOR, NOT NativeConstruct - see the declaration comment. Assigning this in
	// NativeConstruct would stomp a designer's WBP_WarMap override on every instance and make
	// an EditDefaultsOnly property inert while still looking editable.
	ArenaConfigAsset = TSoftObjectPtr<USiegeScatterConfig>(
		FSoftObjectPath(TEXT("/Game/Data/DA_BattlefieldScatter.DA_BattlefieldScatter")));

	// ⭐ TASK-685 (WM-§1): the three POI icon soft defaults - the exact TASK-683 asset paths
	// (composed object-path form, the DA_BattlefieldScatter idiom above). Same
	// constructor-not-NativeConstruct reasoning; the FSoftObjectPath argument is
	// BRACE-initialised per the MOST-VEXING-PARSE law (TASK-416). ⛔ Soft on purpose: the
	// code lands with or without the textures (the two-null-paths degrade at
	// ResolvePoiIconTexture), so this file never blocks on the art lane.
	MineIconTexture = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath{ TEXT("/Game/UI/WarMap/T_WarMap_Icon_Mine.T_WarMap_Icon_Mine") });
	AncientGroundIconTexture = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath{ TEXT("/Game/UI/WarMap/T_WarMap_Icon_AncientGround.T_WarMap_Icon_AncientGround") });
	CastleIconTexture = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath{ TEXT("/Game/UI/WarMap/T_WarMap_Icon_Castle.T_WarMap_Icon_Castle") });
}

void UWarMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// AddUniqueDynamic keeps repeated construct cycles single-bound - the shipped
	// USessionMenuWidget contract, whose optional-child shape this class clones.
	if (RevealButton)
	{
		RevealButton->OnClicked.AddUniqueDynamic(this, &UWarMapWidget::HandleRevealButtonClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWarMapWidget::HandleCloseButtonClicked);
	}

	// ⛔ CONSTRUCTED CLOSED, ALWAYS, and it is the map's own root visibility that says so -
	// the USiegeAssistantConsoleWidget precedent (that widget drives its own root the same
	// way). Nothing is on screen and nothing is hit-testable until OpenMap().
	bMapOpen = false;
	EnemyDotsWorldXY.Reset();
	AllyDotsWorldXY.Reset();
	// TASK-685: the POI census starts empty too - OpenMap seeds it before the first painted
	// frame, so construction-time content would only ever be stale.
	MinePoiWorldXY.Reset();
	AncientGroundPoiWorldXY.Reset();
	BlueCastlePoiWorldXY.Reset();
	RedCastlePoiWorldXY.Reset();
	// TASK-745: the hover HIGHLIGHT starts clear (the cursor has not moved over this instance
	// yet). ⛔ The MARKS are not touched - they live in the local player's subsystem and outlive
	// every widget construct/destruct cycle by design (M-4).
	HoveredMarkNumber = 0;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UWarMapWidget::NativeDestruct()
{
	// ⛔ THE TIMER IS CLEARED TWICE ON PURPOSE - here and in CloseMap. A widget torn down
	// while open (level travel, Play Again, viewport teardown) never reaches CloseMap, and a
	// surviving repeating timer on a dead widget is the leak class WR-§4 spends a whole row
	// on for the torches.
	SetAllyRefreshTimerEnabled(false);

	if (RevealButton)
	{
		RevealButton->OnClicked.RemoveDynamic(this, &UWarMapWidget::HandleRevealButtonClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UWarMapWidget::HandleCloseButtonClicked);
	}

	// ⛔ The reveal never survives a teardown either (WR-§7: no persistence across opens and
	// none across Play Again / match reset).
	EnemyDotsWorldXY.Reset();
	AllyDotsWorldXY.Reset();
	MinePoiWorldXY.Reset();
	AncientGroundPoiWorldXY.Reset();
	BlueCastlePoiWorldXY.Reset();
	RedCastlePoiWorldXY.Reset();
	// TASK-745: the highlight, ⛔ never the marks (M-4 - the store outlives this widget on
	// purpose, so a torn-down and rebuilt map still shows the plan the player drew).
	HoveredMarkNumber = 0;

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

void UWarMapWidget::OpenMap()
{
	if (bMapOpen)
	{
		return;
	}

	bMapOpen = true;

	// ⚠️ Visible, NOT SelfHitTestInvisible. See the declaration comment: the one-word
	// difference is the difference between a map you can click and a map that only looks
	// clickable.
	SetVisibility(ESlateVisibility::Visible);

	// ⛔ NO PAUSE, NO TIME DILATION, NO INPUT-MODE CHANGE HAPPENS HERE (WR-§6, WR-§9
	// outcome 3). The match keeps running while the map is up - that is what makes reading
	// it a real tactical cost - and the cursor/posture owner is the controller (TASK-563).

	// ⭐ TASK-684 (WM-§2) - THE LAZY ELEVATION BAKE. First open of a match traces the field
	// ONCE (7,800 rays, a one-shot sub-frame cost class); every later open, and every paint,
	// reuses the cached texture at pointer-check cost. A failed bake keeps the shipped flat
	// background - the map is degraded, never broken. ⛔ Nothing on this path reads, creates
	// or reaches a snapshot: the airlock below is untouched by the bake.
	bElevationBakeFailedThisOpen = false;
	EnsureElevationBake();

	// ⭐ TASK-685 (WM-§1) - resolve the POI icon textures lazily on the open path (never at
	// paint: LoadSynchronous is a load, and the painter is const). Idempotent after the
	// first successful open; an unresolved ref degrades to the tinted dot primitive at
	// paint. ⛔ Nothing on this path reads, creates or reaches a snapshot either.
	ResolvePoiIconTextures();

	// Seed immediately so the map is never blank for up to one refresh period, THEN start
	// the interval. The POI census (TASK-685) seeds with the dots for the same reason: the
	// first painted frame already carries mines, grounds and castles.
	RefreshAllyDots();
	RefreshPoiIcons();
	SetAllyRefreshTimerEnabled(true);

	// ⭐ TASK-579's FIRST-OPEN STATUS LINE, RE-POINTED BY TASK-685 (ruling W691-3 - the
	// chosen defensive option; WR-§9 row 12's corrected mechanism).
	//
	// ⛔ THE DISCRIMINATOR IS NOW "DID ANY PLACE RESOLVE", ⛔ NOT "does a snapshot object
	// exist" - because TASK-691 proved the object exists from match start (BeginPlay's
	// EnsureSnapshot(), since cd5f4ed) while listing ZERO places until the first real
	// Capture(). The old pointer test sent every open down the with-snapshot arm, so "War
	// map" rendered over a marker-less map - VID-003's confusing chrome, and the exact
	// sentence W691-3 forbids the map to repeat. GetPlaceNames() is the very list
	// BuildMarkerRects consumes, so the status line and the marker layer can no longer
	// disagree: no place listed ⇒ the honest line names the cause (nothing surveyed yet)
	// and the remedy (one console order). A null snapshot short-circuits into the SAME arm
	// - the null-guard stays, as defensive code (W691-3's own clause).
	// ✅ Correct with or without TASK-580 landed: pre-seed, the honest line shows (true
	// today); post-seed, the at-rest capture lists places and the line stays away.
	//
	// ⛔ GetReadOnlySnapshot() READS. It does not Capture(), does not EnsureSnapshot() and does
	// not reach anything that does - WR-§6's snapshot-hazard row, and the property TASK-565
	// re-greps. Opening this panel still surveys NOTHING; Num() on a const array reads only.
	{
		const USiegeAssistantSnapshot* const StatusSnapshot = GetReadOnlySnapshot();
		if (StatusSnapshot == nullptr || StatusSnapshot->GetPlaceNames().Num() == 0)
		{
			ShowNoSnapshotHint();
		}
		else
		{
			SetStatusLine(SiegeWarMap::OpenStatusText);
		}
	}

	OnWarMapOpenStateChanged(bMapOpen);
	OnMapOpenChanged.Broadcast(bMapOpen);
}

void UWarMapWidget::CloseMap()
{
	if (!bMapOpen)
	{
		return;
	}

	bMapOpen = false;

	SetVisibility(ESlateVisibility::Collapsed);
	SetAllyRefreshTimerEnabled(false);
	AllyDotsWorldXY.Reset();

	// TASK-685: the POI census dies with the open - the next OpenMap re-seeds before its
	// first painted frame, so nothing stale can survive a close, and nothing iterates while
	// the map is closed (the AllyDotsWorldXY lifecycle, verbatim).
	MinePoiWorldXY.Reset();
	AncientGroundPoiWorldXY.Reset();
	BlueCastlePoiWorldXY.Reset();
	RedCastlePoiWorldXY.Reset();

	// ⭐ TASK-745 - ONLY THE **HIGHLIGHT** DIES WITH THE OPEN. The cursor is no longer over
	// anything, so a surviving hover would highlight a circle on the next open before the first
	// mouse move.
	//
	// ⛔⛔ AND THE MARKS THEMSELVES ARE **NOT** CLEARED HERE - ⛔ THIS OMISSION IS THE FEATURE,
	// NOT AN OVERSIGHT (MARK-§ M-4). ⛔ DO NOT COPY THE ENEMY-REVEAL RULE BELOW ONTO THEM: that
	// rule governs PURCHASED enemy intel and its whole point is that the snapshot goes stale.
	// A mark is the player's own note about his own ground and does not decay - he opens the
	// map, draws his plan, closes it, plays, and reopens to the SAME plan. Clearing on match
	// reset is the STORE's job (USiegeMapMarkSubsystem::ClearMarks), ⛔ never this widget's.
	HoveredMarkNumber = 0;

	// ⛔⛔ ALWAYS, UNCONDITIONALLY, AND ⛔ NOT BEHIND ANY FLAG (WR-§7 / WR-§9 outcome 4).
	// Closing the map DISCARDS the paid reveal; re-opening shows nothing until the player
	// pays again, even one second after paying. That is Jonathan's mechanic stated in his
	// own words - "the enemy locations go away as soon as you close the map and you have to
	// pay another 30 gold" - and a "keep it if it was recent" kindness here would delete it.
	ClearEnemyReveal();

	OnWarMapOpenStateChanged(bMapOpen);
	OnMapOpenChanged.Broadcast(bMapOpen);
}

void UWarMapWidget::ToggleMap()
{
	if (bMapOpen)
	{
		CloseMap();
	}
	else
	{
		OpenMap();
	}
}

// ---------------------------------------------------------------------------
// The enemy reveal - this widget is the SINK, never the source (WR-§7)
// ---------------------------------------------------------------------------

void UWarMapWidget::ReceiveEnemyReveal(const TArray<FVector2D>& EnemyWorldXY)
{
	// ⛔ REPLACE, NEVER APPEND. A second purchase in the same open shows the SECOND survey,
	// not the union of both - "pay another 30 gold to reveal the NEW locations" is a
	// replacement in Jonathan's own sentence, and an accumulating list would slowly turn
	// into a heat map of everywhere the enemy has ever been.
	EnemyDotsWorldXY = EnemyWorldXY;

	UE_LOG(LogSiegeWarMap, Log, TEXT("[WarMap] Enemy reveal received: %d dots (frozen at purchase; cleared on close)."), EnemyDotsWorldXY.Num());

	OnWarMapRevealStateChanged(EnemyDotsWorldXY.Num() > 0, EnemyDotsWorldXY.Num());
}

void UWarMapWidget::ClearEnemyReveal()
{
	if (EnemyDotsWorldXY.Num() == 0)
	{
		return;
	}

	EnemyDotsWorldXY.Reset();
	OnWarMapRevealStateChanged(false, 0);
}

// ---------------------------------------------------------------------------
// The read-only snapshot route (WR-§6's snapshot hazard)
// ---------------------------------------------------------------------------

const USiegeAssistantSnapshot* UWarMapWidget::GetReadOnlySnapshot() const
{
	const ASiegePlayerController* const Controller = Cast<ASiegePlayerController>(GetOwningPlayer());
	if (Controller == nullptr)
	{
		return nullptr;
	}

	const USiegeAssistantComponent* const Assistant = Controller->GetAssistantComponent();
	if (Assistant == nullptr)
	{
		return nullptr;
	}

	// ⛔⛔ GetTurnSnapshot() AND NOTHING ELSE. It returns the EXISTING per-turn object or
	// null; there is no Capture() call, no EnsureSnapshot() call and no second snapshot
	// anywhere in this file. Opening a UI panel must never re-survey the world (WR-§6), and
	// the class being read says so itself: "⛔ Do NOT re-Capture() from the executor: a
	// second survey mid-turn would silently answer a different question from the one the
	// model was asked."
	const USiegeAssistantSnapshot* const Snapshot = Assistant->GetTurnSnapshot();

	if (Snapshot == nullptr && !bWarnedNoSnapshot)
	{
		bWarnedNoSnapshot = true;

		// ⚠️ Log, not Warning: a degradation line, not a fault line. ⭐ TEXT CORRECTED BY
		// TASK-685's W691-3 rider: the old line claimed "expected before the player's first
		// console sentence (the component allocates its snapshot on the turn path)" - FALSE
		// since TASK-447 (cd5f4ed): BeginPlay's EnsureSnapshot() makes the snapshot non-null
		// from match start, so a null HERE means the read ROUTE failed, not "no sentence
		// yet". The marker-less-map story lives at the status-line discriminators now.
		UE_LOG(LogSiegeWarMap, Log,
			TEXT("[WarMap] No assistant snapshot resolved - the map draws dots but NO place markers, so there is nothing to click. ")
			TEXT("The component allocates its snapshot in BeginPlay (TASK-447), so a null here means the read route failed (no ASiegePlayerController / assistant component on the owning player yet) - rare, and self-heals once the route exists. ")
			TEXT("Still read-only by design: forcing a Capture() is the exact side effect the WAR-ROOM law forbids (WR-§6). Diagnosis: handoffs/TASK-691-programmer.md."));
	}

	return Snapshot;
}

FVector2D UWarMapWidget::ResolveArenaHalfExtent() const
{
	if (const USiegeScatterConfig* const Config = ArenaConfigAsset.IsNull() ? nullptr : ArenaConfigAsset.LoadSynchronous())
	{
		return Config->ArenaHalfExtent;
	}

	// ⛔ THE FALLBACK IS THE CLASS DEFAULT OBJECT - THE SAME FIELD, READ FROM THE SAME CLASS -
	// AND ⛔ NOT A TRANSCRIBED (26000, 12000). SC-§34 names this the structural escape and
	// prefers it outright: "derive at runtime from the asset instead of transcribing a
	// number". A literal here is a legal number in the right type that compiles, passes every
	// test, and quietly draws the wrong battlefield the day the arena changes size - which is
	// verbatim the defect class SC-§34 exists to prevent.
	const FVector2D Fallback = GetDefault<USiegeScatterConfig>()->ArenaHalfExtent;

	if (!bWarnedNoArenaConfig)
	{
		bWarnedNoArenaConfig = true;
		UE_LOG(LogSiegeWarMap, Warning,
			TEXT("[WarMap] '%s' did not resolve - falling back to the USiegeScatterConfig CDO extent (%.0f x %.0f uu). ")
			TEXT("The map is still drawn and still hit-tests; it is scaled to the C++ default rather than to the saved DataAsset."),
			*ArenaConfigAsset.ToString(), Fallback.X, Fallback.Y);
	}

	return Fallback;
}

// ---------------------------------------------------------------------------
// ═══ The elevation background (TASK-684; WM-§2) ═══
// A RUNTIME one-time trace bake per match. Everything elevation lives between
// this fence and the "Ally dots" fence below - TASK-685 layers AFTER this
// section and must not need to touch inside it.
// ---------------------------------------------------------------------------

float UWarMapWidget::HeightToBrightness(float HitZ, float GroundZ, float ReliefCeiling)
{
	// ⛔ THE ZERO-DIVIDE FLOOR, applied BEFORE the division - the MinArenaHalfExtentUu
	// doctrine verbatim: ElevationReliefCeiling is an EditDefaultsOnly float a designer can
	// zero, and an unguarded divide would put NaN into every texel of the bake.
	const float SafeCeiling = FMath::Max(ReliefCeiling, 1.f);

	// Linear ramp, clamped BOTH ways: below the floor ⇒ 0 (dark), above the ceiling ⇒
	// exactly 1 (full white - the WM-§2 normalization law; castle shells and boundary walls
	// land here BY DESIGN and read as "walls" rather than crushing the hill band).
	return FMath::Clamp((HitZ - GroundZ) / SafeCeiling, 0.f, 1.f);
}

FColor UWarMapWidget::BrightnessToRampColor(float Brightness)
{
	// Total for any input. HeightToBrightness (the only shipped caller) already returns a
	// clamped [0,1], but this seam is public and tested independently, so it does not trust
	// its caller - the shipped defensive idiom, one line.
	const float T = FMath::Clamp(Brightness, 0.f, 1.f);

	// ⭐⭐ THE LERP IS IN DISPLAY (sRGB) SPACE, PER CHANNEL - the SAME arithmetic the gray
	// ramp shipped with, with three channels instead of one luma repeated three times.
	// ⛔⛔ DO NOT "IMPROVE" THIS by converting to linear, lerping there, and converting back.
	// Both endpoints are ONE chromaticity C at {0.10, 1.00}, so Lerp(C*0.10, C*1.00, T) =
	// C * Lerp(0.10, 1.00, T): hue and saturation are constant along the ramp BY
	// CONSTRUCTION and the luminance curve is byte-identical to the gray ramp this replaced
	// (WM-§8a). A LINEAR-space lerp would move the ramp's midpoint from display 0.55 to 0.73
	// and visibly redistribute where the relief reads - a change WM-§8a forbids, from an
	// edit that would look like a correctness fix.
	const float R = FMath::Lerp(SiegeWarMap::ElevationGrassDark.R, SiegeWarMap::ElevationGrassLight.R, T);
	const float G = FMath::Lerp(SiegeWarMap::ElevationGrassDark.G, SiegeWarMap::ElevationGrassLight.G, T);
	const float B = FMath::Lerp(SiegeWarMap::ElevationGrassDark.B, SiegeWarMap::ElevationGrassLight.B, T);

	// ⛔ ROUND-AND-CAST, ⛔ NEVER FLinearColor::ToFColor(true): these floats are ALREADY
	// sRGB-encoded (the constants' declaration says why at length), so ToFColor(true) would
	// encode them a SECOND time. This is the shipped `RoundToInt(Luma * 255.f)` line,
	// unchanged except that it now runs three times.
	const auto ToByte = [](float Channel) -> uint8
	{
		return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Channel * 255.f), 0, 255));
	};

	// ⚠️ CHANNEL ORDER, STATED BECAUSE A PER-CHANNEL REWRITE IS EXACTLY WHERE IT GETS
	// TRANSPOSED: FColor's CONSTRUCTOR takes R, G, B, A in that order, while its MEMORY
	// layout is B, G, R, A (little-endian) - which is what makes the Memcpy into
	// PF_B8G8R8A8 correct downstream (the comment at that Memcpy says so). Both facts are
	// true and they are not in conflict: the ctor names channels, the memcpy copies members.
	// ⛔ DO NOT verify the order against the ramp's own pixels - the dark end (16, 26, 16) is
	// SYMMETRIC in R and B and the light end (160, 255, 159) differs by ONE byte, so an R/B
	// swap hides there. Verify against this comment (and the suite's G-dominance claim,
	// which catches an R/G or G/B swap outright).
	return FColor(ToByte(R), ToByte(G), ToByte(B), 255);
}

void UWarMapWidget::EnsureElevationBake()
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		// No world, no bake - and no cache mutation either: a torn-down world's texture is
		// unreachable by paint anyway, and the next real open re-evaluates from scratch.
		return;
	}

	// ── Is the cache still true? (The staleness diagnosis is at the declaration.) ─────────
	if (ElevationTexture != nullptr && ElevationBakedWorld.Get() == World)
	{
		if (bElevationSentinelArmed)
		{
			// Steady state: TWO pointer checks. The sentinel dying = ClearScatter ran =
			// the field re-rolled ⇒ fall through and re-bake.
			if (ElevationGenerationSentinel.IsValid())
			{
				return;
			}
		}
		else if (SiegeWarMap::FindScatterGenerationSentinel(*World) == nullptr)
		{
			// Baked on a field with NO scatter actors and none have appeared since - the
			// bake stands (a debug field with no mines has no re-scatter signal to watch;
			// on the shipped field the economy law makes this branch unreachable). The
			// probe's cost class is the RefreshAllyDots sweep already on this timer.
			return;
		}
	}

	// One FAILED attempt per open, max - a pathological world (zero hits) must not re-trace
	// 7,800 rays on every 0.25 s tick. A later OPEN retries; success clears nothing because
	// success replaces the cache wholesale below.
	if (bElevationBakeFailedThisOpen)
	{
		return;
	}

	if (BakeElevationTexture())
	{
		ElevationBakedWorld = World;
		const AActor* const Sentinel = SiegeWarMap::FindScatterGenerationSentinel(*World);
		ElevationGenerationSentinel = Sentinel;
		bElevationSentinelArmed = (Sentinel != nullptr);
		return;
	}

	// ⛔ FAILED ⇒ the shipped flat background stands, and a STALE bake never stands: a map
	// showing LAST generate's hills is a lie about this one (the exact lie WM-§2 refuses
	// offline bakes over), so the texture is dropped rather than kept.
	ElevationTexture = nullptr;
	ElevationBakedWorld = nullptr;
	ElevationGenerationSentinel = nullptr;
	bElevationSentinelArmed = false;
	bElevationBakeFailedThisOpen = true;

	if (!bWarnedElevationBakeFailed)
	{
		bWarnedElevationBakeFailed = true;

		// Log, not Warning, and ONCE (the WarnedNoSnapshot latch idiom): a bake that fails
		// before the arena floor exists is a timing state, not a fault, and the line already
		// explains the degraded look a playtest would report.
		UE_LOG(LogSiegeWarMap, Log,
			TEXT("[WarMap] Elevation bake failed - no static geometry was hit under the sample grid. ")
			TEXT("The map keeps its flat background (degraded, never broken) and will retry on a later open."));
	}
}

bool UWarMapWidget::BakeElevationTexture()
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	// Belt and braces under the UPROPERTY clamps: a hand-edited archive can carry values the
	// editor UI would have refused, and this function multiplies the two into an allocation.
	const int32 GridX = FMath::Clamp(ElevationGridX, 2, 1024);
	const int32 GridY = FMath::Clamp(ElevationGridY, 2, 1024);
	const int32 SampleCount = GridX * GridY;

	// ⛔ THE SAMPLED RECT IS THE CONFIGURED ARENA, from its single owner via the SHIPPED
	// asset→CDO resolver (SC-§34) - not one hand-typed dimension anywhere in the bake.
	const FVector2D ArenaHalfExtent = ResolveArenaHalfExtent();

	// ── The 7,800-trace capture ───────────────────────────────────────────────────────────
	//
	// ⚠️⚠️ OBJECT-TYPE QUERY, ⛔ NOT A CHANNEL TRACE, AND THE DIFFERENCE IS THE WHOLE
	// FEATURE - DIAGNOSED AT SOURCE, DECLARED AS THE SPEC DEPARTURE IT IS (SC-§15). The spec
	// spells "ECC_WorldStatic", but the scatter-channel law (CONVENTIONS "Climbable terrain",
	// verified at BattlefieldScatter.cpp ResolveComponentForMesh: SetCollisionResponseToAllChannels(ECR_Ignore)
	// then Block on Pawn/Team/Visibility/Camera only) makes every hill HISM IGNORE the
	// ECC_WorldStatic *channel* - deliberately, so GroundZAt's floor trace passes through
	// them. A BY-CHANNEL WorldStatic trace therefore sees a FLAT FLOOR PLUS CASTLES: the
	// uniform-gray ground-only bake WM-§2 itself refuses as delivering nothing. The hills ARE
	// WorldStatic OBJECTS (SetCollisionObjectType(ECC_WorldStatic), QueryOnly), and an
	// object-type query filters on OBJECT TYPE, not on channel responses ⇒ it sees the floor,
	// the hills, the rocks, the castles and the walls - the standable field - while excluding
	// pawns, units and projectiles BY CONSTRUCTION (their object types are Pawn/WorldDynamic),
	// which is exactly the spec's "ignore pawns/units/projectiles" with no per-actor ignore
	// list to go stale. The pinned token ECC_WorldStatic is still the filter - as the object
	// type, which is the reading that delivers the ruling's WHY.
	const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic);

	// bTraceComplex=false, the shipped GroundZAt idiom: simple collision IS the truth here by
	// authored law (hill hulls are generate_convex_collisions(hull_count=1) with hull ZMax ==
	// mesh ZMax - the climbable-geometry acceptance gate), and the castle's 61 hulls are its
	// collision truth too. Per-poly would cost more to say the same thing.
	FCollisionQueryParams Params(TEXT("WarMapElevationTrace"), /*bTraceComplex=*/false);

	TArray<float> SampleZ;
	SampleZ.SetNumZeroed(SampleCount);
	TBitArray<> SampleHasHit(false, SampleCount);

	int32 HitCount = 0;
	int32 AboveCeilingCount = 0;
	float MinHitZ = TNumericLimits<float>::Max();
	float MaxHitZ = TNumericLimits<float>::Lowest();

	TArray<FHitResult> Hits;

	for (int32 Iy = 0; Iy < GridY; ++Iy)
	{
		for (int32 Ix = 0; Ix < GridX; ++Ix)
		{
			// Texel CENTRES, through the projection's own inverse - the bake's sample layout
			// and the painter's texel layout agree because both are the same MapUV, and the
			// round trip is pinned by test. Row 0 = UV.Y 0 = the TOP of the drawn map, which
			// is also texture row 0: no flip happens here or at draw.
			const FVector2D MapUV(
				(static_cast<double>(Ix) + 0.5) / static_cast<double>(GridX),
				(static_cast<double>(Iy) + 0.5) / static_cast<double>(GridY));
			const FVector2D WorldXY = FSiegeWarMapProjection::MapUVToWorld(MapUV, ArenaHalfExtent);

			const FVector TraceStart(WorldXY.X, WorldXY.Y, SiegeWarMap::ElevationTraceHalfHeightUu);
			const FVector TraceEnd(WorldXY.X, WorldXY.Y, -SiegeWarMap::ElevationTraceHalfHeightUu);

			Hits.Reset();
			World->LineTraceMultiByObjectType(Hits, TraceStart, TraceEnd, ObjectParams, Params);

			// ⭐ HIGHEST VISIBLE hit wins. HIGHEST: the surface the eye would see from above
			// (the FindHillSurfaceAt "highest hit wins" doctrine - a buried skirt must not
			// out-vote the crown), stated as a max rather than as an ordering assumption over
			// the engine's hit sort. VISIBLE: the tree collision proxies are WorldStatic
			// objects too, but they are SetVisibility(false) invisible cylinders - a heightmap
			// that recorded them would grow a white spike per tree the player cannot see.
			// What the eye cannot see, the map must not claim.
			bool bFound = false;
			float BestZ = 0.f;

			for (const FHitResult& Hit : Hits)
			{
				const UPrimitiveComponent* const Component = Hit.GetComponent();
				if (Component == nullptr || !Component->IsVisible())
				{
					continue;
				}

				const float HitZ = static_cast<float>(Hit.ImpactPoint.Z);
				if (!bFound || HitZ > BestZ)
				{
					bFound = true;
					BestZ = HitZ;
				}
			}

			if (bFound)
			{
				const int32 SampleIndex = Iy * GridX + Ix;
				SampleZ[SampleIndex] = BestZ;
				SampleHasHit[SampleIndex] = true;
				++HitCount;
				MinHitZ = FMath::Min(MinHitZ, BestZ);
				MaxHitZ = FMath::Max(MaxHitZ, BestZ);
			}
			// A miss (the configured extent pokes a little past the floor slab on X by
			// design) simply stays floor-dark below - a partial fringe is not a failure.
		}
	}

	if (HitCount == 0)
	{
		// Nothing static under the whole grid - no floor exists yet (or at all). The caller
		// logs once and keeps the shipped background.
		return false;
	}

	// ── GroundZ is MEASURED off the bake itself, ⛔ never hand-typed ──────────────────────
	// The darkest brightness is anchored at the LOWEST sampled surface - on the shipped
	// field, the flat floor slab - so the ramp needs no transcribed "floor is at Z=0" claim
	// that would rot the day the arena mesh moves (the SC-§34 doctrine, applied to Z).
	const float GroundZ = MinHitZ;

	// ── One transient texture, one pixel per sample ───────────────────────────────────────
	UTexture2D* const Texture = UTexture2D::CreateTransient(GridX, GridY, PF_B8G8R8A8);
	if (Texture == nullptr)
	{
		return false;
	}

	// UI-surface settings: sRGB like every T_ UI texture; bilinear so 7,800 texels stretch
	// into smooth slopes across the map rect instead of visible squares.
	Texture->SRGB = true;
	Texture->Filter = TF_Bilinear;

	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(SampleCount);

	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		// A missed sample reads as floor (brightness 0) - dark, honest, and indistinguishable
		// from the low field it borders.
		const float Brightness = SampleHasHit[SampleIndex]
			? HeightToBrightness(SampleZ[SampleIndex], GroundZ, ElevationReliefCeiling)
			: 0.f;

		if (Brightness >= 1.f)
		{
			++AboveCeilingCount;
		}

		// TWO seams, and the split is the point (WM-§2, restated by WM-§8a): HeightToBrightness
		// above owns the ramp's SHAPE, BrightnessToRampColor owns its SCREEN MAPPING. TASK-722
		// turned this ramp from gray to green by changing ONLY the second one - the first is
		// byte-identical to the day it shipped, and its pinned three-parameter signature never
		// had to move (SC-§33). Both are tested headlessly in Tests/SiegeWarMapTest.cpp.
		Pixels[SampleIndex] = UWarMapWidget::BrightnessToRampColor(Brightness);
	}

	FTexturePlatformData* const PlatformData = Texture->GetPlatformData();
	if (PlatformData == nullptr || PlatformData->Mips.Num() == 0)
	{
		return false;
	}

	// FColor's little-endian memory layout is B,G,R,A - byte-identical to PF_B8G8R8A8, the
	// standard CreateTransient fill idiom.
	void* const MipData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	if (MipData == nullptr)
	{
		PlatformData->Mips[0].BulkData.Unlock();
		return false;
	}

	FMemory::Memcpy(MipData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	PlatformData->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();

	// Publish: the UPROPERTY roots the texture for GC; the brush only mirrors it.
	ElevationTexture = Texture;
	ElevationBrush.SetResourceObject(Texture);
	ElevationBrush.ImageSize = FVector2f(static_cast<float>(GridX), static_cast<float>(GridY));
	ElevationBrush.DrawAs = ESlateBrushDrawType::Image;
	ElevationBrush.TintColor = FSlateColor(FLinearColor::White);

	// ONE line per bake - and bakes happen once per match (plus once per Play Again), so
	// this is the whole diagnostic surface of the feature, not spam. Figures, not vibes:
	// TASK-689's editor verify and any playtest report land on a line that already answers
	// "did it see the hills" (relief > 0) and "did the clamp fire where expected".
	// ⚠️ The tail reads "clamped to the ramp's LIGHT END", ⛔ no longer "full white":
	// TASK-722 made the ramp green, and a diagnostic a human reads while looking at a green
	// map must not name a colour the map no longer contains.
	UE_LOG(LogSiegeWarMap, Log,
		TEXT("[WarMap] Elevation baked: %dx%d samples, %d hits, groundZ=%.1f, maxZ=%.1f (relief %.1f uu), ceiling=%.1f, %d texel(s) clamped to the ramp's light end."),
		GridX, GridY, HitCount, GroundZ, MaxHitZ, MaxHitZ - GroundZ, FMath::Max(ElevationReliefCeiling, 1.f), AboveCeilingCount);

	return true;
}

// ---------------------------------------------------------------------------
// Ally dots (WR-§6) - the only POSITION-reading actor iteration in this file
// (TASK-684's sentinel probe above iterates for IDENTITY only, comment-audited
// at its own declaration - still no place iterator anywhere here)
// ---------------------------------------------------------------------------

void UWarMapWidget::SetAllyRefreshTimerEnabled(bool bEnabled)
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FTimerManager& Timers = World->GetTimerManager();

	if (!bEnabled)
	{
		Timers.ClearTimer(AllyDotTimerHandle);
		return;
	}

	// ⚠️ A TIMER, ⛔ NOT NativeTick, AND THE CHOICE IS A CORRECTNESS ONE RATHER THAN A STYLE
	// ONE. UUserWidget is declared meta=(DisableNativeTick) (UserWidget.h:279) and
	// UUserWidget::UpdateCanTick only re-enables native ticking for a
	// UWidgetBlueprintGeneratedClass when ClassRequiresNativeTick() says so
	// (UserWidget.cpp:2358-2361). ⇒ A NativeTick override would run on the bare C++ class
	// and could silently STOP running the moment TASK-568 reparents WBP_WarMap to it - the
	// worst possible failure shape, because it works right up until the art lands. A timer
	// is owned by the world and is indifferent to all of that.
	//
	// ⚠️ And the interval is the point: at 0.25 s this sweep runs 4x a second while the map
	// is OPEN and never at all while it is closed - not once per frame, and never in a match
	// where the player never opens the map.
	//
	// ⚠️ TASK-579 POINTED THIS AT HandleMapRefreshTimer RATHER THAN AT RefreshAllyDots DIRECTLY.
	// The handle, the interval and this function's name are all UNCHANGED; only the callback
	// moved, so it can do the ally sweep AND retire the first-open hint on the same tick. ⛔ A
	// second timer for a bool that flips once per open would be a second lifetime to clear on
	// close and again on teardown - the bookkeeping NativeDestruct already double-covers here.
	Timers.SetTimer(
		AllyDotTimerHandle,
		this,
		&UWarMapWidget::HandleMapRefreshTimer,
		FMath::Max(AllyDotRefreshInterval, 0.05f),
		/*bLoop=*/ true);
}

void UWarMapWidget::HandleMapRefreshTimer()
{
	RefreshAllyDots();

	// ⭐ TASK-685 - the POI census rides the same tick, right after the dot sweep it is the
	// sibling of (WM-§1: the ally-dot precedent). Mines deplete and castles fall mid-match,
	// so the census refreshes at the dots' cadence rather than once per open - a handful of
	// world actors 4x a second while open, never while closed, never per frame.
	RefreshPoiIcons();

	// ⛔ AFTER the sweeps, never before: the sweeps are the thing the interval exists for,
	// and a hint check that threw would otherwise take the dots down with it. Order is cheap
	// insurance here.
	UpdateNoSnapshotHint();

	// ⭐ TASK-684 - the elevation cache's SELF-HEAL, riding the timer the class already owns
	// (the TASK-579 one-timer doctrine: no second lifetime to clear on close and again on
	// teardown). Steady state this is TWO pointer checks; it re-bakes only when the field
	// actually re-rolled under an OPEN map (Play Again with the map up) or when a
	// late-generating client's scatter arrived after an early first open. Still ⛔ zero
	// per-frame cost - this runs at the refresh interval, and only while the map is open.
	EnsureElevationBake();
}

void UWarMapWidget::RefreshAllyDots()
{
	AllyDotsWorldXY.Reset();

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const APlayerController* const Controller = GetOwningPlayer();
	const ASiegePlayerState* const OwningState = Controller ? Cast<ASiegePlayerState>(Controller->PlayerState) : nullptr;
	if (OwningState == nullptr)
	{
		// ⛔ NEVER GUESS A TEAM - the shipped ResolveOrderingTeam doctrine, applied. A default
		// of Blue on a Red client would paint the ENEMY army in friendly blue, which is worse
		// than an empty map and much harder to notice.
		return;
	}

	const ETeamId LocalTeam = OwningState->GetTeam();

	for (TActorIterator<ASummonedUnit> UnitIt(World); UnitIt; ++UnitIt)
	{
		const ASummonedUnit* const Unit = *UnitIt;
		if (!IsValid(Unit) || Unit->IsUnitDead() || Unit->GetTeamId() != LocalTeam)
		{
			continue;
		}

		const FVector Location = Unit->GetActorLocation();
		AllyDotsWorldXY.Emplace(Location.X, Location.Y);
	}

	for (TActorIterator<AHeroCharacter> HeroIt(World); HeroIt; ++HeroIt)
	{
		const AHeroCharacter* const Hero = *HeroIt;
		if (!IsValid(Hero) || Hero->IsDead() || Hero->GetTeamId() != LocalTeam)
		{
			continue;
		}

		const FVector Location = Hero->GetActorLocation();
		AllyDotsWorldXY.Emplace(Location.X, Location.Y);
	}
}

// ---------------------------------------------------------------------------
// ═══ The POI icon census (TASK-685; WM-§1) ═══
// READ-ONLY world iteration for a DISPLAY-ONLY layer. ⛔ Nothing gathered here
// is hit-testable, enters the marker array, or leaves this widget; ⛔ nothing
// on this path reads, creates or reaches a snapshot (the airlock stands) - the
// dot sweep's exact discipline, pointed at the field's fixtures.
// ---------------------------------------------------------------------------

void UWarMapWidget::RefreshPoiIcons()
{
	MinePoiWorldXY.Reset();
	AncientGroundPoiWorldXY.Reset();
	BlueCastlePoiWorldXY.Reset();
	RedCastlePoiWorldXY.Reset();

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// ── Mines = the GOLD-NODE mines, and ONLY them (J4, flagged and defaulted) ────────────
	// ADeepMine is an ABuilding subclass, so this iterator excludes player-built DeepMines
	// BY CONSTRUCTION - no filter to write, none to go stale. Depleted mines are skipped
	// (AGoldNode::IsDepleted - the one-way latch its own header documents): a mine with no
	// gold is no longer a point of interest, and its glow gauge already tells the player so
	// in-world; the map saying otherwise would be a lie about where gold is.
	for (TActorIterator<AGoldNode> MineIt(World); MineIt; ++MineIt)
	{
		const AGoldNode* const Mine = *MineIt;
		if (!IsValid(Mine) || Mine->IsDepleted())
		{
			continue;
		}

		const FVector Location = Mine->GetActorLocation();
		MinePoiWorldXY.Emplace(Location.X, Location.Y);
	}

	// ── Both ancient grounds - team-neutral fixtures, no state to filter on ───────────────
	for (TActorIterator<AAncientGround> GroundIt(World); GroundIt; ++GroundIt)
	{
		const AAncientGround* const Ground = *GroundIt;
		if (!IsValid(Ground))
		{
			continue;
		}

		const FVector Location = Ground->GetActorLocation();
		AncientGroundPoiWorldXY.Emplace(Location.X, Location.Y);
	}

	// ── The castles, split BY TEAM for the W4-R5 tint ─────────────────────────────────────
	// ⚠️ BY THE CASTLE'S OWN TEAM, not by ally/enemy relation: J2's ruling is "castles
	// blue/red via the team palette", so a Red-team player sees his own castle RED - the
	// castle wears its team's colour everywhere else in the game too. ETeamId has exactly
	// Blue and Red (TeamId.h), so the else-arm is exhaustive, not a guess. A DESTROYED
	// castle is skipped (IsCastleDestroyed - DestroyCastle hides the actor): what the eye
	// cannot see in the world, the map must not claim - the elevation bake's own doctrine.
	for (TActorIterator<ACastle> CastleIt(World); CastleIt; ++CastleIt)
	{
		const ACastle* const Castle = *CastleIt;
		if (!IsValid(Castle) || Castle->IsCastleDestroyed())
		{
			continue;
		}

		const FVector Location = Castle->GetActorLocation();
		if (Castle->GetTeamId() == ETeamId::Blue)
		{
			BlueCastlePoiWorldXY.Emplace(Location.X, Location.Y);
		}
		else
		{
			RedCastlePoiWorldXY.Emplace(Location.X, Location.Y);
		}
	}
}

void UWarMapWidget::ResolvePoiIconTextures()
{
	// Three refs through ONE resolver (SiegeWarMap::ResolvePoiIconTexture - the two-null-
	// paths contract lives there). Idempotent per icon; the aggregate log below fires ONCE
	// per widget lifetime, listing exactly which of the SET refs failed to load.
	const bool bMineMissing = SiegeWarMap::ResolvePoiIconTexture(
		MineIconTexture, MineIconResolvedTexture, MineIconBrush);
	const bool bGroundMissing = SiegeWarMap::ResolvePoiIconTexture(
		AncientGroundIconTexture, AncientGroundIconResolvedTexture, AncientGroundIconBrush);
	const bool bCastleMissing = SiegeWarMap::ResolvePoiIconTexture(
		CastleIconTexture, CastleIconResolvedTexture, CastleIconBrush);

	if ((bMineMissing || bGroundMissing || bCastleMissing) && !bWarnedPoiIconUnresolved)
	{
		bWarnedPoiIconUnresolved = true;

		// Log, not Warning, and ONCE (the bWarnedElevationBakeFailed idiom): a missing UI
		// texture is a degraded look with a working fallback (tinted dots), not a fault -
		// and the line already explains the look a playtest would report.
		UE_LOG(LogSiegeWarMap, Log,
			TEXT("[WarMap] POI icon texture(s) set but unresolved (mine=%d ancientGround=%d castle=%d; 1=missing) - ")
			TEXT("those POIs draw as tinted dots instead of glyphs (degraded, never broken). ")
			TEXT("Expected paths: /Game/UI/WarMap/T_WarMap_Icon_{Mine,AncientGround,Castle} (TASK-683)."),
			bMineMissing ? 1 : 0, bGroundMissing ? 1 : 0, bCastleMissing ? 1 : 0);
	}
}

// ---------------------------------------------------------------------------
// Marker geometry - ONE builder, shared by the painter and the hit test
// ---------------------------------------------------------------------------

void UWarMapWidget::BuildMarkerRects(const FGeometry& AllottedGeometry, TArray<FSiegeWarMapMarker>& OutMarkers) const
{
	OutMarkers.Reset();

	const USiegeAssistantSnapshot* const Snapshot = GetReadOnlySnapshot();
	if (Snapshot == nullptr)
	{
		return;
	}

	const FVector2f PanelSizeF = AllottedGeometry.GetLocalSize();
	const FVector2D PanelSize(PanelSizeF.X, PanelSizeF.Y);
	const FVector2D ArenaHalfExtent = ResolveArenaHalfExtent();

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(PanelSize, MapPaddingPx, ArenaHalfExtent, RectOrigin, RectSize);

	if (RectSize.X <= 0.0 || RectSize.Y <= 0.0)
	{
		return;
	}

	const FVector2D HitHalfSize(MarkerHitHalfSizePx, MarkerHitHalfSizePx);

	// ⛔ THE SYMBOL LIST IS READ, NEVER SPELLED. GetPlaceNames() publishes exactly the places
	// that RESOLVE this match, in fixed vocabulary order - so this file contains no place
	// literal, cannot add an eighth place, and picks up any future vocabulary change for free
	// (WR-§6: "NO TASK IN THIS BATCH MAY ADD A PLACE SYMBOL").
	for (const FName& PlaceSymbol : Snapshot->GetPlaceNames())
	{
		FVector PlaceLocation = FVector::ZeroVector;

		// ⚠️ ResolvePlace leaves OutLocation UNTOUCHED on failure, by its own contract, so the
		// return value is checked rather than the value inspected. An unresolved place simply
		// has no marker - the same fail-closed direction the grammar takes.
		if (!Snapshot->ResolvePlace(PlaceSymbol, PlaceLocation))
		{
			continue;
		}

		FSiegeWarMapMarker& Marker = OutMarkers.AddDefaulted_GetRef();
		Marker.PlaceSymbol = PlaceSymbol;
		Marker.LocalCentre = FSiegeWarMapProjection::MapUVToLocal(
			FSiegeWarMapProjection::WorldToMapUV(FVector2D(PlaceLocation.X, PlaceLocation.Y), ArenaHalfExtent),
			RectOrigin,
			RectSize);
		Marker.LocalHitHalfSize = HitHalfSize;
	}
}

// ---------------------------------------------------------------------------
// ═══ MAP MARKS - the player's numbered circles (TASK-745; MARK-§) ═══
//
// ⛔ THIS WIDGET OWNS NO MARK STATE. Everything below either READS
// USiegeMapMarkSubsystem::GetMarks() or calls exactly one of the four mutating
// methods on the PINNED registry. There is no cached array, no shadow count and
// no second copy of a number - so nothing here can go stale across an open, a
// panel resize, a match reset or a delete.
//
// ⛔ AND NOTHING HERE READS, CREATES OR REACHES A SNAPSHOT. The mark lane never
// touches USiegeAssistantSnapshot at all: no Capture(), no EnsureSnapshot(), not
// even a GetPlaceNames() read. The airlock is untouched by every line of it
// (WR-§6, MARK-§5) - and TASK-746, in a file this task does not own, is what
// makes a mark answerable as a place.
// ---------------------------------------------------------------------------

FName UWarMapWidget::MakeMarkPickSymbol(int32 Number)
{
	// ⛔ A NON-POSITIVE NUMBER CAN NEVER BECOME A SYMBOL. FSiegeMapMark::Number is 1..9 by the
	// store's own contract, so 0 here means the store handed back an uninitialised entry - and
	// the correct answer to that is NAME_None, which the controller's binder already refuses
	// with a log (`war-map place pick carried no symbol - ignored`). ⇒ a corrupt store degrades
	// to a logged no-op and can ⛔ never write an empty token into the player's input box.
	if (Number <= 0)
	{
		return NAME_None;
	}

	// ⛔⛔ DELEGATED, ⛔ NEVER SPELLED. FSiegeMapMark::MakeSymbol is the ONE seam this widget and
	// USiegeAssistantSnapshot must agree on (MARK-§5). A literal `circle_` here would be a
	// second transcription, and its drift would be silent in the worst possible way: the map
	// would insert a name TASK-746 never published, the grammar would refuse it, and the
	// feature would read to Jonathan as "the AI ignores my circles."
	return FName(*FSiegeMapMark::MakeSymbol(Number));
}

FLinearColor UWarMapWidget::GetMarkRingColor()
{
	// ⛔ TRUE LINEAR (Slate tint). The whole colour-space argument, the magenta derivation and
	// the 3.84:1 flat-colour ceiling live at the constant's declaration above; the declaration
	// comment in the header explains why this accessor exists (so a test can MEASURE the
	// legibility claim instead of a comment asserting it).
	return SiegeWarMap::MarkRingColor;
}

USiegeMapMarkSubsystem* UWarMapWidget::GetMarkSubsystem() const
{
	// ⛔ A LOOKUP, ⛔ NEVER A CREATE - GetSubsystem<T>() returns the collection's existing
	// instance. No owning local player (a headless widget, a viewport teardown, a widget built
	// before possession) ⇒ null ⇒ every mark verb degrades to "unavailable" and the SHIPPED
	// pre-TASK-745 behaviour runs instead. ⛔ Never a crash, ⛔ never a silent half-state.
	const ULocalPlayer* const LocalPlayer = GetOwningLocalPlayer();
	if (LocalPlayer == nullptr)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<USiegeMapMarkSubsystem>();
}

int32 UWarMapWidget::GetMapMarkCount() const
{
	const USiegeMapMarkSubsystem* const Marks = GetMarkSubsystem();
	return (Marks != nullptr) ? Marks->GetMarks().Num() : 0;
}

void UWarMapWidget::BuildMarkCircles(const FGeometry& AllottedGeometry, TArray<FSiegeWarMapMarkCircle>& OutCircles) const
{
	// ⛔ RESET FIRST, BEFORE EVERY EARLY RETURN - the BuildMarkerRects contract, verbatim: the
	// caller must never be left holding LAST frame's circles because this frame had no store.
	// A stale circle is worse than no circle: it would hit-test at a position nothing is drawn.
	OutCircles.Reset();

	const USiegeMapMarkSubsystem* const Marks = GetMarkSubsystem();
	if (Marks == nullptr)
	{
		return;
	}

	const FVector2f PanelSizeF = AllottedGeometry.GetLocalSize();
	const FVector2D PanelSize(PanelSizeF.X, PanelSizeF.Y);
	const FVector2D ArenaHalfExtent = ResolveArenaHalfExtent();

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(PanelSize, MapPaddingPx, ArenaHalfExtent, RectOrigin, RectSize);

	if (RectSize.X <= 0.0 || RectSize.Y <= 0.0)
	{
		return;
	}

	// ⭐ STORE ORDER IS PAINT ORDER IS HIT ORDER. FindMarkIndexAtLocal scans BACKWARDS
	// (last-match-wins) and the painter draws forwards, so preserving the store's order here is
	// what makes "the topmost circle is the one you hit" true rather than hoped.
	const TArray<FSiegeMapMark>& StoredMarks = Marks->GetMarks();
	OutCircles.Reserve(StoredMarks.Num());

	for (const FSiegeMapMark& Mark : StoredMarks)
	{
		FSiegeWarMapMarkCircle& Circle = OutCircles.AddDefaulted_GetRef();

		// ⛔ THE NUMBER IS COPIED, ⛔ NEVER RE-DERIVED FROM THE LOOP INDEX. `M-1`: deleting
		// circle 2 of 3 leaves the hole, so index 1 holds number 3 - and an index-derived
		// number would silently rename every surviving mark the moment one was deleted, which
		// is precisely the order-redirection the no-renumber rule exists to prevent.
		Circle.Number = Mark.Number;

		// ⛔ THE SHIPPED PROJECTION CHAIN, ⛔ NEVER RE-DERIVED (WM-§7's single-owner law): the
		// same WorldToMapUV → MapUVToLocal the markers, dots, icons and elevation bake all use,
		// so a mark and a dot can never disagree about where the arena is.
		Circle.LocalCentre = FSiegeWarMapProjection::MapUVToLocal(
			FSiegeWarMapProjection::WorldToMapUV(Mark.WorldXY, ArenaHalfExtent),
			RectOrigin,
			RectSize);

		Circle.LocalRadiusPx = FSiegeWarMapProjection::MapWorldRadiusToLocalPx(
			Mark.RadiusUU, ArenaHalfExtent, RectSize);
	}
}

bool UWarMapWidget::TryPlaceMarkAtLocal(const FGeometry& InGeometry, const FVector2D& LocalPoint)
{
	USiegeMapMarkSubsystem* const Marks = GetMarkSubsystem();
	if (Marks == nullptr)
	{
		// ⛔ NOT CONSUMED. The caller then runs the SHIPPED empty-click arm verbatim, which is
		// how WR-§9 outcome 1's retired behaviour survives as a real degrade path rather than
		// as deleted code.
		return false;
	}

	const FVector2f PanelSizeF = InGeometry.GetLocalSize();
	const FVector2D PanelSize(PanelSizeF.X, PanelSizeF.Y);
	const FVector2D ArenaHalfExtent = ResolveArenaHalfExtent();

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(PanelSize, MapPaddingPx, ArenaHalfExtent, RectOrigin, RectSize);

	// ⛔⛔ OUTSIDE THE DRAWN RECT IS ⛔ NOT A PLACE. The map is letterboxed inside the panel, so
	// a click in the padding or the letterbox is chrome, not battlefield - and LocalToMapUV
	// REFUSES rather than clamping, because a clamped mark would sit on ground the player never
	// pointed at and ResolvePlace would later answer an order with that ground.
	FVector2D MapUV = FVector2D::ZeroVector;
	if (!FSiegeWarMapProjection::LocalToMapUV(LocalPoint, RectOrigin, RectSize, MapUV))
	{
		return false;
	}

	// ⚠️⚠️ THIS IS THE ONE LINE IN THE FILE THAT PRODUCES A WORLD COORDINATE, AND ITS THREE
	// SAFETY PROPERTIES ARE STRUCTURAL (WarMapWidget.h §8(a)):
	//   1. it goes into a CLIENT-LOCAL ULocalPlayerSubsystem and nowhere else - the map's entire
	//      outbound surface is still FOnWarMapPlacePicked(FName);
	//   2. MARK-§1 sanctions exactly this shape - "a mark's centre is resolved on the GAME side
	//      by ResolvePlace, exactly as nearest_mine resolves to a world position today without
	//      ever printing one";
	//   3. ⛔ it does NOT come from the elevation bake. It comes from the projection pair's
	//      exact X/Y inverse, and an FVector2D HAS NO Z - so the 1,000-uu-clamped display buffer
	//      this same class owns is structurally unreachable from a mark (WM-§8d's firewall, and
	//      the SHIP-§9 fake-instrument class it exists to prevent).
	const FVector2D WorldXY = FSiegeWarMapProjection::MapUVToWorld(MapUV, ArenaHalfExtent);

	// The birth radius is a WIDGET-SPACE tunable (MARK-§4) converted through the one crossing.
	const float RadiusUU = FSiegeWarMapProjection::MapLocalPxToWorldRadius(
		FMath::Clamp(MarkDefaultRadiusPx, MarkMinRadiusPx, FMath::Max(MarkMaxRadiusPx, MarkMinRadiusPx)),
		ArenaHalfExtent,
		RectSize);

	FSiegeMapMark Placed;
	if (!Marks->AddMark(WorldXY, RadiusUU, Placed))
	{
		// ⛔⛔ THE CAP REFUSES **LOUDLY** (M-5, the shipped refusal doctrine) - ⛔ never a silent
		// no-op, which would read as a broken map. ⚠️ THE COUNT IS READ FROM THE STORE at the
		// moment of refusal rather than transcribed: AddMark only refuses at MaxMapMarks, so
		// Num() IS the cap by definition, and a line built this way cannot drift the day the
		// cap is retuned on the subsystem's EditDefaultsOnly property.
		SetStatusLine(SiegeWarMap::MakeMarkCapRefusedStatusLine(Marks->GetMarks().Num()));

		// ⭐ CONSUMED. The click was answered - with a refusal the player can read and act on -
		// so the shipped empty-click hint must NOT also fire and overwrite it.
		return true;
	}

	// ⛔ THE LINE NAMES THE **NUMBER**, ⛔ NEVER THE SYMBOL (MARK-§2): the player sees `1`, the
	// model sees `circle_1`, and neither may be shown in the other's place.
	SetStatusLine(SiegeWarMap::MakeMarkPlacedStatusLine(Placed.Number));

	// The newly placed circle is under the cursor by construction, so the hover highlight is
	// correct immediately rather than one mouse-move later.
	HoveredMarkNumber = Placed.Number;

	UE_LOG(LogSiegeWarMap, Verbose,
		TEXT("[WarMap] Map mark placed: number %d (the symbol is only ever composed at pick time; no coordinate leaves this widget)."),
		Placed.Number);

	return true;
}

bool UWarMapWidget::TryDeleteMarkAtLocal(const FGeometry& InGeometry, const FVector2D& LocalPoint)
{
	USiegeMapMarkSubsystem* const Marks = GetMarkSubsystem();
	if (Marks == nullptr)
	{
		return false;
	}

	// ⭐ THE SAME BUILDER THE PAINTER USES, against the geometry Slate just handed this event -
	// so the circle the player is pointing at is the circle that dies.
	TArray<FSiegeWarMapMarkCircle> Circles;
	BuildMarkCircles(InGeometry, Circles);

	const int32 HitIndex = FSiegeWarMapProjection::FindMarkIndexAtLocal(Circles, LocalPoint);
	if (HitIndex == INDEX_NONE)
	{
		// ⛔ A RIGHT-CLICK ON EMPTY MAP DOES NOTHING. No mark is deleted, no status line is
		// written, no coordinate is invented and no nearest-circle guess is made - deleting the
		// closest circle to a miss is exactly the silent wrong answer this feature must not have.
		return false;
	}

	// ⛔ ADDRESSED BY **NUMBER**, ⛔ NEVER BY INDEX (M-1). The store's array order is its own
	// business and holes make index ≠ number; RemoveMark(Number) is the pinned contract.
	const int32 Number = Circles[HitIndex].Number;
	if (!Marks->RemoveMark(Number))
	{
		return false;
	}

	if (HoveredMarkNumber == Number)
	{
		// ⛔ A HIGHLIGHT THAT OUTLIVES ITS CIRCLE IS A LIE ABOUT WHAT THE WHEEL WILL RESIZE.
		HoveredMarkNumber = 0;
	}

	// ⛔ THE LINE SAYS THE HOLE STAYS, AND THAT SENTENCE IS LOAD-BEARING (M-1): a player who
	// deletes 2 of 3 and then sees 1 and 3 remain will otherwise report it as a numbering bug -
	// and the "fix" a future reader would reach for is the renumber that would silently
	// redirect an order already sitting unsent in his input box.
	SetStatusLine(SiegeWarMap::MakeMarkDeletedStatusLine(Number));

	UE_LOG(LogSiegeWarMap, Verbose,
		TEXT("[WarMap] Map mark deleted: number %d. The hole is LEFT - surviving marks keep their numbers (MARK-M-1)."),
		Number);

	return true;
}

bool UWarMapWidget::TryResizeMarkAtLocal(const FGeometry& InGeometry, const FVector2D& LocalPoint, float WheelDelta)
{
	USiegeMapMarkSubsystem* const Marks = GetMarkSubsystem();
	if (Marks == nullptr)
	{
		return false;
	}

	TArray<FSiegeWarMapMarkCircle> Circles;
	BuildMarkCircles(InGeometry, Circles);

	// ⭐ THE TARGET IS THIS EVENT'S OWN CURSOR POSITION, ⛔ NOT HoveredMarkNumber. That state is
	// a cosmetic highlight updated by a different event on a different cadence; resizing off it
	// would let a stale hover silently resize a circle the cursor had already left.
	const int32 HitIndex = FSiegeWarMapProjection::FindMarkIndexAtLocal(Circles, LocalPoint);
	if (HitIndex == INDEX_NONE)
	{
		// ⛔ THE WHEEL OUTSIDE A MARK CHANGES NOTHING, ANYWHERE. ⛔ Not the nearest circle, ⛔ not
		// the last-touched one, ⛔ not a map zoom (there is none). MARK-§4's "INERT" in full.
		return false;
	}

	const FSiegeWarMapMarkCircle& Circle = Circles[HitIndex];

	// ⛔ THE STEP AND THE CLAMP ARE IN **PIXELS** (MARK-§4: widget-space tunables, ⛔ never the
	// world-space GroupRadiusWheelStep/Min/Max), and the STORE is written in uu - the one
	// crossing lives in MapLocalPxToWorldRadius and its inverse, and nowhere else.
	const float NewRadiusPx = FSiegeWarMapProjection::StepMarkRadiusPx(
		Circle.LocalRadiusPx, WheelDelta, MarkWheelStepPx, MarkMinRadiusPx, MarkMaxRadiusPx);

	if (NewRadiusPx == Circle.LocalRadiusPx)
	{
		// Pinned at a clamp, or a zero delta ⇒ nothing to write. ⛔ Not an error and ⛔ not a
		// status line: the player is holding the wheel against a limit, which the ring's own
		// size already tells him.
		return false;
	}

	const FVector2f PanelSizeF = InGeometry.GetLocalSize();
	const FVector2D PanelSize(PanelSizeF.X, PanelSizeF.Y);
	const FVector2D ArenaHalfExtent = ResolveArenaHalfExtent();

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(PanelSize, MapPaddingPx, ArenaHalfExtent, RectOrigin, RectSize);

	const float NewRadiusUU = FSiegeWarMapProjection::MapLocalPxToWorldRadius(NewRadiusPx, ArenaHalfExtent, RectSize);

	// ⛔ BY NUMBER, not by index - the M-1 discipline again, on the third verb.
	return Marks->SetMarkRadius(Circle.Number, NewRadiusUU);
}

// ---------------------------------------------------------------------------
// The painter (WR-§6's rendering row)
// ---------------------------------------------------------------------------

int32 UWarMapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bMapOpen)
	{
		// Belt and braces: a Collapsed widget is not painted anyway, but this class is the
		// only thing that guarantees a closed map draws nothing.
		return MaxLayer;
	}

	const FVector2f PanelSizeF = AllottedGeometry.GetLocalSize();
	const FVector2D PanelSize(PanelSizeF.X, PanelSizeF.Y);
	const FVector2D ArenaHalfExtent = ResolveArenaHalfExtent();

	FVector2D RectOrigin = FVector2D::ZeroVector;
	FVector2D RectSize = FVector2D::ZeroVector;
	FSiegeWarMapProjection::ComputeMapRectLocal(PanelSize, MapPaddingPx, ArenaHalfExtent, RectOrigin, RectSize);

	if (RectSize.X <= 0.0 || RectSize.Y <= 0.0)
	{
		return MaxLayer;
	}

	// ⛔ THE TEAM PALETTE IS READ FROM THE SHIPPED OWNER, ⛔ NEVER RE-TYPED (WR-§6's ally and
	// enemy dot rows). The accessors return the owner's own class defaults (W4-R5), so blue
	// here is byte-identical to blue on every health bar in the game and a future palette
	// change reaches this map with no edit. Same structural-escape reasoning as
	// ResolveArenaHalfExtent, applied to a colour.
	const FLinearColor AllyColor = UCombatantHealthBarComponent::GetDefaultBlueBarColor();
	const FLinearColor EnemyColor = UCombatantHealthBarComponent::GetDefaultRedBarColor();

	// ⭐ TASK-684: the elevation background takes the LOWEST native layer (WM-§2's draw-order
	// law: above the WBP panel Super just painted, below everything the map itself says).
	// ⭐ TASK-685 FILLS THE RESERVED SLOT: the POI icons sit between the elevation and the
	// unit dots (WM-§2's full order: elevation < icons < dots < markers < chrome), and every
	// shipped layer above the slot moves up ONE - ORDER PRESERVED, exactly the TASK-684
	// precedent. The SEVEN ResolvePlace markers stay the topmost solids, so a marker draws
	// ON TOP of any coincident POI icon (spec item 3) - and stay the ONLY thing the hit test
	// reads: nothing painted on PoiIconLayer exists to NativeOnMouseButtonDown.
	// ⭐ TASK-745 TAKES **TWO** SLOTS AND SPLITS THEM, WHICH IS THE ONE PLACE THIS FEATURE
	// DEPARTS FROM THE ONE-LAYER-PER-WAVE PRECEDENT - so the reason is written down:
	//   • THE RING is ground ANNOTATION and belongs UNDER the content it annotates. Drawn above
	//     the elevation but below the icons and the dots, so a circle can never hide the units
	//     and fixtures the player opened the map to read.
	//   • THE NUMBER is the mark's IDENTITY - it is literally the name the player speaks to his
	//     commander ("hold 1") - so it may NOT be buried under a unit dot that happens to sit at
	//     a circle's centre. It is drawn above every dot.
	// ⛔ BOTH stay BELOW MarkerLayer, and that is not a preference: it keeps the paint order and
	// the HIT order identical (markers are tested first, §8(c)), so the seven place markers
	// remain both the topmost solids AND the first thing a click finds. Every shipped layer
	// above each inserted slot moves up - ORDER PRESERVED, the TASK-684/685 precedent.
	const int32 ElevationLayer = MaxLayer + 1;
	const int32 MarkRingLayer = MaxLayer + 2;
	const int32 PoiIconLayer = MaxLayer + 3;
	const int32 AllyLayer = MaxLayer + 4;
	const int32 EnemyLayer = MaxLayer + 5;
	const int32 MarkNumberLayer = MaxLayer + 6;
	const int32 MarkerLayer = MaxLayer + 7;
	const int32 LabelLayer = MaxLayer + 8;

	// Null ⇒ never baked or bake failed ⇒ this frame looks exactly like the pre-TASK-684 map
	// (the WBP's flat panel shows through). ⛔ The painter never bakes - it is const, and the
	// guarantee that painting cannot change what is painted is load-bearing here: the bake
	// runs on OpenMap and the refresh timer only.
	if (ElevationTexture != nullptr)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			ElevationLayer,
			// FVector2f at the Slate boundary, the standing PaintQuad discipline.
			AllottedGeometry.ToPaintGeometry(
				FVector2f(static_cast<float>(RectSize.X), static_cast<float>(RectSize.Y)),
				FSlateLayoutTransform(FVector2f(static_cast<float>(RectOrigin.X), static_cast<float>(RectOrigin.Y)))),
			&ElevationBrush,
			ESlateDrawEffect::None,
			FLinearColor::White);
	}

	// ═══ MAP MARKS - the rings (TASK-745; MARK-§) ════════════════════════════════════════
	// ⭐ THE SAME BUILDER EVERY MARK VERB CALLS. What is drawn here is exactly what a click, a
	// right-click and a wheel notch will hit, because all four come out of BuildMarkCircles
	// against the same geometry (WR-§6's rendering ruling, on a feature with four verbs).
	// ⛔ Built ONCE and reused by the number pass below - one store read, not two.
	TArray<FSiegeWarMapMarkCircle> MarkCircles;
	BuildMarkCircles(AllottedGeometry, MarkCircles);

	for (const FSiegeWarMapMarkCircle& Circle : MarkCircles)
	{
		// ⭐ THE HOVER BOOST IS THE ONLY THING THAT MAKES "hover + wheel" VISIBLE. Without it the
		// player cannot tell which of two overlapping circles the wheel will resize, and the
		// first thing he learns is that the wheel picks one at random. ⛔ Cosmetic only - the
		// resize target is recomputed from the wheel event's own cursor position.
		const float Thickness = MarkRingThicknessPx
			+ ((Circle.Number == HoveredMarkNumber) ? SiegeWarMap::MarkHoverThicknessBoostPx : 0.f);

		// ⭐⛔ RIM FIRST, COLOUR ON TOP - the marker glyph's shipped pattern, and the file's own
		// measured finding applied: a FLAT colour cannot clear 3:1 against BOTH ends of this
		// ramp's 14.74:1 span (3.84:1 is the ceiling for any of them), so the repair is an
		// OUTLINE and ⛔ not another re-tint. The rim is the SHIPPED MarkerOutlineColor - reused,
		// ⛔ never re-typed, so there is one fewer colour constant whose SPACE could be wrong.
		SiegeWarMap::PaintCircleRing(
			OutDrawElements, MarkRingLayer, AllottedGeometry, Circle.LocalCentre,
			Circle.LocalRadiusPx, Thickness + 2.f, SiegeWarMap::MarkerOutlineColor);

		SiegeWarMap::PaintCircleRing(
			OutDrawElements, MarkRingLayer, AllottedGeometry, Circle.LocalCentre,
			Circle.LocalRadiusPx, Thickness, SiegeWarMap::MarkRingColor);
	}

	// ═══ The POI icon layer (TASK-685; WM-§1) ════════════════════════════════════════════
	// DISPLAY ONLY. Positions are the census the refresh timer read off the live world
	// actors (RefreshPoiIcons); the projection is the SHIPPED WorldToMapUV → MapUVToLocal
	// chain - byte-identical to the dots', so an icon and a dot can never disagree about
	// where the arena is (⛔ the transform is never re-derived here or anywhere). A null
	// brush (icon cleared / unresolved) degrades to the tinted dot primitive inside
	// PaintPoiIcon - drawn, never skipped.
	{
		const FSlateBrush* const MineBrush =
			(MineIconResolvedTexture != nullptr) ? &MineIconBrush : nullptr;
		const FSlateBrush* const GroundBrush =
			(AncientGroundIconResolvedTexture != nullptr) ? &AncientGroundIconBrush : nullptr;
		const FSlateBrush* const CastleBrush =
			(CastleIconResolvedTexture != nullptr) ? &CastleIconBrush : nullptr;

		for (const FVector2D& MineWorldXY : MinePoiWorldXY)
		{
			SiegeWarMap::PaintPoiIcon(
				OutDrawElements, PoiIconLayer, AllottedGeometry,
				FSiegeWarMapProjection::MapUVToLocal(
					FSiegeWarMapProjection::WorldToMapUV(MineWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
				SiegeWarMap::PoiIconDrawHalfSizePx, MineBrush, SiegeWarMap::MineIconTint);
		}

		for (const FVector2D& GroundWorldXY : AncientGroundPoiWorldXY)
		{
			SiegeWarMap::PaintPoiIcon(
				OutDrawElements, PoiIconLayer, AllottedGeometry,
				FSiegeWarMapProjection::MapUVToLocal(
					FSiegeWarMapProjection::WorldToMapUV(GroundWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
				SiegeWarMap::PoiIconDrawHalfSizePx, GroundBrush, SiegeWarMap::AncientGroundIconTint);
		}

		// ⛔ CASTLE TINTS ARE THE W4-R5 ACCESSOR VALUES AND NOTHING ELSE - AllyColor /
		// EnemyColor above ARE GetDefaultBlueBarColor() / GetDefaultRedBarColor(), reused
		// here keyed by the CASTLE'S OWN TEAM (the census split the lists by team): a blue
		// castle wears the blue accessor's colour on every client, red the red's - J2's
		// "castles blue/red via the team palette", with zero re-typed literals.
		for (const FVector2D& CastleWorldXY : BlueCastlePoiWorldXY)
		{
			SiegeWarMap::PaintPoiIcon(
				OutDrawElements, PoiIconLayer, AllottedGeometry,
				FSiegeWarMapProjection::MapUVToLocal(
					FSiegeWarMapProjection::WorldToMapUV(CastleWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
				SiegeWarMap::PoiIconDrawHalfSizePx, CastleBrush, AllyColor);
		}

		for (const FVector2D& CastleWorldXY : RedCastlePoiWorldXY)
		{
			SiegeWarMap::PaintPoiIcon(
				OutDrawElements, PoiIconLayer, AllottedGeometry,
				FSiegeWarMapProjection::MapUVToLocal(
					FSiegeWarMapProjection::WorldToMapUV(CastleWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
				SiegeWarMap::PoiIconDrawHalfSizePx, CastleBrush, EnemyColor);
		}
	}

	for (const FVector2D& AllyWorldXY : AllyDotsWorldXY)
	{
		SiegeWarMap::PaintQuad(
			OutDrawElements, AllyLayer, AllottedGeometry,
			FSiegeWarMapProjection::MapUVToLocal(
				FSiegeWarMapProjection::WorldToMapUV(AllyWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
			// ⭐ TASK-685 (WM-§3): the promoted, bumped AllyDotRadius tunable - no longer the
			// file-local literal the enemy loop below still (deliberately) uses.
			AllyDotRadius, AllyColor);
	}

	// ⛔ RED DOTS EXIST ONLY WHILE A PAID REVEAL IS HELD. An empty array draws nothing; there
	// is no "show them faintly" state and no decay (WR-§6's enemy-dot row).
	for (const FVector2D& EnemyWorldXY : EnemyDotsWorldXY)
	{
		SiegeWarMap::PaintQuad(
			OutDrawElements, EnemyLayer, AllottedGeometry,
			FSiegeWarMapProjection::MapUVToLocal(
				FSiegeWarMapProjection::WorldToMapUV(EnemyWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
			SiegeWarMap::DotDrawHalfSizePx, EnemyColor);
	}

	// ═══ MAP MARKS - the centred numbers (TASK-745; MARK-§2) ═════════════════════════════
	//
	// ⭐⭐ "Every time you make a new circle, it gets its own number in the middle of it" - his
	// sentence, and the whole feature's identity. ⛔ THE PLAYER SEES `1`; THE MODEL SEES
	// `circle_1`; ⛔ NEITHER MAY EVER BE SHOWN IN THE OTHER'S PLACE (MARK-§2). Nothing in this
	// block ever renders a symbol, and nothing in the pick path ever inserts a bare digit.
	//
	// ── HOW IT STAYS LEGIBLE AT **ANY** RADIUS, WHICH IS TWO INDEPENDENT MECHANISMS ──
	//  (1) SIZE follows the ring: FontSize = Radius x MarkNumberFontRadiusFraction, CLAMPED to
	//      [MarkNumberFontMinSize, MarkNumberFontMaxSize]. ⇒ a circle scrolled to its 12-px
	//      floor still carries a readable 10-pt digit, and one scrolled to its 240-px ceiling
	//      carries a 28-pt digit instead of a 144-pt one stamped across the battlefield.
	//  (2) CONTRAST comes from an OUTLINE, ⛔ not from the fill colour, and that is this file's
	//      OWN measured finding rather than a guess: the three overlays that fail the 3:1 gate
	//      are EXACTLY the three drawn with no outline, and "no flat colour can clear 3:1
	//      against BOTH ends of a 14.74:1 background by luminance alone, so the repair is an
	//      OUTLINE." ⇒ the fill is the shipped MarkerLabelColor and the outline the shipped
	//      MarkerOutlineColor - the identical pairing that puts the marker GLYPH at 12.22:1 -
	//      and the digit therefore reads over the ramp's dark end, its light end, a castle
	//      shell clamped white, and its own magenta ring alike.
	if (!MarkCircles.IsEmpty())
	{
		// ⚠️ THE MEASURE SERVICE IS THE ONLY WAY TO CENTRE TEXT IN SLATE, and the seven place
		// markers' labels are deliberately LEFT-ANCHORED beside their glyph precisely to avoid
		// needing it. This feature pays that cost because "in the middle of it" is Jonathan's
		// contract, ⛔ not a layout preference.
		//
		// ⛔ THE GUARD IS DEFENSIVE AND STRUCTURALLY UNREACHABLE FROM A PAINT PASS - said
		// plainly rather than dressed up as a real degrade path: NativePaint is invoked only by
		// SObjectWidget::OnPaint, which cannot run without a live Slate application and a
		// renderer. It is here because FSlateApplicationBase::Get() check()s, and a check() in
		// a painter is a crash where a missing digit would do.
		const FSlateRenderer* const Renderer =
			FSlateApplicationBase::IsInitialized() ? FSlateApplicationBase::Get().GetRenderer() : nullptr;

		if (Renderer != nullptr)
		{
			const TSharedRef<FSlateFontMeasure> FontMeasure = Renderer->GetFontMeasureService();

			for (const FSiegeWarMapMarkCircle& Circle : MarkCircles)
			{
				if (Circle.Number <= 0 || !(Circle.LocalRadiusPx > 0.f))
				{
					// A degenerate circle draws no ring (PaintCircleRing refuses it) and must
					// draw no number either - a floating digit with nothing around it would be
					// the picture disagreeing with the hit test.
					continue;
				}

				// ⭐ THE SIZE RULE IS THE PURE, TESTED SEAM - ⛔ not a clamp inlined here where
				// no test could sweep it across the radius range (W4-R1, the same reason
				// CentreTextTopLeft below is a function).
				const float FontSize = FSiegeWarMapProjection::MarkNumberFontSizePx(
					Circle.LocalRadiusPx, MarkNumberFontRadiusFraction,
					MarkNumberFontMinSize, MarkNumberFontMaxSize);

				// ⚠️ "Bold", ⛔ not the markers' "Regular": a single digit inside a ring has no
				// neighbouring letters to give it word-shape, so weight is what carries it at
				// the 10-pt floor. The FFontOutlineSettings overload is FCoreStyle's own
				// (CoreStyle.h:51) - ⛔ the outline is part of the FONT, never a second MakeText
				// pass offset by a pixel, which is the hand-rolled shadow trick that shows its
				// seams at large sizes.
				const FSlateFontInfo NumberFont = FCoreStyle::GetDefaultFontStyle(
					TEXT("Bold"),
					FontSize,
					FFontOutlineSettings(SiegeWarMap::MarkNumberOutlineSizePx, SiegeWarMap::MarkerOutlineColor));

				// ⛔ THE NUMBER, ⛔ NOT THE SYMBOL. FString::FromInt, so `1` - never `circle_1`.
				const FString NumberText = FString::FromInt(Circle.Number);

				// FVector2f at the Slate boundary - Measure returns a FDeprecateVector2DResult,
				// which IS an FVector2f (the GetLocalSize() idiom above, verbatim).
				const FVector2f MeasuredF = FontMeasure->Measure(NumberText, NumberFont);

				// ⭐ THE CENTRING RULE IS THE PURE, TESTED SEAM - ⛔ not arithmetic inlined here
				// where no test could ever read it (W4-R1).
				const FVector2D TopLeft = FSiegeWarMapProjection::CentreTextTopLeft(
					Circle.LocalCentre, FVector2D(MeasuredF.X, MeasuredF.Y));

				FSlateDrawElement::MakeText(
					OutDrawElements,
					MarkNumberLayer,
					AllottedGeometry.ToPaintGeometry(
						PanelSizeF,
						FSlateLayoutTransform(FVector2f(
							static_cast<float>(TopLeft.X),
							static_cast<float>(TopLeft.Y)))),
					NumberText,
					NumberFont,
					ESlateDrawEffect::None,
					SiegeWarMap::MarkerLabelColor);
			}
		}
	}

	// ⭐ THE SAME BUILDER THE HIT TEST CALLS. What is painted below is exactly what
	// NativeOnMouseButtonDown will test against, because both come out of this one function.
	TArray<FSiegeWarMapMarker> Markers;
	BuildMarkerRects(AllottedGeometry, Markers);

	const FSlateFontInfo LabelFont =
		FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SiegeWarMap::MarkerLabelFontSize);

	for (const FSiegeWarMapMarker& Marker : Markers)
	{
		// Outline first, glyph on top: a bright marker on a dark rim reads over any
		// background WBP_WarMap supplies (the M5.5 white-on-white lesson).
		SiegeWarMap::PaintQuad(
			OutDrawElements, MarkerLayer, AllottedGeometry, Marker.LocalCentre,
			SiegeWarMap::MarkerDrawHalfSizePx + 2.f, SiegeWarMap::MarkerOutlineColor);

		SiegeWarMap::PaintQuad(
			OutDrawElements, MarkerLayer, AllottedGeometry, Marker.LocalCentre,
			SiegeWarMap::MarkerDrawHalfSizePx, SiegeWarMap::MarkerColor);

		// ⭐ THE LABEL IS THE RAW SYMBOL, and that is deliberate rather than lazy: what the
		// player reads on the map is character-for-character what a click inserts into the
		// console, so the affordance teaches the vocabulary instead of hiding it (WR-§9
		// outcome 2, and the DEV-01 reasoning behind it).
		//
		// ⚠️ LEFT-ANCHORED beside the marker, ⛔ NOT centred under it. Centring needs the
		// font measure service; anchoring needs nothing, cannot be wrong, and is the ordinary
		// map convention. Overflow near the right edge is bounded by MapPaddingPx.
		const FVector2f LabelOffset(
			static_cast<float>(Marker.LocalCentre.X) + SiegeWarMap::MarkerDrawHalfSizePx + SiegeWarMap::MarkerLabelGapPx,
			static_cast<float>(Marker.LocalCentre.Y) - SiegeWarMap::MarkerLabelFontSize * 0.6f);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LabelLayer,
			AllottedGeometry.ToPaintGeometry(PanelSizeF, FSlateLayoutTransform(LabelOffset)),
			Marker.PlaceSymbol.ToString(),
			LabelFont,
			ESlateDrawEffect::None,
			SiegeWarMap::MarkerLabelColor);
	}

	return FMath::Max(MaxLayer, LabelLayer);
}

// ---------------------------------------------------------------------------
// The hit test - CLICK → SYMBOL, never CLICK → COORDINATE (WR-§6)
// ---------------------------------------------------------------------------

FReply UWarMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bMapOpen)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2f LocalPointF = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D LocalPoint(LocalPointF.X, LocalPointF.Y);

	// ⭐ TASK-745 - THE RIGHT BUTTON NOW DELETES A MAP MARK (MARK-§; Jonathan: "You can right
	// click to delete that circle").
	//
	// ⛔ IT IS HANDLED **BEFORE** THE NON-LEFT ABSORB BELOW AND ⛔ NEVER INSTEAD OF IT: a
	// right-click that hits no circle still returns Handled and still does nothing, so the
	// shipped "every button is absorbed while the map is open" property is byte-for-byte intact.
	// ⚠️ The absorb is what stops a click falling through a full-screen panel and issuing a real
	// order at a real world position - "the map made my army walk somewhere" - and it is also
	// why the RIGHT button is safe to spend here: it already went nowhere.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// ⛔ ITS RETURN VALUE IS DELIBERATELY DISCARDED. There is exactly one behaviour for a
		// right-click on empty map, on a mark-less map, and on a map with no store at all:
		// NOTHING, silently. ⛔ No status line, ⛔ no hint, ⛔ no nearest-circle guess.
		TryDeleteMarkAtLocal(InGeometry, LocalPoint);
		return FReply::Handled();
	}

	// ⚠️ EVERY OTHER BUTTON IS ABSORBED WHILE THE MAP IS OPEN - shipped, unchanged. The map
	// fills the screen, so a fall-through click would land on the WORLD behind it and issue a
	// real order at a real position. Absorbing is the fail-safe direction; only the LEFT button
	// names or places anything.
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	// ⭐ ONE SOURCE OF TRUTH: the same builder, against the geometry Slate just handed this
	// event. What the player can SEE and what the player can CLICK cannot disagree.
	TArray<FSiegeWarMapMarker> Markers;
	BuildMarkerRects(InGeometry, Markers);

	const int32 HitIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, LocalPoint);

	if (HitIndex == INDEX_NONE)
	{
		// ⭐⭐ TASK-745 - THE MARK LANE, AND ITS PRECEDENCE IS DELIBERATE AND LOAD-BEARING.
		//
		// ⛔ THE SEVEN PLACE MARKERS WERE TESTED FIRST, ABOVE, AND THEY WIN. TASK-745's spec
		// lists "the seven place markers' hit-testing and symbol insertion" among the shipped
		// things it may not disturb - and a mark is an arbitrarily large disc the player can
		// drop anywhere, so if marks won, one big circle over `own_castle` would silently make
		// that marker unclickable for the rest of the match. The PAINT order matches (marks
		// below markers), so what you see and what you click still agree.
		//
		// ⇒ Only now, on ground that named no marker:
		//    (a) inside an existing MARK  ⇒ insert `circle_N` - the whole point of the feature;
		//    (b) on genuinely empty map   ⇒ PLACE a new mark at the lowest free number;
		//    (c) mark lane unavailable    ⇒ the SHIPPED empty-click arm, verbatim.
		{
			TArray<FSiegeWarMapMarkCircle> MarkCircles;
			BuildMarkCircles(InGeometry, MarkCircles);

			const int32 MarkIndex = FSiegeWarMapProjection::FindMarkIndexAtLocal(MarkCircles, LocalPoint);
			if (MarkIndex != INDEX_NONE)
			{
				// ⛔⛔ THE SYMBOL, AND ONLY THE SYMBOL, LEAVES THIS CLASS - on the SAME delegate
				// the seven markers already use, so TASK-563's binder needs ⛔ not one line of
				// change and the console seam (ComposeAppendedInput) is untouched. ⛔ No
				// position, no radius, no count and no circle geometry rides along.
				const int32 PickedNumber = MarkCircles[MarkIndex].Number;
				const FName PickedMarkSymbol = MakeMarkPickSymbol(PickedNumber);

				if (!PickedMarkSymbol.IsNone())
				{
					OnPlacePicked.Broadcast(PickedMarkSymbol);

					// ⛔ THE STATUS LINE ECHOES THE SYMBOL, ⛔ not the number - and that is the
					// ONE place the two renderings deliberately swap roles (MARK-§2). The line
					// is telling the player what was just written into his input box, so it must
					// show him exactly the text he is about to send; the NUMBER is what is drawn
					// on the map. Same identity, two renderings, each in its own place.
					SetStatusLine(PickedMarkSymbol.ToString());

					UE_LOG(LogSiegeWarMap, Verbose,
						TEXT("[WarMap] Map mark picked: number %d -> symbol '%s' (symbol only - no coordinate leaves this widget)."),
						PickedNumber, *PickedMarkSymbol.ToString());
				}

				return FReply::Handled();
			}

			if (TryPlaceMarkAtLocal(InGeometry, LocalPoint))
			{
				// Placed, or refused at the cap with a line naming the cap. Either way the
				// click was ANSWERED, so the shipped hint below must not overwrite it.
				return FReply::Handled();
			}
		}

		// ⛔ A CLICK ON EMPTY MAP DOES NOTHING beyond one line of static chrome naming what IS
		// clickable (WR-§9 outcome 1 - a DESIGNED outcome, on Jonathan's playtest sheet, so a
		// report of it is read correctly). ⛔ It does NOT invent a coordinate, a grid cell, a
		// snap radius or a nearest-marker guess: the vocabulary has no primitive for an
		// arbitrary point, and manufacturing one is the exact thing AS-§21.4 reserves for
		// Jonathan.
		//
		// ⚠️⚠️ TASK-745 (MARK-§) DEMOTED THIS ARM TO A **DEGRADE PATH**, AND THE CHANGE IS
		// DECLARED RATHER THAN SLIPPED IN (SC-§15; WarMapWidget.h §2's amendment block): by
		// Jonathan's own directive an empty-map LEFT click now PLACES A NUMBERED MARK, so
		// WR-§9 outcome 1 is no longer true of the LEFT button. ⛔ IT IS STILL TRUE OF THE
		// RIGHT BUTTON, and it is still true HERE - this arm is now reached only when the mark
		// lane is UNAVAILABLE (no owning local player ⇒ no USiegeMapMarkSubsystem) or the click
		// landed OUTSIDE the drawn map rect (the letterbox, where LocalToMapUV refuses rather
		// than clamping a mark onto ground the player never pointed at). ⇒ the retired
		// behaviour survives as REACHABLE CODE with a real reason to run, ⛔ not as a deleted
		// branch nobody can audit - and the airlock argument above is untouched either way,
		// because the mark lane also emits a SYMBOL and never a coordinate.
		//
		// ⚠️ TASK-579 MOVED THIS DISCRIMINATOR FROM Markers.Num() TO THE SNAPSHOT POINTER;
		// ⭐ TASK-685 (ruling W691-3) RE-POINTS IT TO "DID ANY PLACE RESOLVE". The pointer
		// test aimed at a state TASK-691 proved unreachable (the snapshot exists from
		// BeginPlay, cd5f4ed), so every pre-sentence empty click answered "Click a marked
		// place to add its name to the console." over a map with ZERO markers - VID-003's
		// observed string, a remedy that cannot work, i.e. the status line lying to him.
		// GetPlaceNames() is the very list BuildMarkerRects consumes, so this arm and the
		// marker layer cannot disagree. ⛔ Deliberately NOT re-pointed back to Markers.Num():
		// a degenerate-panel frame would then claim "no places surveyed" - the wrong cause
		// with the wrong remedy, TASK-579's original argument, still standing. The
		// null-guard stays as defensive code and short-circuits into the SAME arm (W691-3).
		{
			const USiegeAssistantSnapshot* const StatusSnapshot = GetReadOnlySnapshot();
			if (StatusSnapshot == nullptr || StatusSnapshot->GetPlaceNames().Num() == 0)
			{
				ShowNoSnapshotHint();
			}
			else
			{
				SetStatusLine(SiegeWarMap::EmptyClickHintText);
			}
		}

		return FReply::Handled();
	}

	const FName PickedSymbol = Markers[HitIndex].PlaceSymbol;

	// ⛔⛔ THE SYMBOL, AND ONLY THE SYMBOL, LEAVES THIS CLASS. No position, no dot, no count
	// and no marker rect rides along, so nothing this widget computed can ever reach a prompt
	// zone (WR-§6). The binder hands it to the console's input box; THE PLAYER STILL SENDS IT.
	OnPlacePicked.Broadcast(PickedSymbol);

	// Chrome only - it echoes the symbol the player just chose. ⚠️ It is a UI line, never a
	// prompt string and never a game-authored sentence about an order.
	SetStatusLine(PickedSymbol.ToString());

	UE_LOG(LogSiegeWarMap, Verbose, TEXT("[WarMap] Place marker picked: '%s' (symbol only - no coordinate leaves this widget)."), *PickedSymbol.ToString());

	return FReply::Handled();
}

// ---------------------------------------------------------------------------
// ═══ THE WHEEL - hover + scroll resizes a mark (TASK-745; MARK-§4) ═══
//
// ⛔⛔ READ MARK-§4 BEFORE TOUCHING THIS FUNCTION. The shipped clause it amends
// says the wheel "must stay INERT outside the pick flow", and the amendment is
// safe for ONE structural reason, verified at the source rather than assumed:
//
//   • THE CONTROLLER'S WHEEL IS A **POLL** - WasInputKeyJustPressed(EKeys::
//     MouseScrollUp/Down) inside ASiegePlayerController::ApplyGroupPickWheel,
//     whose ONLY call site is PlayerTick's `GroupPickStage != EGroupPickStage::
//     None` branch (SiegePlayerController.cpp:647-657 and :2843-2876).
//   • THIS ONE IS A **SLATE EVENT** on a focused widget.
//
// ⇒ ⛔ NOT ONE LINE, ⛔ NOT ONE SYMBOL AND ⛔ NOT ONE FILE OF THE CONTROLLER'S
// PATH IS TOUCHED, and its "inert outside the pick flow" property is LITERALLY
// unchanged. ⛔ THE WHEEL NOW HAS EXACTLY TWO CONSUMERS AND ⛔ NO THIRD MAY BE
// ADDED WITHOUT AMENDING MARK-§4 BY NAME.
// ---------------------------------------------------------------------------

FReply UWarMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bMapOpen)
	{
		// ⛔ CLOSED ⇒ THIS WIDGET IS Collapsed AND THE EVENT NEVER REACHES IT AT ALL, so this is
		// belt and braces in the NativePaint sense. It matters anyway: it is the line that
		// guarantees, in code a reviewer can point at, that a closed map cannot consume a wheel
		// notch the controller's group-pick poll is waiting for.
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	const FVector2f LocalPointF = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D LocalPoint(LocalPointF.X, LocalPointF.Y);

	// ⛔ ITS RETURN VALUE IS DELIBERATELY DISCARDED, exactly as the right-click's is: a wheel
	// notch that is not over a mark changes NO state anywhere - ⛔ not the nearest circle, ⛔ not
	// the last-touched one, ⛔ not a map zoom (there is none, and adding one would be the third
	// consumer MARK-§4 forbids). That is "INERT" in full.
	TryResizeMarkAtLocal(InGeometry, LocalPoint, InMouseEvent.GetWheelDelta());

	// ⚠️⚠️ ABSORBED WHILE OPEN, EVEN ON A MISS, AND IT IS A DECISION RATHER THAN AN OVERSIGHT.
	// ⚖️ It is the SAME fail-safe direction and the SAME argument as the shipped
	// NativeOnMouseButtonDown, which already absorbs every mouse BUTTON while the map is up: the
	// map fills the screen, so letting input fall through would drive something the player
	// cannot see - here, a group-pick circle sitting behind a full-screen panel. MARK-§4's own
	// wording contemplates it ("UWarMapWidget while the map is open and the cursor is over it")
	// and it adds ⛔ NO third consumer: absorbing is the same one consumer declining to act.
	// ⚠️ THE ALTERNATIVE, RECORDED SO IT IS NOT RE-ARGUED FROM SCRATCH: returning Unhandled on a
	// miss would let the notch reach the controller's poll. It was refused because it makes the
	// map's WHEEL behave differently from the map's CLICKS - the same gesture over the same
	// pixels falling through or not depending on which button it is - and inconsistency here is
	// exactly how a player learns to distrust a UI.
	return FReply::Handled();
}

FReply UWarMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bMapOpen)
	{
		// ⭐ THE HOVER HIGHLIGHT - the affordance that makes "hover + wheel" visible. ⛔ A
		// NUMBER, ⛔ not an index (M-1: indices shift when a mark is deleted, numbers never do).
		TArray<FSiegeWarMapMarkCircle> Circles;
		BuildMarkCircles(InGeometry, Circles);

		const FVector2f LocalPointF = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const int32 HitIndex = FSiegeWarMapProjection::FindMarkIndexAtLocal(
			Circles, FVector2D(LocalPointF.X, LocalPointF.Y));

		HoveredMarkNumber = (HitIndex != INDEX_NONE) ? Circles[HitIndex].Number : 0;
	}

	// ⛔ NOT ABSORBED, and that is the opposite decision from the wheel and the buttons above -
	// deliberately. A MOVE is not a command: swallowing moves would break hover, tooltips and
	// button highlighting on every optional child WBP_WarMap supplies, to buy nothing at all
	// (a move that falls through to the world drives nothing).
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UWarMapWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	// ⛔ A HIGHLIGHT THAT OUTLIVES THE CURSOR IS A LIE ABOUT WHAT THE WHEEL WOULD RESIZE.
	HoveredMarkNumber = 0;

	Super::NativeOnMouseLeave(InMouseEvent);
}

// ---------------------------------------------------------------------------
// Chrome
// ---------------------------------------------------------------------------

void UWarMapWidget::SetStatusLine(const FString& Line)
{
	// ⭐⛔ THE LATCH IS RETIRED BY EVERY LINE, AND RE-ARMED ONLY BY ShowNoSnapshotHint() BELOW,
	// WHICH RE-ARMS IT *AFTER* CALLING THIS (TASK-579). ⚖️ Written this way so the invariant is
	// enforced by CONSTRUCTION rather than by remembering: bShowingNoSnapshotHint means "the
	// hint is the line ON SCREEN RIGHT NOW", and with one writer for false and one for true it
	// cannot drift out of agreement with what StatusTextBlock actually holds. The alternative -
	// an assignment beside each of the four call sites - is exactly the shape that goes stale
	// the first time somebody adds a fifth.
	bShowingNoSnapshotHint = false;

	if (StatusTextBlock)
	{
		StatusTextBlock->SetText(FText::FromString(Line));
	}

	// Fires whether or not the optional child is bound, so a WBP that names its status line
	// something else can still present it - the USessionMenuWidget both-routes contract.
	OnWarMapStatusLine(Line);
}

void UWarMapWidget::ShowNoSnapshotHint()
{
	SetStatusLine(SiegeWarMap::NoSnapshotStatusText);

	// ⛔ AFTER, NEVER BEFORE - SetStatusLine clears this by design (see its comment). Arming it
	// first would be silently undone and the hint would never retire.
	bShowingNoSnapshotHint = true;
}

void UWarMapWidget::UpdateNoSnapshotHint()
{
	// ⛔ THE BOOL IS TESTED FIRST SO THE COMMON CASE READS NOTHING AT ALL. Once the hint has
	// retired - and once the snapshot lists a place it never comes back this open - this
	// function costs one branch per timer tick and never touches the controller, the
	// component or the snapshot again.
	if (!bShowingNoSnapshotHint)
	{
		return;
	}

	// ⛔ STILL A READ, STILL NOT A SURVEY. Same GetTurnSnapshot() route as everywhere else in
	// this file: no Capture(), no EnsureSnapshot(), nothing that reaches either. This function
	// WAITS for places the player's own sentence (or, once TASK-580 lands, the component's
	// own at-rest seed) resolves; it does not resolve any.
	//
	// ⭐ RE-POINTED BY TASK-685 (W691-3): the retire condition now MIRRORS the arming
	// condition - "at least one place listed", not "a snapshot object exists". ⚠️ The old
	// pointer test here was DEAD CODE walking: TASK-691 proved the snapshot is non-null from
	// BeginPlay, so OpenMap's old pointer discriminator NEVER ARMED the hint and this
	// function's early-out bool never let it run - the "monotonic null→non-null" proof the
	// old comment carried was correct about a transition that happens at match start, before
	// any open, watched by a latch that could never be armed (TASK-691 §5's finding,
	// corrected here per the W691-3 rider).
	const USiegeAssistantSnapshot* const Snapshot = GetReadOnlySnapshot();
	if (Snapshot == nullptr || Snapshot->GetPlaceNames().Num() == 0)
	{
		return;
	}

	// ⛔ THE HINT MUST DISAPPEAR: it says "send an order and the markers appear", the markers
	// have now appeared (GetPlaceNames() is the marker builder's own list), and a hint that
	// outlives its own remedy tells the player the fix did not work. ⚠️ ONE-WAY: this never
	// re-shows the hint mid-open, so it can never overwrite a symbol the player clicked one
	// tick earlier.
	//
	// ⭐ THE LATCH DISCIPLINE, RE-ARGUED FOR THE NEW WATCHED TRANSITION (the W691-3 rider
	// asks for the argument, not an assertion): the transition is now "zero places listed →
	// some place listed". Within one open that is monotonic on every shipped path -
	// PlaceNames is filled only by Capture()'s resolved-slot loop, and the places that
	// resolve are structural fixtures (castles, grounds, hero), so a later capture listing
	// ZERO places after one listed some has no shipped route. And if a future regression
	// invented one, the latch fails SAFE, exactly as before: it only ever RETIRES the hint
	// (one writer for true, one for false - the SetStatusLine invariant is untouched), the
	// status line keeps its last text, and the next OpenMap re-evaluates the discriminator
	// from scratch. ⛔ No per-frame work was added: the bool early-out above is unchanged,
	// and the new census read costs one Num() on ticks where the hint is still up.
	SetStatusLine(SiegeWarMap::OpenStatusText);
}

void UWarMapWidget::HandleRevealButtonClicked()
{
	// ⛔ BROADCAST AND NOTHING ELSE. No cost is read, no balance is checked, no gold moves and
	// no RPC is sent from this file. TASK-563's controller owns EnemyRevealCost, the
	// authority check, ASiegePlayerState::SpendGold, the net-zero refusal and both RPCs
	// (WR-§7). ⛔ A second affordability check here would be a second economy rule to get
	// wrong, and the first one that drifted would refuse a purchase the authority allowed.
	OnRevealButtonClicked.Broadcast();
}

void UWarMapWidget::HandleCloseButtonClicked()
{
	CloseMap();
}
