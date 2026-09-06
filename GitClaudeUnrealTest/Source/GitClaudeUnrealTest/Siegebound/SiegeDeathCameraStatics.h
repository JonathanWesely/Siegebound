// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// FRotator and nothing else. No UWorld, no AActor, no AController, no UObject, no component,
// no clock — the FSiegeInvisibilityStatics / FSiegeLadderClimbStatics / FSiegeStuckStatics
// idiom (complete-type include law, TASK-110). That purity is not tidiness: it is the ONLY
// reason the two claims below can be asserted by a headless test that spawns nothing.
#include "CoreMinimal.h"

/**
 *  ═══ TASK-1102 (DEATH-CAM-ROLL): THE TWO PURE CAMERA-ROLL RULES ═══
 *
 *  ⭐ THE DEFECT THIS FILE EXISTS FOR, MEASURED, NOT REPORTED:
 *  `handoffs/TASK-1094-buildmaster.md` §5.6 + capture
 *  `playtest-evidence/2026-09-06/TASK-1094-E-OBSERVATION-death-camera-roll-90deg.png` — the host
 *  walked the hero into the RED army at 500 uu/s, was killed TWICE, and both times the player
 *  camera ended up **rolled ≈ 90°** (`control rotation roll 89.9`), the whole frame on its side,
 *  and it **stayed rolled through `AHeroCharacter::ResetHero()`**.
 *
 *  ── ⛔ THE WRITE SITE, NAMED (the row's clause 2 — diagnose in the CALL GRAPH) ────────────────
 *  ⚠️ EVERY LINE NUMBER IN THIS FILE IS ⛔ PRE-FIX (the tree TASK-1102 was handed), so the
 *  diagnosis can be checked against the evidence it was derived from. The fix adds comment lines,
 *  which shifts them: post-fix the two sites below are `:896` and `:867`.
 *
 *  `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:862` —
 *      `PC->SetControlRotation(GhostRotation);`
 *  fed one line-block above at `:833` by
 *      `const FRotator GhostRotation = DeadHero->GetActorRotation();`
 *  It is the ⛔ ONLY site in `Source/**` that can put a non-zero ROLL into the player's control
 *  rotation on the death path: a grep of the whole module finds `SetControlRotation` at exactly
 *  three sites (`SiegeGameMode.cpp:514` recall, `:862` death-ghost, `:1051` respawn), and the
 *  other two are fed by `GetHeroStartTransform`, which is provably yaw-only at every one of its
 *  three returns (`:1171`, `:1254`, `:1268`). ⛔ There is ⛔ no ragdoll anywhere in `Siegebound`
 *  (`SetSimulatePhysics` appears only in the read-only `Variant_*` template donors), so the
 *  leading hypothesis in the row — "the mesh went ragdoll and the camera copied it" — is
 *  ⛔ REFUTED. What actually happens is the OTHER half of that sentence: the death path copies
 *  the dead pawn's FULL actor rotation — roll included — into the control rotation.
 *
 *  ⚠️ AND `Possess()` COPIES IT TOO, WHICH IS WHY THE FIX CANNOT BE A DOWNSTREAM CLAMP:
 *  `AController::OnPossess` itself does `ClientSetRotation(GetPawn()->GetActorRotation())`, and
 *  `PC->Possess(Ghost)` runs at `:861`, i.e. BEFORE the explicit write at `:862`. The ghost was
 *  spawned at `GhostRotation` too (`:836`). ⇒ sanitising only the explicit `SetControlRotation`
 *  would leave two more copies of the same rolled value in flight. ⭐ Sanitising at the SOURCE
 *  (`:833`) closes all three with one expression — which is what "suppress it at the write site,
 *  ⛔ not by clamping downstream" means here.
 *
 *  ── ⛔ WHY A ROLLED CAPSULE IS NEVER SELF-CORRECTED (the persistence half) ────────────────────
 *  `AGitClaudeUnrealTestCharacter` ctor `:27` sets `RotationRate = FRotator(0.f, 500.f, 0.f)`.
 *  `UCharacterMovementComponent::PhysicsRotation()` turns a component toward its desired rotation
 *  ⛔ per-axis at that rate, so with Pitch-rate and Roll-rate BOTH zero it corrects ⛔ YAW ONLY —
 *  any roll that ever reaches the hero capsule is held forever. ⇒ the death path is handed a
 *  value nothing upstream will ever clean, and it must clean it itself.
 *
 *  ── ⛔ AND WHY IT SURVIVES THE RESET (the actual complaint) ───────────────────────────────────
 *  `AHeroCharacter::ResetHero()` (HeroCharacter.cpp:928) touches ⛔ ZERO rotation state — HP,
 *  bar, visibility, collision, movement mode, input, cooldowns, aura, upgrades, and ⛔ nothing
 *  about where the player is looking. The ONE place the roll is levelled today is
 *  `ASiegeGameMode::RestoreHeroAtStart` at `:1051`, which is a ⛔ DIFFERENT function ⛔ 180 s
 *  downstream of the death (`GHOST-§0`). ⇒ measured, `SC-§90`:
 *    • ⛔ LIVE: the camera is rolled for the WHOLE ghost period (up to 180 s of play). That is
 *      the player-visible defect and half (a) is what removes it.
 *    • ⚠️ LATENT: `ResetHero()` itself not levelling. Its only shipped caller is
 *      `RestoreHeroAtStart` (`:1040`), one line before `:1051` — so on the shipped path the roll
 *      IS levelled a moment later. The host's observation came from a FORCED, direct
 *      `ResetHero()` (their §4c), which is precisely the caller that gets no levelling. Half (b)
 *      moves the levelling INTO the reset primitive, where its name already promises it.
 *  ⛔ Both ship. The row is explicit that (b) ships even if (a) is deferred, ⛔ never the reverse.
 *
 *  ── ⛔ WHAT THIS FILE IS ⛔ NOT ──────────────────────────────────────────────────────────────
 *  ⛔ Not a death-cam effect, not a tilt, not a shake, not a clamp on player look input. It
 *  removes roll from ⛔ two specific rotations at ⛔ two specific moments and has no opinion about
 *  pitch or yaw anywhere else. A player who is looking down is still looking down after a reset.
 *
 *  📌 NOTE FOR QA — the pair is a header+cpp of ⛔ pure statics, ⛔ no UCLASS, ⛔ no
 *  `GITCLAUDEUNREALTEST_API` (every consumer is inside this module), matching
 *  `SiegeInvisibilityStatics.h:302-307`'s recorded shape. ⭐ It exists as statics rather than as
 *  two inline expressions ⛔ for one reason: `SC-§79`. An inline `FRotator(0.f, X.Yaw, 0.f)` can
 *  only ever be asserted by a name census, and this project has been bitten twice by a census
 *  that could not see a site which RECOMPUTES a value. A pure function can be CALLED by the
 *  suite with roll 89.9 and made to go genuinely red.
 *  ⚠️ And because "a landed file is not a landed feature" (`SC-§36.1` — a built-and-tested
 *  trigger shipped here with ZERO callers and nothing failed), the two tests over this file each
 *  carry a CALL-SITE CENSUS half that fails if either function stops being called.
 */
