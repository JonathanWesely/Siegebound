// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/FogVolume.h"

#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator — the ONE fog-state actor lookup (Find / FindOrSpawn) + the hero height sample
#include "GitClaudeUnrealTest.h"
#include "Math/UnrealMathUtility.h"
#include "Siegebound/HeroCharacter.h"  // TASK-982 (J-F15): the DURATION accessor samples the caster team's LIVING hero Z
#include "Siegebound/ScatterConfig.h"  // TASK-1068: USiegeScatterConfig::ArenaHalfExtent — the ONE owner of the arena extent the fog box is DERIVED from (SC-§34)
#include "TimerManager.h"              // TASK-1068: the visual-expiry WAKE-UP (⛔ not a second deadline — see FogVolume.h's one-way-door paragraph)
#include "UObject/UObjectGlobals.h"    // TASK-1068: GetDefault<> and LoadClass<> (explicit IWYU — no compile verifies transitive pulls; the SessionMenuWidget.cpp precedent)

AFogVolume::AFogVolume()
{
	// No per-frame work and nothing to draw: the fog's liveness is a COMPARISON against the world
	// clock, evaluated by whoever asks, so there is nothing for a tick to do (TASK-004
	// never-per-tick law). ⛔ A ticking expiry would also be a SECOND source of truth about the
	// same deadline — the exact shape FOG-§10.1 forbids.
	// ⭐⭐ AND IT STAYS FALSE THROUGH TASK-1068, WHICH IS THE POINT WORTH RECORDING: giving the
	// visual a lifetime did NOT need a tick. One wake-up whose instant is DERIVED from the
	// deadline replaces 60 polls a second that would each re-ask a question whose answer is
	// already known — and a poll would have been the natural way to get this wrong.
	PrimaryActorTick.bCanEverTick = false;

	// ⛔⛔ NO ROOT COMPONENT, NO MESH, NO DECAL, NO COLLISION — see the class doc. THIS actor
	// renders nothing; TASK-841 owns everything visible. An empty actor is the deliberate shape
	// here, not an unfinished one.
	// ⚠️ NARROWED 2026-09-05 (TASK-1068): this comment used to say the actor was "the timer's
	// home AND NOTHING ELSE". It now also owns the fog visual's LIFETIME — it spawns and destroys
	// BP_SiegeFog, a SEPARATE actor. ⛔ That is not a component and not a draw call: this class
	// still has no geometry of its own, and the sentence above is about THIS actor's body.

	// ⚖️ NET RELEVANCY TIER A — REPLICATED WHEN M8 LANDS. bReplicates is deliberately left at the
	// AActor default (false) and is never touched today, the AAncientGround/ACommanderNpc idiom.
	// The class doc names exactly what M8 adds and the server-clock caveat that comes with it.
}

AFogVolume* AFogVolume::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr; // no world (CDO/test context) — never a crash, and never fog
	}

	for (TActorIterator<AFogVolume> It(World); It; ++It)
	{
		AFogVolume* const Volume = *It;
		if (IsValid(Volume))
		{
			// Exactly one instance exists by construction — FindOrSpawn is the ONLY spawn site in the
			// project and it LOOKS BEFORE IT CREATES, so the first valid hit is the whole answer.
			//
			// ⛔⛔ CORRECTED 2026-09-05 (TASK-1053) — THE INVARIANT ABOVE IS TRUE; THE GROUND THIS
			// COMMENT USED TO GIVE FOR IT WAS FALSE. It said TActorIterator's order is stable "so a
			// level that ALSO placed a BP_SiegeFog answers the same actor on every call rather than
			// churning between two" — i.e. it TIE-BROKE BETWEEN TWO CANDIDATES IN A WORLD WHERE THE
			// SECOND ONE CANNOT EXIST. This loop is TActorIterator<AFogVolume>, and BP_SiegeFog's
			// parent is BP_FogArea_C (read back live under TASK-1043, committed ef2c901) ⇒ it is NOT
			// an AFogVolume subclass and THIS ITERATOR NEVER SEES IT. ⛔ There is no tie to break —
			// not a tie that happens to break stably.
			//
			// ⚠️⚠️ WHAT WOULD COMPETE, AND IT IS THE HALF WORTH READING: a Blueprint child of
			// AFogVolume ITSELF. TActorIterator matches SUBCLASSES, and FindOrSpawn calls Find BEFORE
			// it spawns ⇒ a level-placed child would be RETURNED FROM HERE AS THE ONE AUTHORITATIVE
			// FOG-STATE ACTOR, with its own overrides of the five EditDefaultsOnly tunables IN FORCE,
			// and the native spawn in FindOrSpawn would never run.
			// ⛔ The AUTHORITATIVE statement of the parent fact and of the DORMANT-with-a-live-wire
			// ruling over that hazard is the `CoreRedirects` paragraph in the FogVolume.h class doc —
			// read it THERE rather than re-deriving it here, and ⛔ IF THIS COMMENT AND THAT PARAGRAPH
			// EVER DISAGREE, THE HEADER WINS.
			return Volume;
		}
	}

	// No fog-state actor in this world: the Fog card has not been played yet (the actor is spawned
	// on first cast), or this is a map that never will. ⇒ "no fog", which is the pre-fog game.
	return nullptr;
}

