// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

#include "WarMapWidget.generated.h"

class APlayerController;
class UButton;
class USiegeAssistantSnapshot;
class USiegeScatterConfig;
class UTextBlock;

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
	 *  ── ORIENTATION, PINNED SO A LATER READER DOES NOT RE-DECIDE IT ──
	 *    UV.X = 0 at world X = −HalfX  ⇒  world **+X grows RIGHT** on screen.
	 *    UV.Y = 0 at world Y = +HalfY  ⇒  world **+Y grows UP** on screen.
	 *
	 *  ⚠️ THE Y FLIP IS NOT A TASTE CALL: Slate's local Y grows DOWNWARD, so a map that did
	 *  not flip would render the battlefield MIRRORED about the castle axis, and every dot
	 *  would be on the wrong side of the lane while looking perfectly plausible. The long
	 *  arena axis is X (`ArenaHalfExtent` ships `(26000, 12000)`), which is also the
	 *  castle-to-castle axis, so X-horizontal puts the two castles left and right — the
	 *  reading a player expects of a battlefield map.
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
 *  `USiegeAssistantSnapshot::ResolvePlace` positions. ⛔ A click on empty map does NOTHING
 *  beyond one line of static chrome naming what IS clickable (`WR-§9` outcome 1 — a designed
 *  outcome, not a bug).
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
 *  ⚠️⚠️ **THE RESIDUAL, REPORTED RATHER THAN CODED AROUND — THIS IS THE TASK'S NAMED
 *  FINDING.** `GetTurnSnapshot()` is **null until the player's FIRST console sentence**
 *  (`Snapshot` is allocated in `EnsureSnapshot()`, reached only from the turn path). ⇒ **A
 *  player who opens the war map before ever typing into the console sees ally dots and enemy
 *  dots but NO PLACE MARKERS, and therefore has nothing to click.** The map is otherwise
 *  fully functional and self-heals the moment one sentence is sent.
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
 *  EXPLAIN ITSELF. IT DOES ⛔ NOT POPULATE IT.** The snapshot is still null on a first open,
 *  there are still no markers, and ⛔ nothing here calls `Capture()` or `EnsureSnapshot()`.
 *  What changed is that the player is TOLD, in the status line the map already owns, why the
 *  map has nothing to click and what single action fixes it.
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
 *  screen; the map's existing refresh timer clears it the moment a snapshot exists, and any
 *  other line (a picked symbol, the empty-click hint) clears it immediately. ⛔ No new timer,
 *  ⛔ no tick, ⛔ no per-frame log and ⛔ no `Warning` — a status line the player reads IS the
 *  whole mechanism.
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
 *  - **D8 whether the `SM_POI_01..04` props become nameable places.** ⛔ NOT ASSUMED. They
 *    have no symbol and no `ResolvePlace` entry, so they simply do not appear as markers.
 *  - **D9 whether the map pauses.** `WR-§6` rules NO PAUSE and this class obeys that ruling;
 *    it is recorded here as ruled-by-law rather than decided-by-programmer.
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
	 *  component, no snapshot yet (see §3), or a degenerate panel.
	 */
	void BuildMarkerRects(const FGeometry& AllottedGeometry, TArray<FSiegeWarMapMarker>& OutMarkers) const;

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

	/** Own-team `ASummonedUnit` + `AHeroCharacter` positions, refreshed on the timer while open. Cleared on close. */
	void RefreshAllyDots();

	/**
	 *  ⭐ THE ONE THING THE REFRESH TIMER CALLS: the ally sweep, then the hint latch (TASK-579).
	 *
	 *  ⚖️ ONE TIMER, TWO JOBS, AND THE ALTERNATIVE IS WORSE. A second timer for a bool that
	 *  flips ONCE per open would be a second lifetime to start, stop, clear on close and clear
	 *  again on teardown — the exact bookkeeping `NativeDestruct` already carries a
	 *  double-clear for. ⛔ And the latch costs nothing after it clears: `UpdateNoSnapshotHint`
	 *  early-outs on the bool BEFORE it reads anything.
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
	 *  RETIRES the hint, and only when the hint is what is on screen AND a snapshot now exists —
	 *  so it can never stomp a symbol the player just clicked, and never re-arms mid-open.
	 *
	 *  ⚠️ Bound above by the refresh interval (0.25 s by default), ⛔ not instantaneous — stated
	 *  rather than implied. The MARKERS themselves appear on the very next paint; only the line
	 *  of text lags, by at most one interval.
	 */
	void UpdateNoSnapshotHint();

	/** Auto-wire target for `RevealButton`. Broadcasts and nothing else — ⛔ it prices nothing. */
	UFUNCTION()
	void HandleRevealButtonClicked();

	/** Auto-wire target for `CloseButton`. */
	UFUNCTION()
	void HandleCloseButtonClicked();

	/** True while the map is on screen and hit-testable. */
	bool bMapOpen = false;

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

	/** Ally positions in WORLD `(X, Y)`. Projected at paint, never stored projected — a resize must not need a re-survey. */
	TArray<FVector2D> AllyDotsWorldXY;

	/** The paid reveal's FROZEN snapshot, in WORLD `(X, Y)`. ⛔ Does not track; ⛔ cleared on close, always (`WR-§7`). */
	TArray<FVector2D> EnemyDotsWorldXY;

	/** The ally refresh timer. Cleared in `CloseMap` and again in `NativeDestruct`. */
	FTimerHandle AllyDotTimerHandle;
};
