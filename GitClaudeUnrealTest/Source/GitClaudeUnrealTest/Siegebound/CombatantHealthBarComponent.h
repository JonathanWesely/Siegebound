// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "Engine/EngineTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "CombatantHealthBarComponent.generated.h"

class UUserWidget;
class UCombatantHealthBarWidget;

/**
 *  Push-model overhead health bar (TASK-130, castle-parity REBUILD of the retired
 *  poll system). A screen-space UWidgetComponent subclass, added ONCE in-constructor
 *  to ASummonedUnit, ABuilding, and AHeroCharacter (each names its instance
 *  HPBarWidget; subclasses — AMinerUnit, ATower, ABarracks, ADeepMine — inherit it).
 *
 *  MIRRORS ACastle::HPBarWidget: at BeginPlay it soft-resolves the widget class,
 *  reads the owner as IHealthBarProvider + ITeamAgent, and calls
 *  UCombatantHealthBarWidget::InitForCombatant (SEED-THEN-BIND) + SetTeamColor once.
 *  There is NO poll timer (the failed poll system is retired); the bar's VALUE updates
 *  only when the owner BROADCASTS its OnHPChanged delegate. (TASK-791 added a periodic
 *  occlusion poll for VISIBILITY — still no FTimerHandle: it rides the tick the
 *  component was already required to run, and it never touches the HP path.)
 *
 *  Visibility (reversed hide-at-full law): the bar is ALWAYS VISIBLE while the owner
 *  is alive and opted-in (bShowHealthBar). Hide/show on death/respawn is the OWNER's
 *  job (HideBar / ShowBarIfEnabled) — the ACastle::HandleDestroyed / ResetCastle
 *  parity, since screen-space widget components do not follow actor hidden-in-game
 *  state and there is no poll to observe death here.
 *
 *  Boost row (TASK-362, ancient grounds): the SAME widget grew a second bar above the
 *  health bar showing the owner's permanent damage boost. This component owns the whole
 *  boost path — it SEEDS UNCONDITIONALLY at BeginPlay (so a non-boostable owner's row is
 *  driven to opacity 0 instead of sitting at its design-time state — the qa/TASK-005
 *  major-2 seed-then-bind law), THEN binds FOnCombatantDamageBoostChanged only if the
 *  owner provides one, and it does ALL the banding math before pushing the single atomic
 *  SetDamageBoost BIE. DrawSize is (90, 22): ~8 px boost row + the original ~12 px health
 *  bar. Still NO poll, still no gameplay tick.
 *  ⚠️ THAT SPLIT SENTENCE IS STALE AND IS LEFT STANDING ONLY BECAUSE IT IS SHIPPED PROSE
 *  (TASK-861 re-measured the live slots): 22 px less BoostOutline's 1 px bottom pad leaves 21 px
 *  split Fill 1:2, i.e. BoostOutline 7.00 px and Bar 14.00 px — the law's "~8 / ~12" predates
 *  TASK-368's Fill 1.0 → 2.0 fix and was never re-derived. Nothing is broken; anyone budgeting
 *  pixels from the old numbers is 2 px out. The cast row below budgets from 7.00 / 1 / 14.00.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────
 *  THE CAST ROW (TASK-860 — law WITCH-§9; the WITCH'S TWO-ENDED, INTERRUPTIBLE CHANNEL)
 *  ─────────────────────────────────────────────────────────────────────────────────
 *  SK_Witch does not exist, so the witch spawns STATIC, and on a 3-second INTERRUPTIBLE cast
 *  "starting", "running" and "broken" are IDENTICAL PIXELS. Jonathan did not ask for a 3-second
 *  cast; he asked for one THAT CAN BE INTERRUPTED — and counterplay the player cannot perceive
 *  is not counterplay. WITCH-§9.2 rules the tell is a THIRD segment in THIS SAME widget, driven
 *  on BOTH the witch AND her target, because his sentence names both actors and the bar must
 *  therefore appear on EXACTLY the two units you can attack to break the cast.
 *
 *  ⛔ EXTENDED, NEVER DUPLICATED. A new WBP_WitchCastBar / a second UWidgetComponent on the
 *  unit is an automatic QA fail (WITCH-§9.3): two worldspace widgets on one actor fight for
 *  screen space, sort against each other, and double the per-unit widget cost on a 50,000-uu
 *  field. The boost row is the shipped precedent for exactly this shape, down to the DrawSize.
 *
 *  ⛔⛔ IT IS A POLL, AND THAT IS THE CONTRACT RATHER THAN A SHORTCUT. IHealthBarProvider's cast
 *  surface is two DEFAULTED getters and carries NO delegate by design (TASK-830): a cast
 *  progresses CONTINUOUSLY, so a push model would need a per-frame broadcast from the unit —
 *  strictly worse than one read on the bar's own update. ⭐ Both getters are read TOGETHER on
 *  one poll because they share ONE resolver on the owner and therefore cannot disagree; reading
 *  the gate this poll and the fill the next would reintroduce that disagreement by hand.
 *
 *  ⛔⛔ AND THE OWNER ANSWERS ABOUT ITSELF, ALWAYS. The witch NEVER pushes a percent onto her
 *  target's widget (WITCH-§9.3 forbids it by name) — the subject PULLS the same live timer
 *  through its own weak back-pointer. That is what makes the target's bar vanish on the SAME
 *  FRAME the witch dies, which the interrupt rule makes the COMMON case rather than an edge one.
 *
 *  ⛔ THE PIXEL-IDENTICAL GUARANTEE IS A REQUIREMENT (WITCH-§9.6), NOT AN EXPECTATION, AND IT IS
 *  WHY DrawSize GROWS AT CAST TIME RATHER THAN IN THE CONSTRUCTOR: BoostOutline and Bar are Fill
 *  slots and absorb every spare pixel, so a constructor bump to (90, 30) would render Bar at
 *  19.33 px instead of 14.00 on EVERY building, the hero and all 20+ units, casting or not — a
 *  permanent, silent, game-wide health-bar resize shipped by a witch task. See ApplyCastRowGeometry.
 *
 *  ─────────────────────────────────────────────────────────────────────────────────
 *  OCCLUSION CULL (TASK-791, VIS-§2 / ruling VIS-R1) — WHY THE BAR CAN NOW HIDE ITSELF
 *  ─────────────────────────────────────────────────────────────────────────────────
 *  VID-004 showed bars drawn straight through the watchtower's stonework, and four bars
 *  floating over an EMPTY deck belonging to units standing a tower-height below it.
 *  ⛔ THAT IS NOT A BUG AND THERE IS NO DEPTH SETTING TO REPAIR: SetWidgetSpace(Screen)
 *  in the constructor composites this widget as a SLATE OVERLAY, and a Slate overlay
 *  NEVER participates in the depth buffer — by engine design. Drawing over solid world
 *  is the DEFINED behaviour of the widget space this component chose.
 *
 *  ⇒ the fix is a VISIBILITY CULL, not a rendering change: poll a single line trace from
 *  the owning player's camera to the bar's world anchor, and hide the bar when the world
 *  is in the way. ⛔ EWidgetSpace::World is REFUSED for this (VIS-R1): it costs a RENDER
 *  TARGET PER BAR across a 20+ pawn roster and would be owed at three call sites.
 *
 *  ⛔ AND IT IS A POLL, NOT A TICK-TRACE, WHICH IS A SPEC TERM RATHER THAN AN
 *  OPTIMISATION: this component was built with NO poll timer on purpose (updates arrive
 *  by delegate), so a per-frame trace per pawn would be a roster-wide cost it was
 *  deliberately built without. Per frame per unit the cull costs ONE float add and ONE
 *  compare; the trace itself runs at 1/HealthBarOcclusionIntervalSeconds (≈6.7 Hz).
 *
 *  ⛔⛔ THE LAW TASK-791 PUT ON EVERY OWNER OF THIS COMPONENT — READ IT BEFORE ADDING A NEW
 *  ONE (qa/TASK-801 `B-1`): an owner hides or shows this bar ONLY through HideBar() /
 *  ShowBarIfEnabled(). ⛔ NEVER SetVisibility() DIRECTLY. Before TASK-791 the two were
 *  equivalent and SetVisibility(false) was TERMINAL; the cull is now a SECOND writer running
 *  on the tick, so a raw write leaves the owner-intent latch reading "shown" and the next poll
 *  (≈150 ms) puts the bar straight back — a 0-HP bar over a corpse for the whole death-anim
 *  hold, on every rigged unit on the field. ⭐ ASummonedUnit shipped exactly that for one
 *  review cycle. It is asserted now, in SiegeHealthBarOcclusionTest.
 *
 *  ⛔ AND THE CULL FAILS OPEN ON A BURIED CAMERA (`B-2`): TASK-790 deliberately parks the
 *  follow camera up to 150 uu INSIDE the watchtower for ≈2 s at every ladder approach, which
 *  makes EVERY unit's ray start inside one blocking hull at once. A zero-distance hit is a
 *  NON-ANSWER and resolves to VISIBLE — see ComputeOcclusionFromTraceResult(). Without that
 *  rule the whole roster's bars blink off together at the exact moment VID-004 filmed.
 *
 *  Everything null-safe: a missing widget class is a silent no-bar (logged once),
 *  never a crash. Zero combat/stat behavior change — it only reads HP getters + binds.
 */