AFogVolume* AFogVolume::FindOrSpawn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	if (AFogVolume* const Existing = Find(World))
	{
		return Existing;
	}

	// ⛔ THE ONE SPAWN SITE IN THE PROJECT. AlwaysSpawn because this actor has no collision
	// primitive at all — there is nothing for the default handling to test against, and a
	// state holder must never be refused for standing where something else stands.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFogVolume* const Spawned = World->SpawnActor<AFogVolume>(AFogVolume::StaticClass(), FTransform::Identity, SpawnParams);
	if (!Spawned)
	{
		// The caller logs and refuses; saying it here too would double the noise on one event.
		return nullptr;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] AFogVolume spawned — the fog-state actor now exists for this match (FOG-§10.1: one state object)."),
		*GetNameSafe(Spawned));

	return Spawned;
}

bool AFogVolume::RaiseFog()
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return false;
	}

	// ⛔⛔⛔ J-F19 / FOG-§10.6 — THE PREVENTION WINDOW REFUSES NEW FOG, AND THIS IS THE GUARD THAT
	// MAKES FOG-§10.3'S FOURTH STATE ("fogged AND shielded") UNREACHABLE BY CONSTRUCTION. It sits
	// in the ONE writer rather than at the call site precisely so a second caller cannot get it
	// backwards. Jonathan: prevention "will not allow any new fog to come in".
	// ⛔ NOTHING is written on this path — not the deadline, not a partial stamp, nothing — so the
	// two properties this row owns hold structurally rather than by care: the caller propagates
	// false, ASiegePlayerController refunds the full cost and never reaches ConfirmInstantDraw,
	// ⇒ ⛔ ZERO gold moves AND ⛔ the card stays in hand. They are TWO assertions, never one.
	// ⛔ A SILENT no-op is banned in every branch of this mechanic, so it logs; the PLAYER-FACING
	// message carrying the LIVE seconds remaining is TASK-989's, in SiegePlayerController.cpp,
	// and it reads GetFogPreventionSecondsRemaining() at CLICK time rather than here.
	if (IsFogPrevented())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Fog REFUSED — the BrightSun prevention window is up for another %.0f s (J-F19: no gold spent, card kept). ")
			TEXT("The state is unchanged; nothing was stamped."),
			*GetNameSafe(this), GetFogPreventionSecondsRemaining());
		return false;
	}

	// ⭐⭐ REFRESH, NEVER STACK (J-F16), AND IT IS THE `=` THAT GUARANTEES IT.
	// ⛔ NEVER `+=`. A second Fog cast during fog RESETS the full 5 minutes — his words, exactly —
	// and this line does that without an `if (IsFogActive())` branch, so there is no guard for a
	// future editor to get backwards. ⚠️ The previous deadline is DISCARDED even when it was
	// LONGER than the new one; that is deliberate and it is what "reset to 5 minutes" means. It is
	// NOT the `max(remaining, new)` refresh ASummonedUnit::ApplyFreeze uses — that rule is the
	// freeze's, and copying it here would make a re-cast do nothing near the end of a fog.
	FogActiveUntilTimeSeconds = World->GetTimeSeconds() + static_cast<double>(FogDurationSeconds);

	// ⭐⭐⭐ THE VISUAL, TASK-1068 — AND THIS ONE LINE IS THE WHOLE REPAIR THAT ROW EXISTS FOR.
	// BP_SiegeFog shipped CORRECT, integration-checked and COMMITTED with ZERO CALLERS: every
	// reference to it in all of Source/ was inside a COMMENT, so he paid 50 gold, the army went
	// 87.8% blind, and NOTHING APPEARED on screen.
	// ⛔ It is called AFTER the assignment, never before: RefreshFogVisual reads IsFogActive(),
	// which reads the deadline this line just wrote. Called first it would read the OLD state and
	// do exactly the wrong thing on the first cast of a match.
	// ⛔ It is called UNCONDITIONALLY on this path rather than guarded by "was fog already up?":
	// the reconciler is idempotent, and a guard here would be a second place that has to know
	// whether the visual exists.
	RefreshFogVisual();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Fog raised for %.0f s (refresh, never stack — J-F16); it lifts at world time %.1f."),
		*GetNameSafe(this), FogDurationSeconds, FogActiveUntilTimeSeconds);
	return true;
}

