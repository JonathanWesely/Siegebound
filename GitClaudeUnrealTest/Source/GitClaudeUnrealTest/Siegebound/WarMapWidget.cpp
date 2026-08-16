// Copyright Epic Games, Inc. All Rights Reserved.

#include "WarMapWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
// ⚠️ EngineUtils.h is TActorIterator's home, and the ONLY iteration in this file is the
// ALLY DOT sweep. ⛔ There is no place iterator here and there must never be one:
// USiegeAssistantSnapshot::ResolvePlace is the SINGLE OWNER of place → position, and a
// second search would be a second answer that drifts (WR-§6).
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
// ⚠️ REQUIRED, NOT INHERITED — CHECKED, NOT ASSUMED. NativeOnMouseButtonDown compares against
// EKeys::LeftMouseButton, and EKeys lives here; UserWidget.h brings FPointerEvent but not the
// key table. The console widget's header carries the same include for the same reason.
#include "InputCoreTypes.h"
#include "TimerManager.h"

// ── Slate, for the C++ painter (WR-§6's rendering row) ──
// ⚠️ Both modules are ALREADY public dependencies of this module and neither is added by
// this task: GitClaudeUnrealTest.Build.cs lists "Slate" and "SlateCore" with a standing
// comment explaining that SlateCore is not implied by Slate. ⛔ No Build.cs change is owed.
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#include "Siegebound/CombatantHealthBarComponent.h" // the SHIPPED team palette - GetDefaultBlueBarColor() / GetDefaultRedBarColor()
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/ScatterConfig.h"               // USiegeScatterConfig::ArenaHalfExtent - the single owner
#include "Siegebound/SiegeAssistantComponent.h"     // GetTurnSnapshot() - READ-ONLY (WR-§6 snapshot hazard)
#include "Siegebound/SiegeAssistantSnapshot.h"      // GetPlaceNames() / ResolvePlace()
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
	 *  ⭐⛔ TASK-579's WHOLE DELIVERABLE IS THIS ONE STRING AND THE THREE PLACES IT IS DECIDED.
	 *
	 *  Shown when the map is open and the assistant has NO snapshot yet - the KNOWN GAP of
	 *  WR-§9 row 12, which is the ONE row on that list explicitly NOT a designed outcome. The
	 *  markers are absent because ResolvePlace lives on a snapshot the assistant allocates on
	 *  its turn path, so it does not exist until the player's first sentence.
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
	static const TCHAR* NoSnapshotStatusText = TEXT("No place markers yet - send your commander one order in the console and they appear. The console opens over this map, so you do not have to close it.");

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

	/** Half the drawn dot, in local px. */
	static constexpr float DotDrawHalfSizePx = 3.f;

	/** Gap between a marker's right edge and its label. */
	static constexpr float MarkerLabelGapPx = 6.f;

	static constexpr float MarkerLabelFontSize = 11.f;

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

	// +Y world grows UP: Slate's local Y grows DOWNWARD, so the axis is inverted here and
	// NOWHERE ELSE. Doing it at the projection means the ally dots, the enemy dots and the
	// markers cannot disagree about which way the field runs.
	MapUV.Y = FMath::Clamp((HalfY - WorldXY.Y) / (2.0 * HalfY), 0.0, 1.0);

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

	// Seed immediately so the map is never blank for up to one refresh period, THEN start
	// the interval.
	RefreshAllyDots();
	SetAllyRefreshTimerEnabled(true);

	// ⭐ TASK-579 - THE FIRST-OPEN STATUS LINE (WR-§9 row 12).
	//
	// ⛔ THE DISCRIMINATOR IS THE SNAPSHOT ITSELF, ⛔ NOT "did BuildMarkerRects return
	// anything", and the difference is the point rather than a shortcut. The snapshot is the
	// CAUSE the line is about; an empty marker array has three possible causes (no snapshot,
	// nothing resolved, a degenerate panel) and only ONE of them is fixed by sending an order.
	// A status line that names the wrong remedy is worse than no status line: the player does
	// the thing it says, nothing changes, and he stops believing the map.
	//
	// ⛔ GetReadOnlySnapshot() READS. It does not Capture(), does not EnsureSnapshot() and does
	// not reach anything that does - WR-§6's snapshot-hazard row, and the property TASK-565
	// re-greps. Opening this panel still surveys NOTHING.
	if (GetReadOnlySnapshot() == nullptr)
	{
		ShowNoSnapshotHint();
	}
	else
	{
		SetStatusLine(SiegeWarMap::OpenStatusText);
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

		// ⚠️ Log, not Warning: this is a DESIGNED degradation, not a fault. It is the named
		// finding of TASK-560 (WarMapWidget.h §3) and it is escalated in the handoff rather
		// than worked around here - every available workaround crosses a boundary this task
		// is forbidden to cross.
		UE_LOG(LogSiegeWarMap, Log,
			TEXT("[WarMap] No assistant snapshot yet - the map draws dots but NO place markers, so there is nothing to click. ")
			TEXT("Expected before the player's FIRST console sentence (USiegeAssistantComponent allocates its snapshot on the turn path). ")
			TEXT("Deliberately NOT fixed here: forcing a Capture() is the exact side effect the WAR-ROOM law forbids. See handoffs/TASK-560-programmer.md."));
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
// Ally dots (WR-§6) - the ONLY actor iteration in this file
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

	// ⛔ AFTER the sweep, never before: the sweep is the thing the interval exists for, and a
	// hint check that threw would otherwise take the dots down with it. Order is cheap
	// insurance here.
	UpdateNoSnapshotHint();
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

	const int32 AllyLayer = MaxLayer + 1;
	const int32 EnemyLayer = MaxLayer + 2;
	const int32 MarkerLayer = MaxLayer + 3;
	const int32 LabelLayer = MaxLayer + 4;

	for (const FVector2D& AllyWorldXY : AllyDotsWorldXY)
	{
		SiegeWarMap::PaintQuad(
			OutDrawElements, AllyLayer, AllottedGeometry,
			FSiegeWarMapProjection::MapUVToLocal(
				FSiegeWarMapProjection::WorldToMapUV(AllyWorldXY, ArenaHalfExtent), RectOrigin, RectSize),
			SiegeWarMap::DotDrawHalfSizePx, AllyColor);
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

	// ⚠️ EVERY BUTTON IS ABSORBED WHILE THE MAP IS OPEN. The map fills the screen, so a
	// fall-through click would land on the WORLD behind it and issue a real order at a real
	// position - which the player would read as "the map made my army walk somewhere".
	// Absorbing is the fail-safe direction; only the LEFT button picks a symbol.
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	const FVector2f LocalPointF = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D LocalPoint(LocalPointF.X, LocalPointF.Y);

	// ⭐ ONE SOURCE OF TRUTH: the same builder, against the geometry Slate just handed this
	// event. What the player can SEE and what the player can CLICK cannot disagree.
	TArray<FSiegeWarMapMarker> Markers;
	BuildMarkerRects(InGeometry, Markers);

	const int32 HitIndex = FSiegeWarMapProjection::FindMarkerIndexAtLocal(Markers, LocalPoint);

	if (HitIndex == INDEX_NONE)
	{
		// ⛔ A CLICK ON EMPTY MAP DOES NOTHING beyond one line of static chrome naming what IS
		// clickable (WR-§9 outcome 1 - a DESIGNED outcome, on Jonathan's playtest sheet, so a
		// report of it is read correctly). ⛔ It does NOT invent a coordinate, a grid cell, a
		// snap radius or a nearest-marker guess: the vocabulary has no primitive for an
		// arbitrary point, and manufacturing one is the exact thing AS-§21.4 reserves for
		// Jonathan.
		//
		// ⚠️ TASK-579 MOVED THIS DISCRIMINATOR FROM Markers.Num() TO THE SNAPSHOT, AND IT IS A
		// CORRECTNESS CHANGE RATHER THAN A TIDY-UP. Markers can be empty WITH a snapshot present
		// (no place resolved this match, a degenerate panel) - and telling THAT player to send
		// an order is a remedy that will not work, i.e. the status line lying to him. The
		// snapshot is the only condition the explanation is actually about.
		if (GetReadOnlySnapshot() == nullptr)
		{
			ShowNoSnapshotHint();
		}
		else
		{
			SetStatusLine(SiegeWarMap::EmptyClickHintText);
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
	// retired - and once a snapshot exists it never comes back this open - this function costs
	// one branch per timer tick and never touches the controller, the component or the
	// snapshot again.
	if (!bShowingNoSnapshotHint)
	{
		return;
	}

	// ⛔ STILL A READ, STILL NOT A SURVEY. Same GetTurnSnapshot() route as everywhere else in
	// this file: no Capture(), no EnsureSnapshot(), nothing that reaches either. This function
	// WAITS for the snapshot the player's own sentence creates; it does not create one.
	if (GetReadOnlySnapshot() == nullptr)
	{
		return;
	}

	// ⛔ THE HINT MUST DISAPPEAR: it says "send an order and the markers appear", the markers
	// have now appeared, and a hint that outlives its own remedy tells the player the fix did
	// not work. ⚠️ ONE-WAY: this never re-shows the hint mid-open, so it can never overwrite a
	// symbol the player clicked one tick earlier.
	//
	// ⭐ AND ONE-WAY IS PROVABLY SUFFICIENT RATHER THAN OPTIMISTIC - VERIFIED AT THE OWNING
	// FILE, NOT ASSUMED. USiegeAssistantComponent writes its Snapshot member in exactly ONE
	// place, EnsureSnapshot()'s `if (!Snapshot) Snapshot = NewObject<...>(this)`, and ⛔ never
	// assigns it null anywhere; its own comment calls it "ONE object for the life of the
	// component" (Capture() resets the OBJECT's contents, never the pointer). ⇒ The transition
	// this function watches for is MONOTONIC - null to non-null, once, permanently - so there
	// is no later state for a one-way latch to miss and no case where the hint should re-arm.
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