UCLASS(ClassGroup = (Siegebound), meta = (BlueprintSpawnableComponent))
class GITCLAUDEUNREALTEST_API UCombatantHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UCombatantHealthBarComponent();

	/** Shows the bar iff opted-in (bShowHealthBar). Called by the owner on respawn/reset (ACastle::ResetCastle parity). */
	void ShowBarIfEnabled();

	/** Hides the bar. Called by the owner on death/destruction (ACastle::HandleDestroyed parity) — screen-space widgets do not follow SetActorHiddenInGame. */
	void HideBar();

	/**
	 *  ⭐ THE TEAM PALETTE'S PUBLIC READ SEAM — WR-§6, manager ruling W4-R5 (added at TASK-560's
	 *  C2248 repair). These return this component's CLASS DEFAULTS (the CDO's BlueBarColor /
	 *  RedBarColor), so any display that must match a health bar's team tint — UWarMapWidget's
	 *  ally and enemy dots are the first caller — reads the ONE shipped owner instead of
	 *  re-typing the literals and drifting the day the palette moves.
	 *
	 *  ⛔ STATIC, AND THAT IS THE ENTIRE MECHANISM: a static member may read its own class's
	 *  protected members, so BlueBarColor/RedBarColor STAY protected below and gain NO writable
	 *  surface. ⛔ Plain C++ statics, NOT UFUNCTIONs — a palette read is not a Blueprint API and
	 *  reflecting it would invite a second caller. ⛔ ZERO parameters, deliberately: a
	 *  team-parameterised accessor would force Siegebound/TeamId.h into this header, which today
	 *  only the .cpp includes. (Zero parameters also means SC-§33 cannot fire structurally.)
	 *
	 *  ⚠️ CLASS DEFAULTS, ⛔ NOT AN INSTANCE READ, AND THE DIFFERENCE IS DELIBERATE: the per-bar
	 *  team tint applied in BeginPlay reads THIS INSTANCE's fields, which a BP subclass may
	 *  legitimately override. That read is a different question and is left exactly as it is —
	 *  routing it through these accessors would silently delete per-BP tint overrides.
	 */
	static FLinearColor GetDefaultBlueBarColor();
	static FLinearColor GetDefaultRedBarColor();

	/**
	 *  ⭐ THE QA READBACK HOOK (VIS-§2 pinned name; the ASummonedUnit::GetPermanentDamageMultiplier
	 *  precedent — a rule nobody can see from outside is a rule nobody can review from outside).
	 *
	 *  True iff the LAST occlusion poll found world geometry between the owning player's camera and
	 *  this bar's world anchor. ⛔ A CACHED READ, NEVER A LIVE TRACE: calling it a thousand times in
	 *  a frame costs a thousand bool loads and ZERO queries, which is the only shape in which a
	 *  BlueprintPure may be handed to designers.
	 *
	 *  ⚠️ IT REPORTS THE TRACE RESULT, ⛔ NOT THE BAR'S VISIBILITY, and the difference is deliberate:
	 *  a dead unit's bar is hidden while this reads false, and with the cull switched off the poll
	 *  never runs so this stays false forever. Visibility is ComputeDesiredBarVisibility(), which is
	 *  the function that composes the two — assert against THAT, not against this.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|HealthBar")
	bool IsHealthBarOccluded() const { return bHealthBarOccluded; }

	/**
	 *  ⭐ THE PURE DECISION SEAM — the ASummonedUnit::HeightAdvantageMultiplier precedent (HIGH-§3):
	 *  a testability obligation gets a testability seam. THREE BOOLS IN, ONE BOOL OUT — no UWorld,
	 *  no AActor, no UObject, no clock, no trace ⇒ the whole visibility rule runs headless.
	 *
	 *  ⛔ THIS IS NOT AN ACCESSOR ADDED SO A TEST COULD REACH SOMETHING: it is the shipping
	 *  implementation, and ApplyBarVisibility() is its only caller in game code. Factoring it out
	 *  adds no behaviour and removes no privacy — the fields it reasons about stay private.
	 *
	 *  ⭐ THE OWNER'S INTENT IS THE OUTER AND, AND THAT IS THE WHOLE CONTRACT: the cull may only ever
	 *  SUBTRACT visibility. There is deliberately NO path by which an unoccluded trace shows a bar
	 *  the owner hid — resurrecting a dead unit's health bar would be a worse defect than the one
	 *  this cull exists to fix.
	 */
	static bool ComputeDesiredBarVisibility(bool bOwnerWantsBarShown, bool bCullEnabled, bool bOccluded);

	/**
	 *  ⭐ THE POLL GATE, ALSO PURE (one float& and two floats in, one bool out — no world, no clock:
	 *  the caller supplies DeltaTime). Advances InOutAccumulatedSeconds and returns true EXACTLY on
	 *  the frames a trace is due; every other frame this is one add and one compare, which is the
	 *  entire per-frame cost of the cull. (⚠️ qa/TASK-801 N-2, for whoever re-measures: this is
	 *  defined OUT OF LINE in the .cpp, so the true per-frame figure is one CALL around that add and
	 *  compare — ~20 calls/frame across the roster. Kept out of line deliberately: the tests link
	 *  against it as a shipped seam, and inlining it into the header would buy nothing measurable.)
	 *
	 *  ⛔ A NON-POSITIVE ConfiguredIntervalSeconds IS FLOORED, NOT OBEYED. The interval is
	 *  EditDefaultsOnly, so a Blueprint can set it to 0 — and without the floor that would silently
	 *  degrade this into the one thing VIS-§2 forbids by name, a per-frame trace per pawn across the
	 *  roster. Whatever a BP asks for, this polls at most 30×/s.
	 *
	 *  ⛔ AND IT DOES NOT CATCH UP: on a fire the accumulator is RESET TO ZERO rather than having the
	 *  period subtracted. After a one-second hitch, subtracting would leave enough banked time to
	 *  fire on ~7 CONSECUTIVE frames — a trace burst, i.e. precisely the cost this design refuses,
	 *  arriving at the worst possible moment. Dropping the arrears is the correct trade for a cull.
	 */
	static bool ShouldPollOcclusion(float& InOutAccumulatedSeconds, float DeltaSeconds, float ConfiguredIntervalSeconds);

	/**
	 *  ⭐⭐ THE BURIED-CAMERA RULE, AND IT IS THE THIRD PURE SEAM (qa/TASK-801 `B-2`). Two bools and a
	 *  float in, one bool out — no UWorld, no trace, so the rule that protects the ENTIRE HUD runs
	 *  headless even though the query that feeds it cannot.
	 *
	 *  ⛔ A TRACE THAT STARTS INSIDE BLOCKING GEOMETRY IS ⛔ NOT AN OCCLUSION — IT IS A NON-ANSWER.
	 *  The follow camera is deliberately placed up to 150 uu INSIDE the watchtower at the ladder
	 *  approach (TASK-790's near-clip floor), for ≈2 s, every climb. In that window EVERY unit's
	 *  camera→bar ray starts inside the tower hull, and Chaos returns a blocking hit at zero distance
	 *  for all of them (`Chaos::FConvex::RaycastFast`: start inside ⇒ every plane distance negative ⇒
	 *  `EntryTime` never advances past its initial 0 ⇒ `OutTime = 0`, `return true`). Without this rule
	 *  the WHOLE ROSTER'S BARS BLINK OFF TOGETHER at exactly the moment and place VID-004 filmed —
	 *  louder than the defect the cull exists to fix.
	 *
	 *  ⇒ a zero-distance / start-penetrating hit resolves to ⭐ NOT OCCLUDED. That is the same
	 *  fail-open direction as every early-out in UpdateHealthBarOcclusion(): "we could not tell" must
	 *  never blank the HUD, least of all for a player who already cannot see the world.
	 *
	 *  ⭐ THE FIRST TWO CLAUSES ARE THE ENGINE'S OWN RULE, NOT AN INVENTION: `FHitResult::
	 *  IsValidBlockingHit()` (UE 5.8 `HitResult.h:236-239`) is character-for-character
	 *  `bBlockingHit && !bStartPenetrating`. The distance clause is a deliberate SECOND WITNESS to the
	 *  same fact from the raw hit rather than from the conversion layer — on today's engine path the
	 *  two agree by construction (`ChaosInterfaceWrapperCore.h:117` defines the initial-overlap flag as
	 *  `Distance <= 0.f`, and `CollisionConversions.cpp:366` copies it straight into
	 *  `bStartPenetrating`), and a fail-open that guards the whole roster's HUD should not rest on a
	 *  single engine-internal bit.
	 *
	 *  ⛔ AND ⛔ NOT `FCollisionQueryParams::bFindInitialOverlaps` — that flag CANNOT buy this on a line
	 *  trace, which is what made this a shipped defect rather than a design choice. See
	 *  UpdateHealthBarOcclusion() in the .cpp for the three engine lines that prove it inert here.
	 */
	static bool ComputeOcclusionFromTraceResult(bool bTraceBlocked, bool bTraceStartedInsideGeometry, float HitDistanceUU);

	//~ ── Begin THE CAST ROW'S FOUR PURE SEAMS (TASK-860; law WITCH-§9) ────────────────────────────
	//  The ComputeDesiredBarVisibility / ShouldPollOcclusion / ComputeOcclusionFromTraceResult
	//  tradition, and for the same reason: bools and floats in, ONE value out — no UWorld, no AActor,
	//  no widget, no Slate, no clock ⇒ the WHOLE cast-row rule runs headless.
	//
	//  ⭐⭐ AND HERE THAT IS NOT MERELY TIDY, IT IS THE ONLY WAY THE TASK CAN BE GATED AT ALL. The
	//  question SHIP-§9 makes this feature answer is "can the suite tell a BROKEN cast from a
	//  COMPLETED one?" — a question about a SEQUENCE OF EVENTS OVER TIME, not about one value. It can
	//  only be asked of a test that can replay a whole cast, and a test can only replay a whole cast
	//  if the decisions are reachable without a world. ⚖️ This feature exists because "completed" and
	//  "broken" were indistinguishable ON SCREEN; a suite that cannot tell them apart IN CODE has
	//  reproduced the defect one layer down.
	//  ⛔ These are the SHIPPING implementations, not accessors added so a test could reach something:
	//  UpdateCastProgress / PushCastProgress / ApplyCastRowGeometry are their only game-code callers.

	/**
	 *  Fires on exactly the frames a cast poll is due — the cast row's own accumulator and its own
	 *  interval, so it never inherits the occlusion cull's 0.15 s period (see the interval's comment
	 *  for why that period would be visibly wrong here).
	 *
	 *  ⛔ IT FORWARDS TO ShouldPollOcclusion RATHER THAN RESTATING IT, DELIBERATELY. That function is
	 *  a GENERIC fixed-period gate whose name records only its first caller: accumulator, delta and
	 *  interval in, "is a tick due?" out — nothing in it knows what a trace is. Duplicating its six
	 *  lines would create a SECOND implementation of one shipped guarantee (the ≤30 polls/second
	 *  floor a Blueprint cannot lower), and two copies of a guarantee drift.
	 *  ⚠️ Renaming the shared gate to match both callers would be the tidier fix and is deliberately
	 *  NOT done here: the name is bound by SiegeHealthBarOcclusionTest, which is not this task's file.
	 *  Flagged for QA rather than done quietly.
	 */
	static bool ShouldPollCastProgress(float& InOutAccumulatedSeconds, float DeltaSeconds, float ConfiguredIntervalSeconds);

	/**
	 *  Does this poll have anything to say to the widget? ⭐ THE WHOLE RULE, AND IT IS THE REASON THIS
	 *  FEATURE IS FREE FOR THE FLEET: every building, the hero and every non-witch unit answers
	 *  (false, false) for the entire match and therefore makes ZERO Blueprint calls, ever.
	 *
	 *  ⛔⛔ THE FALLING EDGE IS THE HIGHEST-CONSEQUENCE HALF AND IT IS THE ONE AN "if (bCasting)" WOULD
	 *  DROP: when a cast ends — completed OR interrupted — bCasting is false while the row is still
	 *  driven open, and that poll MUST push. Without it the last frame of every cast is the last thing
	 *  the widget is ever told: a cast bar frozen mid-flight forever, on a unit doing nothing, with
	 *  DrawSize left 8 px tall for the rest of the match. ⭐ It is also the ONLY event that reports an
	 *  INTERRUPT, which is the counterplay this entire feature exists to make visible.
	 */
	static bool ShouldPushCastRow(bool bCasting, bool bRowAlreadyDriven);

	/**
	 *  Turns the provider's raw answer into the number the widget is allowed to see.
	 *
	 *  ⛔ NOT CASTING ⇒ EXACTLY 0.f, whatever the provider said. The gate and the fill are pushed as
	 *  ONE atomic event, so a collapsed row carrying a stale 87% is a state this makes unreachable
	 *  rather than merely unlikely.
	 *  ⛔ CASTING ⇒ CLAMPED TO 0..100 (⛔ never 0..1 — the shipped BoostPercent convention, WITCH-§9.3).
	 *  The clamp is not defensive padding: it is the guarantee the surface CANNOT LIE in the one
	 *  direction that matters, because "the fill reached the ends" must be producible ONLY by a cast
	 *  that actually ran its window (WITCH-§9.1 row 4 — completed vs BROKEN, the perception with no
	 *  tell at all in the shipped game).
	 *  ⚠️ A NON-FINITE INPUT RESOLVES TO 0, AND FMath::Clamp CANNOT DO THAT JOB: Clamp is a pair of
	 *  `<` / `>` comparisons and every comparison against NaN is false, so a NaN would pass straight
	 *  through the clamp and into Slate's SetPercent.
	 */
	static float SanitizeCastPercent(bool bCasting, float ProviderCastPercent);

	/**
	 *  The DrawSize height (in screen pixels) the bar grows to while a cast is running.
	 *
	 *  ⛔ ROUNDED TO A WHOLE PIXEL HERE rather than left to the engine, and that matters because the
	 *  pivot below is computed FROM this number: UWidgetComponent stores DrawSize as an FIntPoint and
	 *  SetDrawSize TRUNCATES (`FIntPoint((int32)Size.X, (int32)Size.Y)`, UE 5.8 WidgetComponent.cpp).
	 *  An un-rounded 30.5 would be laid out as 30 while the pivot compensated for 30.5 — a half-pixel
	 *  disagreement between the two halves of ONE geometry change, i.e. a bar that creeps.
	 */
	static float ComputeCastBarHeightPixels(float BaseBarHeightPixels, float CastRowHeightPixels);

	/**
	 *  ⭐⭐ THE PIXEL-IDENTICAL GUARANTEE, EXPRESSED AS ARITHMETIC — the Pivot the grown bar must use so
	 *  that the HP row does NOT MOVE when a cast starts.
	 *
	 *  ⛔ THE PROBLEM THIS SOLVES IS INVISIBLE FROM THE CALL SITE. This component never set Pivot, so
	 *  it is the engine default (0.5, 0.5) — the widget is CENTRED on its projected anchor. Growing
	 *  DrawSize by 8 px about a centred pivot therefore grows 4 px UP and 4 px DOWN, dropping the
	 *  health bar 4 px into the unit's head for the whole cast. ⛔ And the obvious fix — setting
	 *  Pivot=(0.5,1.0) in the constructor — is WORSE: it would move every bar in the game up by half
	 *  its height, permanently, which is the fleet-wide regression WITCH-§9.6 forbids.
	 *
	 *  ⇒ the pivot is recomputed WITH the size, at cast time, to hold the BOTTOM EDGE exactly where
	 *  the default state puts it. In the screen-space path the engine feeds Pivot to the canvas slot
	 *  as its ALIGNMENT (SWorldWidgetScreenLayer.cpp — `CanvasSlot->SetAlignment(ComponentPivot)`, read
	 *  fresh every frame alongside GetDrawSize), and a canvas alignment A offsets a box of height H by
	 *  -A*H ⇒ the bottom edge sits (1 - A) * H BELOW the anchor. Holding that product constant is the
	 *  whole trick, and it makes the +8 px grow ENTIRELY UPWARD — into the empty sky above the unit,
	 *  which is the only direction with room.
	 *
	 *  ⚠️ Domain: GrownBarHeightPixels >= (1 - BasePivotY) * BaseBarHeightPixels, which the shipped
	 *  path guarantees (the row height is floored at 0, so grown >= base). A non-positive grown height
	 *  returns the base pivot unchanged rather than dividing by zero.
	 */
	static float ComputeCastPivotY(float BaseBarHeightPixels, float BasePivotY, float GrownBarHeightPixels);

	//~ ── End the cast row's pure seams ────────────────────────────────────────────────────────────