bool AFogVolume::ApplyBrightSun(ETeamId CasterTeam)
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return false;
	}

	// ⭐ SAMPLED ONCE, HERE, AT THE INSTANT OF THE CAST (J-F15, verbatim: "the height is sampled at
	// the time that the card is cast"). ⛔ The window never re-computes afterwards — a continuously
	// sampled window would SHRINK while he descends, i.e. a timer that runs backwards.
	// ⛔ This is the ONLY place the answer is ever frozen; the accessor itself is live on purpose.
	const float NewWindowSeconds = GetBrightSunWindowSeconds(CasterTeam);

	// ⛔⛔ J-F18 IS A BRANCH (FOG-§10.7 (A)) — ⛔ not `max`, ⛔ not refresh, ⛔ not a blanket refuse.
	// ⚠️ STRICT `<`, and the boundary is a DECLARED DEFAULT rather than his word: he wrote "LESS
	// than", so EQUAL RESETS (a legal, if pointless, cast). A float-equal window is unreachable in
	// practice, which is exactly why the reading has to be the literal one rather than the
	// convenient one.
	// ⭐ The delta from the struck `max(remaining, new)` default is SMALLER than it looks and is
	// worth naming so nobody over-builds it: `max` and "reset if longer" give the IDENTICAL
	// remaining time in the LONGER case. They diverge ONLY here, in the shorter case — where `max`
	// would have silently kept the timer while BILLING 60 gold and EATING the card. The default's
	// arithmetic was right and its economics were wrong, and no DURATION test could have caught it.
	const float RemainingSeconds = GetFogPreventionSecondsRemaining();
	if (NewWindowSeconds < RemainingSeconds)
	{
		// ⛔ NOTHING IS WRITTEN — the stored expiry stays BIT-IDENTICAL and zero gold moves (the
		// caller refunds on false, exactly as for the Fog refusal above). ⛔ The two-value message
		// ("would reduce fog prevention time from X to Y") is TASK-991's, in
		// SiegePlayerController.cpp; it computes Y by calling GetBrightSunWindowSeconds rather than
		// by casting the card to find out whether to cast it. ⛔ Do not build that message here.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] BrightSun REFUSED — a cast from this height would SHORTEN the window from %.0f s to %.0f s (J-F18). ")
			TEXT("The stored expiry is unchanged and no gold moves."),
			*GetNameSafe(this), RemainingSeconds, NewWindowSeconds);
		return false;
	}

	// ⛔⛔⛔ THE ONE-WAY DOOR, AND IT IS THIS LINE — HIS SENTENCE: "Even when the 'bright sun' fog
	// prevention timer ends, the fog that was cleared STILL REMAINS CLEAR."
	// The fog deadline is ZEROED, ⛔ not suspended and ⛔ not remembered. There is therefore nothing
	// to resume when the shield below lapses, so SHIELDED expires to CLEAR by construction rather
	// than by an expiry handler somebody could forget to write. ⛔ A "restore the remaining fog"
	// implementation would be a FAIL against his words.
	// ⛔ It ZEROES rather than adding a `bool bFogCleared`: an EXPIRED deadline is non-zero and also
	// clear, so a second representation would immediately disagree with this one (qa/TASK-1011.md
	// NIT-5). ⭐ It is also what makes "fogged AND shielded" unreachable from this direction.
	const bool bWasFogged = IsFogActive();
	FogActiveUntilTimeSeconds = 0.0;

	// ⭐ THE SECOND SCALAR, STAMPED. `=`, never `+=` — the same refresh-never-stack shape as
	// RaiseFog, for the same reason: J-F18's LONGER case is a RESET to the new window, and an
	// accumulate would let a player bank hours by re-casting from the same perch.
	FogPreventedUntilTimeSeconds = World->GetTimeSeconds() + static_cast<double>(NewWindowSeconds);

	// ⭐⭐⭐ THE VISUAL, TASK-1068 — EXIT (ii), AND IT IS REQUIRED RATHER THAN NICE. 🧑 His
	// FogClear card is LIVE and costs 60 gold. A visual that only cleared on natural EXPIRY would
	// mean PAYING TO BURN OFF FOG THAT IS STILL ON SCREEN — the mechanic would be perfect and the
	// card would look broken, which is the same defect as having no visual at all wearing a
	// different face.
	// ⛔ AFTER both writes, on the SUCCESS path only: the J-F18 refusal above returns without
	// touching a scalar, so there is nothing there to reconcile and calling it would read as an
	// effect on a path whose entire contract is that it has none.
	RefreshFogVisual();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] BrightSun: fog %s and PREVENTED for %.0f s (was %.0f s remaining; sampled once at cast — J-F15). ")
		TEXT("It lapses at world time %.1f, back to CLEAR and never to FOGGED."),
		*GetNameSafe(this), bWasFogged ? TEXT("BURNED OFF") : TEXT("already clear"),
		NewWindowSeconds, RemainingSeconds, FogPreventedUntilTimeSeconds);
	return true;
}

