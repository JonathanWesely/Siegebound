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
};