protected:

	/**
	 *  Soft-resolves the widget class, seeds+binds via the owner's OnHPChanged delegate, pushes the
	 *  team tint once, then seeds the boost row UNCONDITIONALLY and binds
	 *  FOnCombatantDamageBoostChanged iff the owner provides one (TASK-362). All null-safe.
	 */
	virtual void BeginPlay() override;

	/**
	 *  Drives the occlusion poll (TASK-791). Kept alongside BeginPlay() in `protected` — this is
	 *  engine-invoked and has no caller in game code. ⛔ THE ONLY WORK THIS ADDS PER FRAME is one bool test
	 *  (is the cull on?), one bool test (does the owner even want the bar shown?) and — when both
	 *  pass — one float add and one compare inside ShouldPollOcclusion. ⛔ NO TRACE PER FRAME.
	 *
	 *  Super:: RUNS FIRST AND MUST: UWidgetComponent::TickComponent → UpdateWidget() →
	 *  UpdateWidgetOnScreen(), and UpdateWidgetOnScreen is the engine function that adds this bar to
	 *  FWorldWidgetScreenLayer when IsVisible() and REMOVES it when not (WidgetComponent.cpp:1317 in
	 *  UE 5.8 — `if (TargetPlayer && PlayerController && IsVisible() && !GetOwner()->IsHidden())`).
	 *  ⭐ THAT ENGINE LINE IS THE ENTIRE MECHANISM BY WHICH THIS CULL WORKS, and it is why the cull
	 *  is expressed as SetVisibility rather than as anything to do with depth. A visibility change
	 *  made here therefore lands on the NEXT frame's UpdateWidgetOnScreen — a ≤1-frame latency that
	 *  is immaterial next to the 150 ms poll period.
	 *
	 *  ⚠️ SetTickMode(ETickMode::Enabled) in the constructor is load-bearing for this: under
	 *  ETickMode::Disabled the engine switches the component tick OFF outright
	 *  (WidgetComponent.cpp:1275-1279), which would strand a culled bar hidden forever with no tick
	 *  left to un-hide it. ⛔ Do not "optimise" that tick mode. (⚠️ qa/TASK-801 W-4: the OTHER
	 *  auto-disable at :1264 keys on IsWidgetVisible(), which in Screen space reads the INNER widget
	 *  rather than this component — so Automatic alone would not trip it. The constructor comment
	 *  carries the full correction.)
	 */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 *  Runs ONE line trace — camera → this bar's world anchor — and caches the answer in
	 *  bHealthBarOccluded. Called only from the poll, never per frame.
	 *
	 *  ⛔ FAIL-OPEN BY CONSTRUCTION: every early-out CLEARS occlusion rather than setting it. A
	 *  missing world / local player / camera / widget is "we could not tell", and "we could not
	 *  tell" must never blank the health bar of every unit on the field.
	 *
	 *  ⛔ AND THE BURIED CAMERA IS ONE OF THOSE NON-ANSWERS (qa/TASK-801 `B-2`): the hit is fed
	 *  through ComputeOcclusionFromTraceResult(), which discards a zero-distance / start-penetrating
	 *  hit. That is why this is the SINGLE-hit query form and not the cheaper TEST form — the boolean
	 *  alone cannot tell a wall from a camera standing inside one.
	 */
	void UpdateHealthBarOcclusion();

	/**
	 *  ⛔ THE ONE WRITER of this component's visibility. ShowBarIfEnabled(), HideBar() and the
	 *  occlusion poll ALL route through here, which is exactly what stops the cull from ever
	 *  overwriting the owner's hide-on-death decision (and vice versa).
	 */
	void ApplyBarVisibility();

	/**
	 *  Component-side delegate handler (TASK-130 render-fix). Bound to the owner's OnHPChanged in
	 *  BeginPlay, ADDITIVE to the widget's own seed-then-bind. On each HP change it drives the
	 *  CURRENT on-screen `GetWidget()` directly (identity-proof: if the widget's own binding is
	 *  updating a stale/offscreen instance from an earlier frame, this always targets the live one)
	 *  and calls `RequestRedraw()` (World-space stale-render safety; a no-op for Screen-space live Slate).
	 */
	UFUNCTION()
	void HandleOwnerHPChanged(float CurrentHP, float MaxHP);

	/**
	 *  FOnCombatantDamageBoostChanged handler (TASK-362). Bound in BeginPlay ONLY when the owner
	 *  actually provides a delegate (a building/hero returns nullptr). Forwards straight to
	 *  PushDamageBoost — every broadcast re-bands and re-pushes, there is no cached boost state.
	 */
	UFUNCTION()
	void HandleOwnerDamageBoostChanged(float BoostPercent);

	/**
	 *  Bands BoostPercent (0 = none, 100 = +100%, 400 = the cap) and pushes the whole boost row
	 *  through the widget's single atomic SetDamageBoost BIE. ALL banding math lives here — the
	 *  widget has ZERO conditionals (CONVENTIONS §5). Drives the CURRENT on-screen GetWidget()
	 *  (identity-proof, the HandleOwnerHPChanged precedent) and RequestRedraw()s after.
	 *  BoostPercent <= 0 ⇒ RowOpacity 0, which is how a non-boostable actor's row is hidden.
	 */
	void PushDamageBoost(float BoostPercent);

	/** Band index (1-4) → its EditDefaultsOnly tint. Out-of-range clamps to band 4 (the strongest). */
	FLinearColor GetBoostBandColor(int32 Band) const;

	/**
	 *  ONE cast poll (TASK-860). Reads the OWNER'S OWN IHealthBarProvider — both getters together,
	 *  because they share one resolver on the owner and so cannot disagree — and pushes only when
	 *  ShouldPushCastRow says there is something to say. ⛔ A non-provider owner is not a special case:
	 *  it reads as "not casting" and takes the same free path every building already takes.
	 */
	void UpdateCastProgress();

	/**
	 *  Latches what the row was last driven to, applies the geometry, and pushes the single atomic
	 *  OnCastProgressChanged BIE to the CURRENT on-screen GetWidget() (the HandleOwnerHPChanged
	 *  identity-proof), then RequestRedraw()s. The seed at BeginPlay and every poll both come here, so
	 *  there is exactly ONE place that decides what the widget is told.
	 */
	void PushCastProgress(bool bCasting, float RawCastPercent);

	/**
	 *  Grows the bar to fit the cast row while a cast runs and restores the shipped size when it ends —
	 *  DrawSize AND Pivot together, because applying either alone moves the health bar (see
	 *  ComputeCastPivotY). ⛔ Idempotent and self-gating: it early-outs when the size is already right,
	 *  so the pair is written on the cast's two EDGES and never on a poll in between.
	 */
	void ApplyCastRowGeometry(bool bCasting);

	/**
	 *  Widget class shown by this bar (default /Game/UI/WBP_CombatantHealthBar, built
	 *  in TASK-131). Soft — resolved null-safe at BeginPlay; a missing asset means no
	 *  bar (logged once), never a crash. A BP may retarget or clear it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	TSoftClassPtr<UUserWidget> HealthBarWidgetClass;

	/** Per-actor/per-BP opt-out (default true). When false the bar stays hidden — a zero-code way to suppress clutter (e.g. miners). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	bool bShowHealthBar = true;

	/** Relative Z (units) of the bar above the actor root. Default 120; buildings/hero BPs may raise it. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	float BarHeightZ = 120.f;

	//~ Begin occlusion cull (TASK-791, VIS-§2 / ruling VIS-R1). ⛔ ALL THREE NAMES ARE PINNED BY
	//  VIS-§2 and may NOT be renamed. See the class comment for why this is a cull and not a
	//  widget-space change.

	/**
	 *  Master switch for the cull (default true). ⭐ FALSE RESTORES THE PRE-TASK-791 BEHAVIOUR
	 *  EXACTLY — not approximately: with this off, ComputeDesiredBarVisibility() collapses to the
	 *  owner's intent alone, which is bit-for-bit what ShowBarIfEnabled()/HideBar() did before, and
	 *  TickComponent early-outs before it accumulates, traces or writes visibility. That equivalence
	 *  is the regression guard, and it is asserted in SiegeHealthBarOcclusionTest.
	 *
	 *  🧑 FLAGGED FOR JONATHAN'S FEEL PASS: whether a bar behind a wall SHOULD vanish is a design
	 *  call (some games keep enemy bars visible through cover as a readability aid). This is the
	 *  one-word off switch if he wants the old look back.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	bool bOccludeHealthBarWhenBlocked = true;

	/**
	 *  Trace channel for the cull (default ECC_Visibility), and the default is load-bearing rather
	 *  than arbitrary: the engine's stock `Pawn` and `CharacterMesh` collision profiles both set
	 *  Visibility to ECR_Ignore (BaseEngine.ini), so a Visibility trace passes THROUGH other units
	 *  and the hero and stops only on world geometry.
	 *  ⇒ ⭐ EXACTLY THE QUESTION WE WANT ASKED — "is the WORLD in the way?" — and NOT "is a friendly
	 *  standing in front of him?", which would make bars flicker every time a squad crossed the
	 *  camera. ⛔ Retarget this only with that property in mind.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	TEnumAsByte<ECollisionChannel> HealthBarOcclusionChannel = ECC_Visibility;

	/**
	 *  Seconds between occlusion traces (default 0.15 ⇒ ≈6.7 traces/second/unit). ⛔ THE POLL IS A
	 *  SPEC TERM, NOT AN OPTIMISATION (VIS-§2): tracing per frame per pawn is a roster-wide cost
	 *  this component was deliberately built without — it has no poll timer at all and updates by
	 *  delegate. At 60 fps and 20 units this is ≈2.2 traces per frame across the WHOLE roster,
	 *  versus 20 for a per-frame design.
	 *
	 *  ⚠️ THE PRICE, STATED PLAINLY: up to one period of staleness. A bar can persist for ≤150 ms
	 *  after the world closes in front of it, and can stay hidden ≤150 ms after it clears. That is
	 *  the trade the poll buys and it is why the value is exposed.
	 *  ⛔ 0 does NOT mean "every frame" — ShouldPollOcclusion floors it. 🧑 Feel-pass tunable.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	float HealthBarOcclusionIntervalSeconds = 0.15f;

	//~ End occlusion cull

	/** Fill tint for a friendly (Blue) owner — §6 palette / MI_TeamColor linear values. Pushed to SetTeamColor once at init. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	FLinearColor BlueBarColor = FLinearColor(0.05f, 0.30f, 1.00f);

	/** Fill tint for an enemy (Red) owner — §6 palette / MI_TeamColor linear values. Pushed to SetTeamColor once at init. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar")
	FLinearColor RedBarColor = FLinearColor(1.00f, 0.10f, 0.05f);

	//~ Begin permanent damage boost row (TASK-362, ancient grounds). Tint is DATA — these four
	//  colors are pushed to the widget every update and are NEVER hardcoded in WBP_CombatantHealthBar.
	//  The ramp darkens monotonically (light → navy → purple → black) so "deeper = stronger" reads
	//  without a tooltip; band 3 is a DEEP purple on purpose (a bright violet computes to ~1.06:1
	//  against the boost track and vanishes). Contrast figures are vs the BoostBarTrackColor below.

	/** Band 1, 0-100% boost — light blue. ≈#C4E7FF, 2.99:1 on the boost track. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand1Color = FLinearColor(0.55f, 0.80f, 1.00f);

	/** Band 2, 100-200% boost — dark blue. ≈#1927A0, 2.96:1. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand2Color = FLinearColor(0.010f, 0.020f, 0.350f);

	/** Band 3, 200-300% boost — deep purple. ≈#7C19AD, 2.09:1. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand3Color = FLinearColor(0.200f, 0.010f, 0.420f);

	/** Band 4, 300-400% boost — near black. ≈#191920, 4.50:1 (the best on the bar). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBand4Color = FLinearColor(0.010f, 0.010f, 0.014f);

	/**
	 *  The BoostBar's own MEDIUM-GREY track. RECORDED HERE FOR THE RECORD ONLY — this component
	 *  never pushes it; the artist authors BoostBar's background brush tint from THIS number at
	 *  TASK-368. It exists as data so the value has exactly one home. Why BoostBar gets its own
	 *  track instead of reusing Bar's near-black (0.03,0.03,0.03)@0.7: band-4 black on near-black
	 *  is ~1.3:1, i.e. the STRONGEST unit would get the WORST indicator.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	FLinearColor BoostBarTrackColor = FLinearColor(0.22f, 0.22f, 0.24f, 0.85f);

	/**
	 *  Floor for the boost fill fraction so a just-crossed band (e.g. 100.1% ⇒ 0.001) still paints
	 *  a visible sliver of the NEW band color instead of an empty bar. Purely cosmetic: the band
	 *  outline, not the fill, is what disambiguates exactly-100% from just-past-100%.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Boost")
	float MinBoostFillFraction = 0.04f;

	//~ End permanent damage boost row

	//~ Begin cast row (TASK-860, the Witch's interruptible channel — law WITCH-§9).

	/**
	 *  Seconds between cast polls (default 0.05 ⇒ 20 reads/second/actor). ⛔ NOT A COSMETIC SMOOTHNESS
	 *  KNOB — IT IS LOAD-BEARING FOR WITCH-§9.1 REQUIREMENT 4, AND THE ARITHMETIC IS WHY IT IS NOT THE
	 *  OCCLUSION CULL'S 0.15 s:
	 *
	 *  The unit derives the percent from the LIVE timer, and the timer's callback CLEARS the handle —
	 *  so the last value the bar can ever observe on a COMPLETED cast is the one sampled one poll
	 *  before the end: 100 * (Duration - Interval) / Duration. At the shipped 3 s cast that is 98.3%
	 *  here (a 1.5 px shortfall on an 88 px bar — invisible), and 95.0% at a 0.15 s period (a 4.4 px
	 *  shortfall — visible, and directly confusable with an interrupt at 95%). ⇒ THE SLOWER PERIOD
	 *  WOULD MAKE "COMPLETED" AND "BROKEN-AT-THE-END" LOOK THE SAME, which is the precise defect this
	 *  whole feature exists to remove. Asserted in SiegeCastBarTest.
	 *
	 *  ⚠️ THE RESIDUAL, STATED RATHER THAN GLOSSED: an interrupt landing inside the FINAL poll window
	 *  (≤50 ms of a 3 s cast) still paints as "nearly full". That is 1.7% of the window and it is
	 *  disambiguated by the other half of the signal — a COMPLETED cast also turns the target
	 *  translucent (MI_Unit_Invisible), and a broken one changes nothing.
	 *  ⛔ 0 does NOT mean "every frame": this shares the occlusion gate's 30 Hz floor. 🧑 Feel tunable.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Cast")
	float CastProgressPollIntervalSeconds = 0.05f;

	/**
	 *  Screen pixels the bar GROWS BY while a cast runs (default 8 ⇒ DrawSize 22 → 30). ⛔ NOT the
	 *  height of the amber fill: it is one whole BoostOutline-shaped row — 7.00 px of row plus its
	 *  1 px bottom pad — so the arithmetic lands on whole pixels in both states.
	 *
	 *  ⭐ THE BUDGET, RE-DERIVED RATHER THAN QUOTED (TASK-861 measured the live slots; CastBarRoot's
	 *  slot MIRRORS BoostOutline's — Fill 1.0, pad-bottom 1, inserted at index 0):
	 *    · NOT casting: 22 px, CastBarRoot Collapsed ⇒ its slot is SKIPPED ⇒ (22 - 1) split Fill 1:2 ⇒
	 *      BoostOutline 7.00 · Bar 14.00 — byte-identical to today, on every actor in the game.
	 *    · CASTING:     30 px ⇒ (30 - 1 - 1) split Fill 1:1:2 ⇒ CastBarRoot 7.00 · BoostOutline 7.00 ·
	 *      Bar 14.00. ⭐ BoostOutline and Bar keep their EXACT current heights in BOTH states — the
	 *      health bar does not shrink to make room, which is stronger than the spec asked for.
	 *    · CastBarFill = 7.00 - (2 x 1.5 padding) = 4.00 px, identical to BoostBar's 4.00 px.
	 *
	 *  🧑 THIS IS THE ONE NUMBER TASK-861 SAID IT WAS LEAST CONFIDENT IN and it is exposed on purpose:
	 *  if 4 px of amber does not read at gameplay distance, the artist re-slots CastBarRoot to Fill 1.5
	 *  (≈9 px of row) and this becomes 10 — no code change, and the pivot arithmetic follows it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|HealthBar|Cast")
	float CastBarRowHeightPixels = 8.f;

	//~ End cast row

private:

	/** The widget instance cached from GetWidget() after SetWidgetClass; used for the seed-then-bind + team-tint setup in BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<UCombatantHealthBarWidget> BarWidget;

	//~ Begin occlusion cull runtime state (TASK-791). Plain members, deliberately NOT UPROPERTY:
	//  three PODs hold no references, so reflecting them would buy nothing but editor surface.

	/**
	 *  ⭐ THE OWNER'S INTENT, LATCHED — and the reason the cull cannot fight the death path.
	 *  Written ONLY by ShowBarIfEnabled() (to bShowHealthBar) and HideBar() (to false), i.e. by the
	 *  exact two functions that used to call SetVisibility directly. The cull then composes with it
	 *  instead of overwriting it, so a dead unit stays hidden however the trace lands.
	 *
	 *  ⚠️ DEFAULTS TRUE TO MATCH CONSTRUCTION, NOT AS AN OPTIMISM: the constructor deliberately does
	 *  NOT start hidden (castle parity — see its comment), and BeginPlay can return early on an
	 *  unresolved widget class WITHOUT reaching ShowBarIfEnabled(). False here would silently change
	 *  that path's behaviour.
	 */
	bool bBarShownByOwner = true;

	/** Result of the most recent occlusion poll. False until the first poll runs — the fail-open rest state. */
	bool bHealthBarOccluded = false;

	/** Seconds banked toward the next occlusion poll. Reset to zero on each fire (no catch-up — see ShouldPollOcclusion). */
	float OcclusionPollAccumulator = 0.f;

	//~ End occlusion cull runtime state

	//~ Begin cast row runtime state (TASK-860). Plain members, deliberately NOT UPROPERTY — three PODs
	//  holding no references, exactly like the occlusion trio above.

	/**
	 *  ⛔ WHAT THIS COMPONENT LAST TOLD THE WIDGET — ⛔ NOT whether a cast is running. The difference is
	 *  the whole reason this is not a second source of truth: the ANSWER always comes from the owner's
	 *  provider on the poll that asks; this only remembers whether the row is currently driven OPEN, so
	 *  the falling edge can be detected and pushed exactly once (see ShouldPushCastRow).
	 *
	 *  ⛔⛔ AND THERE IS DELIBERATELY NO CACHED PERCENT ANYWHERE IN THIS COMPONENT. WITCH-§4 requires an
	 *  interrupt to leave NO partial state, and the unit already honours that by DERIVING the percent
	 *  per call from the live timer; caching it here would reintroduce, one layer up, exactly the
	 *  remembered value that survives a cancel and freezes the bar mid-flight. Re-pushing an unchanged
	 *  percent costs one Blueprint call at 20 Hz on at most two actors in the world, and buys the
	 *  guarantee that nothing in this file can go stale.
	 */
	bool bCastRowDriven = false;

	/** Seconds banked toward the next cast poll. Its own accumulator — the cast row and the cull run on different periods. */
	float CastPollAccumulator = 0.f;

	/**
	 *  The bar's NOT-CASTING geometry, captured once at BeginPlay from the live component rather than
	 *  from the constructor's constant — so a Blueprint that legitimately retunes DrawSize or Pivot is
	 *  restored to ITS values when a cast ends, not to the C++ defaults. (The BarHeightZ read directly
	 *  above BeginPlay's capture exists for exactly the same reason.)
	 */
	FVector2D CastBarBaseDrawSize = FVector2D::ZeroVector;
	FVector2D CastBarBasePivot = FVector2D(0.5f, 0.5f);

	//~ End cast row runtime state
};