void AFogVolume::ResetFog()
{
	// Play Again / match reset ⇒ CLEAR, BOTH timers zeroed (FOG-§10.3). Unconditional and
	// idempotent: resetting a world that was never fogged is a no-op, which is why the caller
	// needs no check.
	FogActiveUntilTimeSeconds = 0.0;

	// ⭐⭐ AND THE SECOND ONE, ADDED HERE RATHER THAN AT THE CALL SITE (TASK-982): a match-2 player
	// must inherit neither match-1 fog NOR match-1 immunity to it. ⛔ ASiegeGameMode::PlayAgain is
	// BYTE-UNCHANGED by this row, which is the point — the SC-§62 exception granted to TASK-998
	// holds only while the game mode learns NO fog policy. It may tell the volume to clear; it may
	// not know a duration, a ceiling, a density or a window. A second reset entry point, or a
	// second loop in PlayAgain, would leak exactly the policy that exception forbids.
	FogPreventedUntilTimeSeconds = 0.0;

	// ⭐⭐⭐ THE VISUAL, TASK-1068 — EXIT (iii), AND MISSING IT LEAVES A FOG CORPSE IN MATCH 2.
	// The deadlines above would read CLEAR while a fully opaque box stayed in the world with
	// nothing left that would ever destroy it: a match-2 player blinded by match-1 fog the
	// simulation believes is gone. ⛔ "Wait 4 more minutes" is not a reset, and neither is
	// "wait forever".
	// ⭐ It goes HERE and not in ASiegeGameMode::PlayAgain for the SAME reason the second scalar
	// did: the game mode stays BYTE-UNCHANGED and learns no fog policy — not a duration, not a
	// ceiling, not a density, not a window, and ⛔ not an asset path.
	// ⚠️ SAFE INSIDE THE CALLER'S TActorIterator<AFogVolume> LOOP: the actor this destroys is
	// BP_SiegeFog, which is NOT an AFogVolume subclass and was never in that iteration; and
	// UWorld's removal NULLS the level's actor slot rather than compacting the array, so no
	// index the iterator still holds can shift.
	RefreshFogVisual();
}

bool AFogVolume::IsFogActive() const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return false; // no world can never be under fog — the same fail-toward-clear rule as the seam
	}

	// STRICT `<`: at the exact instant of expiry the fog is already gone. ⛔ Fails toward CLEAR on
	// every degenerate input — a zeroed deadline, a reset, a world whose clock has not started —
	// and never toward a blind field, which is the direction that would take combat down.
	return World->GetTimeSeconds() < FogActiveUntilTimeSeconds;
}

bool AFogVolume::IsFogPrevented() const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return false; // no world is never shielded — the same fail-toward-CLEAR rule as IsFogActive
	}

	// The exact sibling of IsFogActive, over the second scalar: STRICT `<`, so at the instant of
	// expiry the shield is already down and the next Fog cast is legal.
	return World->GetTimeSeconds() < FogPreventedUntilTimeSeconds;
}

float AFogVolume::GetFogPreventionSecondsRemaining() const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	// ⛔⛔ RECOMPUTED FROM THE CLOCK ON EVERY CALL — ⛔ never a cached remainder (FOG-§10.6). His
	// ruling is that the message carries "the ACTUAL amount of time left", and a value captured at
	// cast time would be stale by exactly the elapsed duration: the message would count down from
	// the wrong number, or not count down at all.
	// ⛔ CLAMPED AT 0 rather than allowed to go negative: 0 is the caller's "not shielded", and an
	// expired deadline must read exactly the same as a zeroed one so that IsFogPrevented() and
	// this function can never disagree about which state the machine is in.
	const double RemainingSeconds = FogPreventedUntilTimeSeconds - World->GetTimeSeconds();
	return (RemainingSeconds > 0.0) ? static_cast<float>(RemainingSeconds) : 0.f;
}