struct FSiegeDeathCameraStatics
{
	/**
	 *  ⭐ HALF (a) — THE APPLICATION. The only rotation the death path may hand to a controller or
	 *  to the ghost's spawn: the dead pawn's FACING and ⛔ nothing else.
	 *
	 *  ⛔ THE DECISION, ARGUED (the row's clause 3a asks for it): the roll is ⛔ SUPPRESSED, ⛔ not
	 *  kept as an effect. A deliberate death tilt is a few degrees about a level horizon; ≈90°
	 *  with the horizon VERTICAL is a broken frame, and nothing in the GDD, in `GHOST-§` or in any
	 *  handoff asks for a tilt. The ghost's own design sentence — "at the death location, facing
	 *  the way the hero was facing … the swap is meant to read as continuous"
	 *  (SiegeGameMode.cpp:828-830) — is a claim about FACING, i.e. about yaw. Pitch and roll were
	 *  never part of it and are dropped.
	 *
	 *  ⚠️ PITCH IS DROPPED TOO, ⛔ deliberately and ⛔ not as scope creep: this rotation is also the
	 *  GHOST'S SPAWN ROTATION, and a walking `ACharacter` spawned pitched is as wrong as one
	 *  spawned rolled. It matches the in-repo yaw-only contract the codebase already states in
	 *  these words at `SiegeGameMode.cpp:1247` ("Yaw-only on purpose (pitch/roll 0 …)").
	 *
	 *  @param DeadPawnRotation  the dead hero's world actor rotation, exactly as read.
	 *  @return `FRotator(0, DeadPawnRotation.Yaw, 0)` — TOTAL, PURE, allocation-free.
	 */
	static FRotator MakeDeathViewRotation(const FRotator& DeadPawnRotation);

	/**
	 *  ⭐ HALF (b) — THE RESET. Returns the caller's view rotation with ROLL forced to 0 and pitch
	 *  and yaw preserved ⛔ EXACTLY.
	 *
	 *  ⛔ PITCH AND YAW ARE ⛔ NOT TOUCHED, and that is the whole point of the function existing
	 *  instead of `FRotator::ZeroRotator`: a reset that also snapped the player's look direction
	 *  would be a second, self-inflicted defect. ⭐ The test over this asserts BOTH sides — that
	 *  roll goes to 0 AND that pitch/yaw survive byte-for-byte — so a lazy `return
	 *  FRotator::ZeroRotator;` fails just as loudly as a `return CurrentViewRotation;`.
	 *
	 *  ⚠️ DECLARED EDGE, ⛔ not hidden: a rotation whose roll is ±180 is an upside-down view whose
	 *  pitch/yaw are the mirrored pair; zeroing its roll levels the horizon but leaves the mirrored
	 *  pitch/yaw. The measured defect is ≈90 and the reset's contract is "roll returns to 0", which
	 *  this satisfies for every input. Flagged for the gate rather than silently special-cased —
	 *  a normalisation branch here would be untestable guesswork about an input never observed.
	 *
	 *  @param CurrentViewRotation  the controller's current control rotation.
	 *  @return the same pitch and yaw with roll 0 — TOTAL, PURE, allocation-free.
	 */
	static FRotator LevelViewRoll(const FRotator& CurrentViewRotation);
};
