// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/FogVolume.h"

#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator — the ONE fog-state actor lookup (Find / FindOrSpawn) + the hero height sample
#include "GitClaudeUnrealTest.h"
#include "Math/UnrealMathUtility.h"
#include "Siegebound/HeroCharacter.h" // TASK-982 (J-F15): the DURATION accessor samples the caster team's LIVING hero Z

AFogVolume::AFogVolume()
{
	// No per-frame work and nothing to draw: the fog's liveness is a COMPARISON against the world
	// clock, evaluated by whoever asks, so there is nothing for a tick to do (TASK-004
	// never-per-tick law). ⛔ A ticking expiry would also be a SECOND source of truth about the
	// same deadline — the exact shape FOG-§10.1 forbids.
	PrimaryActorTick.bCanEverTick = false;

	// ⛔⛔ NO ROOT COMPONENT, NO MESH, NO DECAL, NO COLLISION — see the class doc. This actor is
	// the timer's home and nothing else; TASK-841 owns everything visible. An empty actor is the
	// deliberate shape here, not an unfinished one.

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
			// Exactly one instance exists by construction — FindOrSpawn is the only spawn site
			// in the project and it looks before it creates. TActorIterator's order is stable for
			// a fixed world, so a level that ALSO placed a BP_SiegeFog answers the same actor on
			// every call rather than churning between two.
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