float AFogVolume::GetBrightSunWindowSeconds(ETeamId CasterTeam) const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		// Fail toward the BASE window, ⛔ never toward a bonus — the same direction every degenerate
		// input in this file takes.
		return BrightSunBaseDurationSeconds;
	}

	// ⛔⛔ NO TRACE (J-F13, ruled): the datum is the FLAT constant ArenaGroundReferenceZUU, the same
	// anywhere on the map, hills EXCLUDED. All this sample does is READ the hero's Z.
	// The LIVING caster-team hero, found the same way USpellLibrary::ResolveLineOrigin finds its
	// line origin (live TActorIterator, team filter, !IsDead) — deliberately the shipped lookup
	// rather than a second convention. ⛔ No living hero (the bot's case) ⇒ the BASE window: the
	// card still works, it just earns no height.
	float HeroZUU = ArenaGroundReferenceZUU;
	for (TActorIterator<AHeroCharacter> It(World); It; ++It)
	{
		const AHeroCharacter* const Hero = *It;
		if (IsValid(Hero) && Hero->GetTeamId() == CasterTeam && !Hero->IsDead())
		{
			// ⚠️ ACTOR ORIGIN Z (capsule centre), deliberately — it is the same quantity
			// ASummonedUnit::ComputeOutputDamage feeds HeightAdvantageMultiplier, so the two
			// elevation lanes measure the same way. ⛔ NOT a hand-rolled "feet" correction: the
			// capsule half-height offset is a declared residual (see the handoff), and inventing a
			// foot offset here would make this lane disagree with the damage lane by construction.
			HeroZUU = static_cast<float>(Hero->GetActorLocation().Z);
			break;
		}
	}

	return BrightSunWindowSeconds(
		HeroZUU,
		ArenaGroundReferenceZUU,
		BrightSunBaseDurationSeconds,
		BrightSunBonusSecondsPerStep,
		BrightSunHeightStepUU);
}

float AFogVolume::BrightSunWindowSeconds(float HeroZUU, float GroundReferenceZUU, float BaseSeconds, float BonusSecondsPerStep, float HeightStepUU)
{
	// ⛔ THE ZERO-DIVIDE GUARD, APPLIED BEFORE THE DIVISION — the shipped
	// ASummonedUnit::HeightAdvantageMultiplier doctrine, copied on purpose rather than re-invented.
	// HeightStepUU is an EditDefaultsOnly float a designer can zero. Written as !(X > 0.f) rather
	// than (X <= 0.f) so a NaN step — which fails EVERY comparison — also lands here instead of
	// propagating into a card duration. A disabled step means NO bonus (exactly the base), never
	// an explosion.
	if (!(HeightStepUU > 0.f) || !FMath::IsFinite(HeroZUU) || !FMath::IsFinite(GroundReferenceZUU))
	{
		return BaseSeconds;
	}

	// ⭐ HEIGHT ABOVE THE FLAT-GRASS DATUM (J-F13). The FMath::Max is the "bonus only when
	// positive" rule: standing at or below the datum falls to 0 here and earns exactly the base.
	// ⛔ There is deliberately no negative branch — he asked for a bonus, and inventing a
	// below-ground malus is inventing a mechanic (the HeightAdvantageMultiplier precedent).
	const float HeightAboveGroundUU = FMath::Max(0.f, HeroZUU - GroundReferenceZUU);

	// ⭐ `floor`, ⛔ NOT round and ⛔ NOT the continuous form the DAMAGE lane uses. The two lanes
	// differ here on purpose: damage is continuous (HIGH-§2 row R-4 — invisible breakpoints on a
	// hillside a player cannot aim for), while this is a WHOLE MINUTE of prevention, which is a
	// discrete reward the player can count. His words are "increases by 1 minute", not "increases
	// proportionally". ⛔ Rounding would grant the next minute HALF a step early.
	const float CompletedSteps = FMath::FloorToFloat(HeightAboveGroundUU / HeightStepUU);

	// ⛔ UNCAPPED (J-F14, RULED — he accepted the proceeding default explicitly). ⭐ If he ever
	// wants a cap the sanctioned shape is ONE EditDefaultsOnly maximum (0 = uncapped), ⛔ never a
	// magic number inside this formula — the HIGH-§5 precedent.
	return BaseSeconds + BonusSecondsPerStep * CompletedSteps;
}

// ═══ ⭐⭐⭐ THE FOG VISUAL'S LIFETIME (TASK-1068; SC-§36.1 instance 2) ══════════════════════════
//
// ⛔⛔ THE DEFECT, MEASURED AT b6a44b8 AND WORTH RE-READING BEFORE EDITING ANYTHING BELOW:
// BP_SiegeFog shipped correct, integration-checked and committed (ef2c901) with ZERO CALLERS.
// EVERY reference to it in all of Source/ was INSIDE A COMMENT; L_Arena.umap held ZERO
// occurrences. ⇒ 🧑 Jonathan played the card, the army went 87.8% blind, and the screen did not
// change. ⭐ An asset with no caller is not a feature; it is a file.
//
// ⭐⭐ THE THREE EXITS FROM FOGGED, AND THE ENUMERATION IS COMPLETE BY CONSTRUCTION RATHER THAN
// BY INSPECTION: RaiseFog, ApplyBrightSun and ResetFog are the ONLY writers of either deadline in
// the project (FogVolume.h's three-state block; a fourth writer is banned there BY NAME). Every
// one of them calls RefreshFogVisual() immediately after it writes. ⇒
//   (i)   NATURAL EXPIRY — the deadline simply passes. ⛔ NO writer runs, so NOTHING would ever
//         notice: this is the one exit that has to be MANUFACTURED, and the expiry wake-up below
//         is that manufacture. ⛔ Without it: PERMANENT FOG, FOREVER, with a green suite.
//   (ii)  BRIGHT SUN — ApplyBrightSun zeroes the fog deadline. ⛔ Without it: 60 gold buys the
//         removal of fog that is still on screen.
//   (iii) MATCH RESET — ResetFog zeroes both. ⛔ Without it: a fog corpse survives into match 2.
// ⚠️ TEARDOWN (EndPlay) is NOT a fourth exit — the state does not change there, the state's OWNER
// ceases to exist. It is handled anyway, for the reason written on the declaration.

void AFogVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ⛔ CLEAR THE WAKE-UP FIRST. A timer bound to `this` fires into a dying actor otherwise, and
	// the ABarracks/ATower EndPlay precedent is to clear synchronously inside teardown rather than
	// to rely on the timer manager noticing.
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FogVisualExpiryTimerHandle);
	}

	// ⛔ AND TAKE THE VISUAL WITH US. On a world teardown this is free — everything dies anyway —
	// but EndPlay also fires for Destroyed and LevelTransition, where the WORLD SURVIVES. There,
	// skipping this would leave a fully opaque box in the world with NOBODY HOLDING ITS
	// REFERENCE: fog no code could ever find again, i.e. permanent fog arriving through the back
	// door of a row whose entire subject is not shipping permanent fog.
	DestroyFogVisual();

	Super::EndPlay(EndPlayReason);
}

const FSoftClassPath& AFogVolume::FogVisualClassPath()
{
	// ⛔⛔ THE ONE PLACE THIS PATH IS WRITTEN IN THE PROJECT. Tests/SiegeFogVisualTest.cpp calls
	// THIS function and asserts the package it names is on disk, so renaming or moving the asset
	// goes RED here instead of going silently invisible in game. ⛔ Do not inline this string at a
	// call site; the whole point is that there is exactly one of it and a test can hold it.
	// ⚠️ `.BP_SiegeFog_C` is the GENERATED CLASS. Dropping the suffix resolves to the UBlueprint
	// ASSET, which is not a UClass and cannot be spawned — and it fails by returning null.
	static const FSoftClassPath VisualClassPath{FString(TEXT("/Game/Blueprints/BP_SiegeFog.BP_SiegeFog_C"))};
	return VisualClassPath;
}

FTransform AFogVolume::FogVisualTransform(const FVector2D& ArenaHalfExtentUU, float GroundReferenceZUU)
{
	// ⭐ HORIZONTAL: the arena's own half-extent (its ONE owner is USiegeScatterConfig) PLUS the
	// margin. ⛔ TASK-841 §5.2 measured what happens without it: sized to the arena EXACTLY, a
	// camera at (24000, 11000, 1200) saw green grass, the blue castle and crisp trees — mean luma
	// 0.6249, i.e. NO FOG AT THE WALL — against 0.6454 (total white-out) with the overhang.
	// ⇒ a player standing at the arena edge would have been THE ONLY ONE WHO COULD SEE.
	const double HalfExtentXUU = ArenaHalfExtentUU.X + static_cast<double>(FogVisualHorizontalMarginUU);
	const double HalfExtentYUU = ArenaHalfExtentUU.Y + static_cast<double>(FogVisualHorizontalMarginUU);

	// ⭐ VERTICAL: from one margin BELOW the flat-grass datum (J-F13) to the ceiling above it.
	// ⛔ The below-ground start is not decoration — a camera standing ON the ground would
	// otherwise sit exactly on the box's bottom face, where the mask's feather thins the fog out,
	// which is the §5.2 failure again rotated 90°.
	const double BottomZUU = static_cast<double>(GroundReferenceZUU) - static_cast<double>(FogVisualHorizontalMarginUU);
	const double TopZUU = static_cast<double>(GroundReferenceZUU) + static_cast<double>(FogVisualCeilingAboveGroundUU);

	// ⛔ The box is NOT vertically centred on the datum, so the centre is DERIVED from the two
	// edges rather than assumed to be the ground.
	const double CentreZUU = (BottomZUU + TopZUU) * 0.5;
	const double HalfExtentZUU = (TopZUU - BottomZUU) * 0.5;

	// ⛔ FULL extent over the cube's edge length — a unit conversion, not a magic number. The
	// vendor mesh is inherited and cannot be scaled on the child through MCP (TASK-841 §2's
	// declared tooling gap), which is exactly why the arena scale rides this transform.
	const FVector Scale3D(
		(HalfExtentXUU * 2.0) / static_cast<double>(FogVisualUnitCubeEdgeUU),
		(HalfExtentYUU * 2.0) / static_cast<double>(FogVisualUnitCubeEdgeUU),
		(HalfExtentZUU * 2.0) / static_cast<double>(FogVisualUnitCubeEdgeUU));

	// ⛔ Centred on X and Y: the arena's own extent is symmetric about the origin (castles at
	// ±25,000), so an offset here would be inventing an asymmetry the fog is forbidden to have.
	return FTransform(FRotator::ZeroRotator, FVector(0.0, 0.0, CentreZUU), Scale3D);
}

void AFogVolume::RefreshFogVisual()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		// No world (CDO / test context) — nothing to spawn into and nothing that could already be
		// spawned. The same fail-toward-nothing direction the rest of this file takes.
		return;
	}

	// ⭐⭐⭐ IsFogActive() IS THE ONE PREDICATE, AND IT IS ASKED HERE RATHER THAN REMEMBERED.
	// ⛔ This function stores NO deadline, keeps NO bool and NEVER re-types FogDurationSeconds:
	// everything below is derived from FogActiveUntilTimeSeconds, the single source of truth.
	// ⭐ That is also what makes the wake-up harmless in both directions — a timer that fires
	// EARLY finds fog still active and re-arms; one that fires LATE destroys a hair late. Neither
	// can make the machine answer differently in two places.
	if (!IsFogActive())
	{
		// EXITS (i), (ii) and (iii) all land here — expiry, BrightSun and match reset are one
		// branch on purpose, because they are one FACT: the fog is not up, so the box must go.
		GetWorldTimerManager().ClearTimer(FogVisualExpiryTimerHandle);
		DestroyFogVisual();
		return;
	}

	// Fog IS up. Spawn only if there is nothing valid already — RaiseFog during fog is a REFRESH
	// (J-F16), and a refresh must not stack two boxes any more than it stacks two timers.
	if (!IsValid(FogVisualActor.Get()))
	{
		SpawnFogVisual();
	}

	// ⭐⭐ THE WAKE-UP, DERIVED — never `FogDurationSeconds`, which would be a SECOND copy of the
	// duration and would be WRONG on every path but the first cast (a refresh, a BrightSun that
	// was refused, a re-arm). This subtraction cannot be non-positive: IsFogActive() is exactly
	// `now < deadline`, so reaching this line means the difference is > 0.
	// ⛔ The FMath::Max is a float-underflow guard, not a policy: SetTimer with a rate of 0 CLEARS
	// the handle instead of firing, and a cleared expiry handle is permanent fog.
	const double SecondsUntilFogLifts = FogActiveUntilTimeSeconds - World->GetTimeSeconds();
	const float WakeUpDelaySeconds = FMath::Max(static_cast<float>(SecondsUntilFogLifts), UE_KINDA_SMALL_NUMBER);

	// ⛔ NOT looping: it re-arms itself from the CURRENT deadline every time it runs, so a fog
	// refreshed at t+299 pushes the next wake-up out instead of firing on the old schedule.
	GetWorldTimerManager().SetTimer(
		FogVisualExpiryTimerHandle, this, &AFogVolume::RefreshFogVisual, WakeUpDelaySeconds, /*bLoop=*/ false);
}

void AFogVolume::SpawnFogVisual()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	// ⛔⛔ LOUD IN THE LOG, NEVER IN THE GAMEPLAY. 🧑 His spell VFX were INVISIBLE rather than
	// ERRORING because the cards' spawn is null-safe (TASK-1025), and that silence is the whole
	// reason nobody noticed for weeks. ⇒ Error level, naming the EXACT path AND the function that
	// produced it — and then RETURNING, because the fog MECHANIC must keep working with no visual.
	// ⛔ An art failure may never refuse a 50-gold card, consume-and-abort, or change one clamp.
	UClass* const VisualClass = FogVisualClassPath().TryLoadClass<AActor>();
	if (!VisualClass)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] ⛔ THE FOG VISUAL FAILED TO LOAD — '%s' (AFogVolume::FogVisualClassPath) resolved to nothing. ")
			TEXT("The fog MECHANIC is unaffected and is NOT refused: it is up, and every unit on both sides is clamped ")
			TEXT("to the fog vision ceiling — so the battlefield will LOOK clear while the army is blind. ")
			TEXT("Check that Content/Blueprints/BP_SiegeFog.uasset exists and that the path keeps its '_C' generated-class suffix."),
			*GetNameSafe(this), *FogVisualClassPath().ToString());
		return;
	}

	// ⭐ DERIVED, NEVER TYPED (SC-§34). The arena half-extent comes from its ONE owner and the
	// ground datum from this class's own J-F13 constant, so resizing the arena moves the fog box.
	// ⚠️ DECLARED RESIDUAL, stated rather than discovered: this is the USiegeScatterConfig CDO
	// lane — the same fallback UWarMapWidget::ResolveArenaHalfExtent uses — and a saved
	// DA_BattlefieldScatter can override that value for the SCATTER actor without moving this box.
	// ⛔ The live route is closed today: ASiegeBattlefieldScatter::ScatterConfig is `protected`.
	// The CDO value is also exactly what TASK-841 derived and read back live, so today the two
	// lanes agree; the day they diverge, this is where to look.
	const FTransform SpawnTransform = FogVisualTransform(
		GetDefault<USiegeScatterConfig>()->ArenaHalfExtent, ArenaGroundReferenceZUU);

	FActorSpawnParameters SpawnParams;

	// Ownership is recorded for net relevancy when M8 lands. ⛔ It is NOT the teardown mechanism —
	// UE does not cascade Destroy() to owned actors, and believing it does is how an orphan
	// happens. EndPlay is the teardown.
	SpawnParams.Owner = this;

	// AlwaysSpawn: this is a 64,000 × 36,000 × 26,000 uu box that deliberately encloses the whole
	// battlefield, so a collision test against what is already standing there would refuse it
	// every single time.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// ⛔⛔ RF_Transient — AND THIS ONE IS INSURANCE AGAINST A STANDING PROJECT LAW, NOT TIDINESS.
	// L_Arena must NEVER be saved. This actor is VISIBLE and world-sized: if anybody ever saved
	// the map with fog up, a fully opaque box would be BAKED INTO THE LEVEL permanently, and no
	// code path in this class would ever destroy it (Find cannot see it — BP_SiegeFog is not an
	// AFogVolume subclass). ⛔ The state actor itself carries no such flag because it draws
	// nothing; the asymmetry is deliberate.
	SpawnParams.ObjectFlags |= RF_Transient;

	AActor* const Spawned = World->SpawnActor<AActor>(VisualClass, SpawnTransform, SpawnParams);
	if (!Spawned)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] ⛔ THE FOG VISUAL CLASS LOADED BUT THE WORLD REFUSED TO SPAWN IT ('%s'). ")
			TEXT("The fog mechanic is unaffected and the card is NOT refused; there will simply be nothing to see."),
			*GetNameSafe(this), *FogVisualClassPath().ToString());
		return;
	}

	// ⛔ HOLD THE REFERENCE. BP_SiegeFog's parent is the VENDOR BP_FogArea_C (read back LIVE under
	// TASK-1043), so it is NOT an AFogVolume subclass and TActorIterator<AFogVolume> will NEVER
	// see it: a Find-style sweep for it later returns NOTHING, SILENTLY. There is no re-finding
	// this actor — if the handle is lost, the box is unreachable forever.
	FogVisualActor = Spawned;

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Fog VISUAL spawned: '%s' at Z=%.0f, scale (%.0f, %.0f, %.0f) — derived from ArenaHalfExtent + a %.0f uu margin ")
		TEXT("(TASK-841 §5.2: sized to the arena exactly, the CORNER rendered clear)."),
		*GetNameSafe(this), *GetNameSafe(Spawned),
		SpawnTransform.GetLocation().Z,
		SpawnTransform.GetScale3D().X, SpawnTransform.GetScale3D().Y, SpawnTransform.GetScale3D().Z,
		FogVisualHorizontalMarginUU);
}

void AFogVolume::DestroyFogVisual()
{
	AActor* const Visual = FogVisualActor.Get();

	// ⛔ The handle is cleared BEFORE the destroy, so nothing reached from Destroy() can ever
	// observe a handle pointing at an actor that is on its way out.
	FogVisualActor = nullptr;

	if (!IsValid(Visual))
	{
		// Nothing was ever spawned, or it is already gone. ⛔ Idempotent on purpose: every exit
		// calls this unconditionally, and a reset of a match that never saw fog must be a no-op.
		return;
	}

	const FString VisualName = GetNameSafe(Visual);
	Visual->Destroy();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Fog VISUAL destroyed ('%s') — the fog has lifted, been burned off by BrightSun, or the match was reset. ")
		TEXT("IsFogActive() is the one predicate that decided this."),
		*GetNameSafe(this), *VisualName);
}
