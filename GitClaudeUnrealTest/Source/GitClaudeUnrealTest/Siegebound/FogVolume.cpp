// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/FogVolume.h"

#include "Components/ExponentialHeightFogComponent.h" // TASK-1147: SetVolumetricFog — the ONLY seam that reaches terms 6/7 of ShouldRenderVolumetricFog's six-term conjunction
#include "Engine/ExponentialHeightFog.h"               // TASK-1147: the actor the integrity floor writes IN MEMORY (⛔ never a map save — GFX-§11)
#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator — the ONE fog-state actor lookup (Find / FindOrSpawn) + the hero height sample + TASK-1147's height-fog lookup
#include "GitClaudeUnrealTest.h"
#include "HAL/IConsoleManager.h" // TASK-1147: the ECVF_SetByCode floor + Unset release (explicit IWYU — no compile verifies a transitive pull)
#include "Math/UnrealMathUtility.h"
#include "Siegebound/HeroCharacter.h"  // TASK-982 (J-F15): the DURATION accessor samples the caster team's LIVING hero Z
#include "Siegebound/ScatterConfig.h"  // TASK-1068: USiegeScatterConfig::ArenaHalfExtent — the ONE owner of the arena extent the fog box is DERIVED from (SC-§34)
#include "TimerManager.h"              // TASK-1068: the visual-expiry WAKE-UP (⛔ not a second deadline — see FogVolume.h's one-way-door paragraph)
#include "UObject/UObjectGlobals.h"    // TASK-1068: GetDefault<> and LoadClass<> (explicit IWYU — no compile verifies transitive pulls; the SessionMenuWidget.cpp precedent)

// ═════════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐⭐ THE DEV TRIGGERS — CONSOLE LANE (`TASK-1173`; law `SC-§113`, `SC-§36.1`, `SC-§111`)
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
//  ⛔⛔⛔ WHY THIS BLOCK EXISTS, AND IT IS NOT A CONVENIENCE. Until it landed, ⛔ NOBODY IN THIS
//  PIPELINE COULD EXECUTE ONE LINE OF THIS FILE. Measured, four ways, by `TASK-1167` §5 and
//  written up as `SC-§113` cl. 1: this class exposed ⛔ NO reflected function; its only runtime
//  caller (`USpellLibrary::ResolveSpell`) is a ⛔ plain static; the editor bridge has ⛔ NO
//  function-invocation tool at all (get / set / list properties only); and the one writable
//  `UPROPERTY` deadline is ⛔ polled by nothing. ⇒ every `Error` site in this file was silent, and
//  ⛔ NOBODY COULD SAY WHETHER THAT WAS GOOD NEWS — an error that ⛔ CANNOT fire and one that
//  ⛔ CHOSE not to fire produce ⛔ byte-identical logs, and the default reading of an empty log is
//  *"fine"*.
//
//  ⛔⛔ AND THE TRAP THAT SHAPED THE CHOICE (`SC-§113` cl. 4): ⛔ A `UFUNCTION` ALONE WOULD NOT
//  HAVE FIXED IT, because the tool that would call one ⛔ does not exist. ⇒ the channel had to be
//  one this project has ⛔ MEASURED ITSELF USING. It is: ⭐ every suite run in this repository's
//  history has been `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit"`
//  — i.e. ⛔ THE CONSOLE IS ALREADY OUR EXECUTION LANE, and a console command reaches it by
//  substituting the command list. See `handoffs/TASK-1173-programmer.md` for the exact invocation.
//
//  ⛔ SHIPPING FENCE — ⛔ TWO OF THEM, DELIBERATELY, BECAUSE EACH COVERS A CONFIGURATION THE
//  OTHER MISSES (the house *"non-shipping by construction"* rule from `USiegeCheatManager`,
//  ⛔ not a second pattern):
//    (1) `#if !UE_BUILD_SHIPPING` — the registration itself ⛔ does not exist in a Shipping build;
//    (2) `ECVF_Cheat` — `IConsoleObject::IsEnabled()` refuses cheat objects wherever
//        `DISABLE_CHEAT_CVARS` is set, which is `UE_BUILD_SHIPPING || (UE_BUILD_TEST &&
//        !ALLOW_CHEAT_CVARS_IN_TEST)` (`Misc/Build.h`) ⇒ it also covers ⛔ TEST, which (1) does not.
//  ⚠️ `SC-§111` DISCLOSURE: (1) is a ⛔ deliberate target divergence and it runs in the ⛔ SAFE
//  direction — the shipped game loses a developer trigger and ⛔ no gameplay mechanism. That is
//  the narrow case cl. 3(c) permits; ⛔ nothing in the fog's own mechanism is guarded.
//
//  ⛔⛔ SCOPE — READ THIS BEFORE EDITING: these commands ⛔ RE-IMPLEMENT NOTHING. They call the
//  SAME two doors `USpellLibrary::ResolveSpell` calls (`FindOrSpawn` then `RaiseFog`), in the same
//  order, with ⛔ no bespoke spawn, ⛔ no raw field write and ⛔ no second copy of any policy — the
//  `USiegeCheatManager` discipline applied to a console command. ⛔ A trigger that re-implemented
//  the path would test the trigger, ⛔ not the game.
// ═════════════════════════════════════════════════════════════════════════════════════════════

#if !UE_BUILD_SHIPPING

namespace SiegeFogDevTrigger
{
	// ⛔⛔ EACH COMMAND NAME HAS ⛔ EXACTLY ONE HOME, and it is these three lines. ⛔ This is the
	// house law the three floored console-variable names already obey (`FogRenderFloorCVar*`,
	// declared once in the header and spelled once here), applied to a command: a name typed at
	// BOTH the registration AND the log line is ⛔ TWO copies that can drift, and the drift is
	// invisible — the command would still register, and the log would name a command that no
	// longer exists.
	static const TCHAR* const CommandNameRaise = TEXT("Siege.Fog.Raise");
	static const TCHAR* const CommandNameClear = TEXT("Siege.Fog.Clear");
	static const TCHAR* const CommandNameStatus = TEXT("Siege.Fog.Status");

	/**
	 *  Gate + disclosure for the two WRITING commands. Returns false when there is no world at
	 *  all; returns true — after a loud `Warning` — when the world is an EDITOR world.
	 *  ⛔ A trigger with no world is a ⛔ NO-OP THAT LOOKS LIKE A PULL, which is the whole class of
	 *  defect this block exists to end. ⇒ it is `Warning`, it names the command, and it says what
	 *  the operator must do differently. ⛔ Never silent.
	 */
	static bool IsWorldUsable(const TCHAR* CommandName, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] ⛔ NOTHING HAPPENED — the console resolved ⛔ NO WORLD for this command, so the fog path was ")
				TEXT("never entered. This is the shape of a FALSE PASS: an empty log below this line means ⛔ 'not run', ")
				TEXT("⛔ never 'ran clean'. Run it from a live PIE session, or from a commandlet that has opened a map."),
				CommandName);
			return false;
		}

		// ⛔⛔ THE EDITOR-WORLD HAZARD, SAID OUT LOUD RATHER THAN REFUSED. Spawning into the world
		// the editor currently has open marks that MAP DIRTY, and this project's standing law is
		// that `L_Arena` is ⛔ NEVER saved (`GFX-§11`). ⛔ Refusing here would be worse: it would
		// make the ONE headless channel we have measured ourselves using — the editor commandlet,
		// whose world is an editor world — dead on arrival, which is the very defect this block
		// repairs. ⇒ ⛔ ACT, and make the consequence impossible to miss.
		// ⭐ The visual itself carries `RF_Transient` (see `SpawnFogVisual`), so the box cannot be
		// baked in even if somebody did save; the state actor is what would persist.
		if (!World->IsGameWorld())
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('%s'), not a game/PIE world. The fog-state actor is spawned ")
				TEXT("into the map you currently have OPEN, which ⛔ MARKS IT DIRTY. ⛔ DO NOT SAVE THE MAP (`GFX-§11`): ")
				TEXT("discard, or close the editor without saving. Prefer PIE for anything you intend to look at."),
				CommandName, *World->GetName());
		}

		return true;
	}

	/**
	 *  ⭐⭐⭐ `Siege.Fog.Raise` — THE CARD'S OWN PATH, PULLED BY HAND.
	 *  ⛔ `FindOrSpawn` then `RaiseFog`, which is character-for-character what the `FogCover` arm
	 *  of `USpellLibrary::ResolveSpell` does. ⛔ It does NOT bypass `J-F19`: a `BrightSun` window
	 *  refuses this exactly as it refuses the card, and the `false` is reported rather than
	 *  swallowed — a trigger that could not be REFUSED would be a different code path.
	 */
	static void ExecRaiseFog(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* const CommandName = CommandNameRaise;

		if (!IsWorldUsable(CommandName, World))
		{
			return;
		}

		AFogVolume* const Volume = AFogVolume::FindOrSpawn(World);
		if (!Volume)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] ⛔ NOTHING HAPPENED — AFogVolume::FindOrSpawn returned null, so there is no fog-state actor to ")
				TEXT("raise. This is the same refusal the card takes: the world declined the spawn."),
				CommandName);
			return;
		}

		const bool bRaised = Volume->RaiseFog();

		// ⛔⛔ THE RETURN VALUE IS REPORTED, ⛔ NOT ASSUMED. `false` here is a LEGITIMATE outcome
		// (`J-F19` — the prevention window refuses new fog, nothing is written, no gold moves), and
		// an operator who could not tell it apart from a crash would learn the wrong thing.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] AFogVolume::RaiseFog() executed on '%s' and returned %s. Fog is now %s. ")
			TEXT("⛔ This line is the POSITIVE CONTROL for `SC-§113` cl. 3(c): its presence proves the fog path RAN. ")
			TEXT("A `false` here is not a failure — it is the BrightSun refusal (J-F19), and the state is unchanged."),
			CommandName, *GetNameSafe(Volume),
			bRaised ? TEXT("TRUE") : TEXT("FALSE"),
			Volume->IsFogActive() ? TEXT("UP") : TEXT("DOWN"));
	}

	/**
	 *  ⭐ `Siege.Fog.Clear` — the match-reset door (`ResetFog`), i.e. exit (iii).
	 *  ⛔ Deliberately `Find`, ⛔ never `FindOrSpawn`: creating a fog-state actor in order to tell
	 *  it there is no fog would be inventing work, and it would put an actor in the world that the
	 *  operator did not ask for.
	 *  ⚠️ This is ⛔ NOT the `BrightSun` card — it opens ⛔ no prevention window. Use it to take the
	 *  box down; use the card for the mechanic.
	 */
	static void ExecClearFog(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* const CommandName = CommandNameClear;

		if (!IsWorldUsable(CommandName, World))
		{
			return;
		}

		AFogVolume* const Volume = AFogVolume::Find(World);
		if (!Volume)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("[%s] There is no fog-state actor in this world, so there is nothing to clear — and no actor was ")
				TEXT("created to say so. Nothing was written."),
				CommandName);
			return;
		}

		Volume->ResetFog();

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] AFogVolume::ResetFog() executed on '%s'. Fog is now %s and prevention is now %s. ")
			TEXT("⛔ Both deadlines are ZEROED, which is the match-reset door (FOG-§10.3) — not the BrightSun card."),
			CommandName, *GetNameSafe(Volume),
			Volume->IsFogActive() ? TEXT("UP") : TEXT("DOWN"),
			Volume->IsFogPrevented() ? TEXT("UP") : TEXT("DOWN"));
	}

	/**
	 *  ⭐⭐ `Siege.Fog.Status` — ⛔ READ-ONLY, and it is the instrument, not a convenience.
	 *  ⛔ It writes NOTHING and creates NOTHING, so it can be run BEFORE a raise as the
	 *  before-picture (`SC-§107`) and after one as the read-back. ⛔ Every number is asked of the
	 *  shipped accessors rather than recomputed here — a status command with its own arithmetic
	 *  would be a second opinion about fog liveness, which `FOG-§10.1` forbids.
	 */
	static void ExecLogFogState(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* const CommandName = CommandNameStatus;

		if (!World)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] ⛔ NO WORLD — nothing was read. An empty answer here means 'not run', never 'no fog'."),
				CommandName);
			return;
		}

		const AFogVolume* const Volume = AFogVolume::Find(World);
		if (!Volume)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("[%s] World '%s' holds NO fog-state actor. ⛔ That is 'the first cast has not happened yet', which is ")
				TEXT("a DIFFERENT fact from 'the fog is down' — nothing has been created, so nothing can be asked."),
				CommandName, *World->GetName());
			return;
		}

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] '%s' — fog %s, prevention %s (%.0f s of prevention remain). Read from the shipped accessors ")
			TEXT("(IsFogActive / IsFogPrevented / GetFogPreventionSecondsRemaining), never recomputed here."),
			CommandName, *GetNameSafe(Volume),
			Volume->IsFogActive() ? TEXT("UP") : TEXT("DOWN"),
			Volume->IsFogPrevented() ? TEXT("UP") : TEXT("DOWN"),
			Volume->GetFogPreventionSecondsRemaining());
	}
}

static FAutoConsoleCommandWithWorldAndArgs GSiegeFogRaiseCommand(
	SiegeFogDevTrigger::CommandNameRaise,
	TEXT("DEV: raises the siege fog through the card's own path (AFogVolume::FindOrSpawn then RaiseFog). Refused by a live BrightSun window, exactly as the card is."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeFogDevTrigger::ExecRaiseFog),
	ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs GSiegeFogClearCommand(
	SiegeFogDevTrigger::CommandNameClear,
	TEXT("DEV: takes the siege fog down through the match-reset door (AFogVolume::ResetFog). Opens no prevention window; this is not the BrightSun card."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeFogDevTrigger::ExecClearFog),
	ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs GSiegeFogStatusCommand(
	SiegeFogDevTrigger::CommandNameStatus,
	TEXT("DEV: read-only. Logs whether fog and BrightSun prevention are up, from the shipped accessors. Writes nothing and creates nothing."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeFogDevTrigger::ExecLogFogState),
	ECVF_Cheat);

#endif // !UE_BUILD_SHIPPING

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
//
// ⛔⛔⛔ AND "NOT AN EXIT FROM THE STATE" IS EXACTLY WHY IT HAS TO BE HANDLED (TASK-1148 BLOCKER-1).
// Both things this actor takes hold of OUTLIVE it: the spawned box is another actor, and the
// INTEGRITY FLOOR is three PROCESS-WIDE console variables. The three enumerated exits above all
// run through RefreshFogVisual() and all let go of both. ⛔ Teardown does NOT reach that
// reconciler at all — so a match LEFT with fog still up (SessionMenuWidget's Back →
// USiegeSessionSubsystem::LeaveMatch() → an ABSOLUTE OpenLevel to L_MainMenu) or a PIE session
// STOPPED with fog up would strand r.VolumetricFog + both grid axes at SetByCode FOR THE REST OF
// THE PROCESS, with the owner of the captured pre-floor state already gone.
// ⛔ THE CONSEQUENCE IS THE EXACT STATE THIS DESIGN CHOSE Unset OVER A LITERAL RESTORE TO PREVENT,
// and it lands where the player can act: the graphics panel is reachable ONLY from L_MainMenu,
// which is precisely where that travel puts him. His Shadows slider would then be refused by the
// engine while the panel truthfully printed "ambient fog OFF" (it reads the GROUP LEVEL, not the
// cvar) ⇒ a menu that disagrees with the machine, i.e. SC-§94 pointed at the player.
// ⛔ AND IT WOULD BE INVISIBLE: no release runs, so no Error line exists; the ONLY symptom is a
// LogConsoleManager "lower priority" warning that the seam's own banner teaches readers to expect.
// ⇒ ⭐ THE PAIRING IS THE RULE, AT BOTH SITES: whatever outlives this actor is let go HERE too.

void AFogVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ⛔ CLEAR THE WAKE-UP FIRST. A timer bound to `this` fires into a dying actor otherwise, and
	// the ABarracks/ATower EndPlay precedent is to clear synchronously inside teardown rather than
	// to rely on the timer manager noticing.
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FogVisualExpiryTimerHandle);
	}

	// ⭐⭐⭐ LET THE INTEGRITY FLOOR GO — IN THE SAME ORDER AS THE FOG-IS-DOWN BRANCH (release,
	// then despawn), so the two exit sites read the same way. ⛔ THIS CALL CANNOT DOUBLE-RELEASE:
	// ReleaseFogRenderFloor() returns immediately unless IsFogRenderFloorEngaged(...) is true, and
	// its LAST statement is FogRenderFloorPriorState = ReleasedFogRenderFloorState() — so a fog
	// that already lifted through the reconciler makes this a no-op, and so does a match that
	// never saw fog at all. The idempotence is STRUCTURAL (one flag, set by the capture and tested
	// by the release), not a convention two call sites have to keep.
	// ⛔ It is also cheap enough not to need a condition: the guard is read BEFORE the height-fog
	// TActorIterator, so an unengaged teardown costs one bool.
	ReleaseFogRenderFloor();

	// ⛔ AND TAKE THE VISUAL WITH US. On a world teardown this is free — everything dies anyway —
	// but EndPlay also fires for Destroyed and LevelTransition, where the WORLD SURVIVES. There,
	// skipping this would leave a fully opaque box in the world with NOBODY HOLDING ITS
	// REFERENCE: fog no code could ever find again, i.e. permanent fog arriving through the back
	// door of a row whose entire subject is not shipping permanent fog.
	DestroyFogVisual();

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR

// ═════════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ THE DEV TRIGGERS — DETAILS-PANEL LANE (`TASK-1173`; law `SC-§113` cl. 3(d), 4)
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
//  ⛔ THESE ARE THE ⛔ WEAKEST AGENT LANE AND THE ⛔ ONLY 🧑 HUMAN ONE — said in that order because
//  it is the honest ordering. ⛔ No agent in this pipeline can press a Details-panel button; the
//  editor bridge has no click. ⇒ they exist because 🧑 he can press them ⛔ WITHOUT typing anything
//  and without a console, while a match is running, on the actor he can already see in the
//  outliner. That is independent value, ⛔ not a substitute for the console or the suite.
//
//  ⚠️⚠️ THE LIMITATION, ⛔ STATED SO NOBODY REPORTS IT AS A BUG: a button lives on an ⛔ INSTANCE.
//  Before the first cast of a match there ⛔ IS no `AFogVolume` to select, so these buttons can
//  ⛔ REFRESH and ⛔ CLEAR an existing fog-state actor but ⛔ CANNOT create one. ⇒ ⛔ to raise fog
//  from nothing, use `Siege.Fog.Raise` (which goes through `FindOrSpawn`) or play the card.
//
//  ⛔ `#if WITH_EDITOR` ON ⛔ BOTH THE DECLARATION AND THE DEFINITION (`SC-§111` cl. 3(c)). This is
//  the ⛔ PERMITTED case of that guard and not the forbidden one: what is guarded is ⛔ OUR OWN
//  developer affordance, ⛔ symmetrically, in a file whose ⛔ MECHANISM is guarded nowhere. ⛔ It is
//  ⛔ NOT an engine editor-only API called from a Runtime module — the defect `TASK-1166`
//  BLOCKER-1 caught in ⛔ this same file ⛔ this same week.
// ═════════════════════════════════════════════════════════════════════════════════════════════

void AFogVolume::DevRaiseFog()
{
	// ⛔ THE SAME DOOR THE CARD USES, on the actor already selected. ⛔ No spawn, no policy, no
	// arithmetic: `RaiseFog` owns the refusal, the refresh-never-stack rule and the reconciler.
	const bool bRaised = RaiseFog();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] DevRaiseFog (Details-panel button) called AFogVolume::RaiseFog(), which returned %s. Fog is now %s. ")
		TEXT("⛔ A FALSE is the BrightSun refusal (J-F19), not a broken button: nothing was written and no gold moved."),
		*GetNameSafe(this),
		bRaised ? TEXT("TRUE") : TEXT("FALSE"),
		IsFogActive() ? TEXT("UP") : TEXT("DOWN"));
}

void AFogVolume::DevClearFog()
{
	// ⛔ The match-reset door, ⛔ not the BrightSun card: both deadlines are zeroed and ⛔ NO
	// prevention window opens. Named `Clear` rather than `Reset` on the button so it reads as what
	// the operator sees happen, and the log line says which mechanic it really is.
	ResetFog();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] DevClearFog (Details-panel button) called AFogVolume::ResetFog(). Fog is now %s and prevention is now %s. ")
		TEXT("⛔ This is the match-reset door (FOG-§10.3) — it opens NO prevention window and is NOT the BrightSun card."),
		*GetNameSafe(this),
		IsFogActive() ? TEXT("UP") : TEXT("DOWN"),
		IsFogPrevented() ? TEXT("UP") : TEXT("DOWN"));
}

void AFogVolume::DevLogFogState()
{
	// ⛔ READ-ONLY. Writes nothing, creates nothing, and asks the shipped accessors rather than
	// recomputing — so it is safe to press before and after anything, which is what makes it a
	// before-picture rather than a summary (`SC-§107`).
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] DevLogFogState (Details-panel button) — fog %s, prevention %s (%.0f s of prevention remain), ")
		TEXT("visual actor %s. Read from IsFogActive / IsFogPrevented / GetFogPreventionSecondsRemaining."),
		*GetNameSafe(this),
		IsFogActive() ? TEXT("UP") : TEXT("DOWN"),
		IsFogPrevented() ? TEXT("UP") : TEXT("DOWN"),
		GetFogPreventionSecondsRemaining(),
		IsValid(FogVisualActor.Get()) ? TEXT("PRESENT") : TEXT("ABSENT"));
}

#endif // WITH_EDITOR

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

bool AFogVolume::FogVisualScaleMatches(const FVector& RequestedScale3D, const FVector& AchievedScale3D)
{
	// ⛔⛔⛔ THE WHOLE POINT OF THIS FUNCTION IS THAT A TEST CAN RUN IT (`SC-§79`). The comparison
	// it makes is trivial; the fact that it is REACHABLE FROM A TEST is not. Written inline at the
	// spawn site, this same expression would be a check nothing could ever exercise — and the
	// defect it guards was invisible for three of 🧑 Jonathan's reports precisely because the only
	// instrument that could have seen it was unreachable. ⇒ `Tests/SiegeFogVisualTest.cpp` hands
	// this the MEASURED vendor substitute and asserts it answers NO. ⛔ A detector nobody has ever
	// seen say NO is not a detector.

	// ⛔ NaN ON EITHER SIDE IS A MISMATCH, AND IT IS STATED RATHER THAN INHERITED. `FVector::Equals`
	// compares with `Abs(A - B) <= Tolerance`, and every comparison against a NaN is false — so a
	// NaN would already fall out as "does not match" without this branch. It is written anyway, in
	// the same `!(X)` form `BrightSunWindowSeconds` and `ASummonedUnit::HeightAdvantageMultiplier`
	// use, so the REASON is on the page instead of resting on an operator's behaviour a future
	// editor would have to know to preserve.
	if (RequestedScale3D.ContainsNaN() || AchievedScale3D.ContainsNaN())
	{
		return false;
	}

	// ⛔ ACHIEVED against REQUESTED, in that order, because that is the direction the caller cares
	// about: "is what the world gave me the thing I asked for?".
	return AchievedScale3D.Equals(RequestedScale3D, static_cast<double>(FogVisualScaleTolerance));
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

		// ⭐⭐⭐ AND THE INTEGRITY FLOOR IS LET GO ON THE SAME FACT (TASK-1147). ⛔ Unconditional
		// and idempotent, exactly like DestroyFogVisual() one line below: every exit calls it, and
		// it is a no-op when nothing was ever engaged. ⛔ A floor that is taken and never released
		// is a player's graphics settings silently overridden for the rest of his session — the
		// same "entered and not left" half-seam the visual's own despawn exists to prevent.
		ReleaseFogRenderFloor();

		DestroyFogVisual();
		return;
	}

	// ⭐⭐⭐ FOG IS UP ⇒ ITS PRESENCE STOPS BEING A GRAPHICS OPTION (TASK-1147, GFX-§12).
	// ⛔ BEFORE the spawn, not after: the box below renders through the volumetric path this call
	// floors, so flooring afterwards would leave the first frames of every fog looking exactly
	// like the bug — and 🧑 the first frames are the ones he watches for the card to take effect.
	// ⛔ Idempotent: RefreshFogVisual runs on every raise, refresh and wake-up, and the capture
	// inside cannot fire twice. See AFogVolume::CaptureFogRenderFloorPriorState.
	EnforceFogRenderFloor();

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

	// Derived ONCE and then referred to BY NAME three times — the set, the comparison and the log.
	// ⛔ Re-spelling `SpawnTransform.GetScale3D()` at each of those sites is how the log below came
	// to print the REQUEST in the first place, so the expression is spent here and never again.
	const FVector RequestedScale3D = SpawnTransform.GetScale3D();

	// ⛔⛔⛔ THE REPAIR, AND THIS ONE LINE IS WHY 🧑 JONATHAN COULD NOT SEE HIS FOG (TASK-1071).
	// The scale carried by the spawn transform above is ⛔ DISCARDED BY THE ENGINE before this
	// line runs: `BP_SiegeFog`'s root is the INHERITED SCS `Mesh` component rather than a native
	// root, so the non-deferred `SpawnActor` path reaches `SCS_Node.cpp:147` with
	// `bIsDefaultTransform == true` and substitutes the VENDOR component template's own
	// `RelativeScale3D`. ⇒ a world-sized fog box silently became a small slab parked thousands of
	// units above the battlefield, outside the volumetric froxel grid in every direction, and
	// rendered EXACTLY NOTHING for anyone, always.
	// ⛔⛔ DO NOT "SIMPLIFY" THIS TO `SpawnParams.TransformScaleMethod` — the substitution runs
	// AFTER that switch is consulted and overwrites the scale whichever method was chosen. The
	// artist checked it; the test file reds on that reach BY NAME.
	// ⛔⛔ AND DO NOT SWAP IT FOR `SpawnActorDeferred` + `FinishSpawning`, which also dodges the
	// clobber: the engine's own comment beside that line says `bIsDefaultTransform` is FALSE IN A
	// COOKED BUILD. ⇒ the deferred form behaves DIFFERENTLY in the editor and in the packaged
	// game, so this defect could return ONLY IN THE SHIPPED PRODUCT — where nobody is watching.
	// The explicit set is correct in BOTH, and that symmetry is the entire reason for the form.
	// ⛔ Scale only: the LOCATION survives the spawn intact (measured), and re-setting a transform
	// that arrived correctly would be inventing work whose failure nobody would notice.
	Spawned->SetActorScale3D(RequestedScale3D);

	// ⭐⭐⭐ NOW MEASURE WHAT WE GOT, ⛔ NEVER WHAT WE ASKED FOR — AND THIS IS THE MORE IMPORTANT
	// HALF OF THE ROW. The log below USED to print `SpawnTransform`, i.e. the REQUEST, and never
	// once read `GetActorScale3D()`. ⇒ every claim it made was true, the one number that mattered
	// was never taken, and a box the engine had shrunk by a factor of 32 read as a CLEAN SPAWN in
	// the log, passed the gate, and was reported to him as "spawned at the right transform".
	// ⛔⛔ AN INSTRUMENT THAT ECHOES THE REQUEST INSTEAD OF MEASURING THE RESULT IS NOT AN
	// INSTRUMENT — it is the failure wearing the evidence's clothes, and this project has now hit
	// that class of lie eleven times.
	// ⚠️ THE LOCATION IS READ BACK FOR THE SAME REASON, even though it is not the value that broke:
	// the old line asserted a Z it had likewise never looked at, and half a readback is the same
	// bug with better odds.
	const FVector AchievedScale3D = Spawned->GetActorScale3D();
	const FVector AchievedLocation = Spawned->GetActorLocation();

	// ⛔⛔ AND A DISAGREEMENT IS **LOUD**. Reaching this branch means the correction above did not
	// take, which is a fog box of the wrong size — i.e. 🧑 the same invisible fog, again, with the
	// mechanic still blinding both armies behind it.
	// ⚖️ Error level, ⛔ never a refusal: the fog MECHANIC keeps working with no visual and an art
	// failure may NEVER refuse a 50-gold card, consume-and-abort, or change one vision clamp —
	// the boundary the two loaders above already hold.
	if (!FogVisualScaleMatches(RequestedScale3D, AchievedScale3D))
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] ⛔⛔ THE FOG VISUAL IS AT THE WRONG SCALE — requested (%.3f, %.3f, %.3f), ACHIEVED (%.3f, %.3f, %.3f). ")
			TEXT("This is the TASK-1071 defect: the visual's root is an INHERITED SCS component, so the engine substitutes the ")
			TEXT("vendor component template's own scale for the one the spawn transform carries (SCS_Node.cpp, bIsDefaultTransform). ")
			TEXT("SpawnFogVisual corrects that with SetActorScale3D immediately after the spawn ⇒ SEEING THIS LINE MEANS THE ")
			TEXT("CORRECTION ITSELF FAILED, and the box on screen is the wrong size or nowhere to be seen. ")
			TEXT("The fog MECHANIC is unaffected and the card is NOT refused: every unit on both sides is still clamped to the ")
			TEXT("fog vision ceiling, so the field will LOOK clearer than the army can see. ")
			TEXT("Check that the visual's root component still accepts a scale change, and that nothing on the Blueprint's own ")
			TEXT("BeginPlay or Tick writes the scale back after this line."),
			*GetNameSafe(this),
			RequestedScale3D.X, RequestedScale3D.Y, RequestedScale3D.Z,
			AchievedScale3D.X, AchievedScale3D.Y, AchievedScale3D.Z);
	}

	// ⛔ HOLD THE REFERENCE. BP_SiegeFog's parent is the VENDOR BP_FogArea_C (read back LIVE under
	// TASK-1043), so it is NOT an AFogVolume subclass and TActorIterator<AFogVolume> will NEVER
	// see it: a Find-style sweep for it later returns NOTHING, SILENTLY. There is no re-finding
	// this actor — if the handle is lost, the box is unreachable forever.
	FogVisualActor = Spawned;

	// ⛔⛔ ACHIEVED VALUES ONLY. The parenthesised request is kept beside them so a human reading
	// one line can see a disagreement even if the predicate's tolerance were itself ever wrong —
	// but the FIRST numbers on the line are the ones read back off the actor, because those are
	// the ones that describe what is actually in the world.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Fog VISUAL spawned: '%s' — ACHIEVED Z=%.0f, ACHIEVED scale (%.0f, %.0f, %.0f), read back from the actor ")
		TEXT("with GetActorLocation/GetActorScale3D (requested (%.0f, %.0f, %.0f), derived from ArenaHalfExtent + a %.0f uu margin; ")
		TEXT("TASK-841 §5.2: sized to the arena exactly, the CORNER rendered clear). ")
		TEXT("⛔ These are the MEASURED values, never the spawn transform — echoing the request is what let an engine-substituted ")
		TEXT("scale read as a clean spawn for three of his reports (TASK-1071 §4)."),
		*GetNameSafe(this), *GetNameSafe(Spawned),
		AchievedLocation.Z,
		AchievedScale3D.X, AchievedScale3D.Y, AchievedScale3D.Z,
		RequestedScale3D.X, RequestedScale3D.Y, RequestedScale3D.Z,
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

// ═══ ⭐⭐⭐ THE INTEGRITY FLOOR (TASK-1147; GFX-§12, FOG-§12.2, FOG-§12.5) ═════════════════════
//
// 🧑 HIS REPORT, AND IT IS THE WHOLE SUBJECT: "players could get an advantage by turning their
// graphics down because now they will be able to see through a fog that they weren't meant to."
//
// ⛔⛔ THE ASYMMETRY, MEASURED (TASK-1146 §1) AND WHY IT IS AN EXPLOIT RATHER THAN A TRADEOFF:
// the fog's MECHANICAL penalty reduces to a world-clock comparison plus a default-constructed
// tuning struct — NO cvar, NO UGameUserSettings, NO viewer, team or controller touches either
// input — so it applies IDENTICALLY at every graphics setting. The VISUAL is deleted outright by
// r.VolumetricFog=0 at [ShadowQuality@0]/@1. ⇒ at Shadows=Low the player SEES through the fog AND
// KEEPS HIS OPPONENT BLIND, for free. A symmetric loss would be a tradeoff; this is one-sided in
// BOTH directions at once, which is GFX-§12's mechanical definition of a cheat with a performance
// alibi.
//
// ⭐⭐ THE SEAM, AND WHY THIS ONE: a cvar written at ECVF_SetByCode OUTRANKS ECVF_SetByScalability,
// and the engine's own CanChange refuses any later lower-priority write. ⇒ "the floor survives a
// settings change" costs NO delegate, NO poll and NO tick — the state actor still never ticks.
// ⛔ THE THREE FORBIDDEN SHORTCUTS ARE NOT TAKEN AND NOTHING HERE NEEDS THEM (FOG-§12.2):
// ⛔ (i) no map save — the height-fog write is IN MEMORY on the running world's own instance, and
//        nothing here marks a package dirty or calls a save;
// ⛔ (ii) no Config/DefaultEngine.ini write — scalability would overwrite an ini line at every
//        quality change and the setting would silently lose, which is an SC-§94-class lie;
// ⛔ (iii) no clamping of the Shadows group — that would charge the cheap-PC player GFX-§8 was
//        written for across the WHOLE match to protect a five-minute spell.
//
// ⛔⛔ AND THE FOG MECHANIC IS NOT TOUCHED BY ONE BYTE. SiegeFogStatics is unchanged, the vision
// ceiling and onset are unchanged, GatherHostileAgents is unchanged. This block makes the VISUAL
// match the MECHANIC; it does not retune either.
//
// 🚨⛔⛔⛔ WHICH FOG THIS FLOORS — READ BEFORE ATTRIBUTING ANY OUTCOME TO IT.
// TASK-1151 measured, ON RENDERED PIXELS, that TWO DIFFERENT FOGS were being discussed as one:
//   • the AMBIENT ExponentialHeightFog + froxel volumetric system — what r.VolumetricFog and
//     bEnableVolumetricFog govern, and what THIS block floors;
//   • BP_SiegeFog, the Fog card's own visual — a RAYMARCHED TRANSLUCENT MESH computing its
//     density inside MF_Fog (5 WorldPosition nodes, CameraPositionWS, 2 Time, a volume-texture
//     sample). It is NOT IN THE FROXEL GRID AT ALL, and neither material sets
//     bUsedWithVolumetricFog.
// ⛔ THE EVIDENCE: in the editor world — height fog live, BP_SiegeFog absent by construction —
// the enemy castle is CRISP AT ~50,000 uu with no wash at any depth. ⇒ the system this block
// floors renders ≈ NOTHING at the player's vantage.
// ~~⇒ ⚖️ THIS FLOOR IS CORRECT FOR THE AMBIENT FOG AND DOES NOT REACH THE CARD'S FOG. It is not
// the fix for 🧑 his report, and it must never be described as one — that would be exactly the
// SC-§94 shape this project has already shipped twice: every read-back reporting success while
// nothing the player can see has changed.~~
// 🚨⛔⛔⛔ REVERSED 2026-09-08 (TASK-1162), MEASURER TASK-1160. STRUCK IN PLACE, NEVER DELETED —
// TASK-1160's handoff §4 quotes the struck sentence BY NAME as the record it refutes.
// ⛔ THE MEASUREMENT: with a Fog card's BP_SiegeFog up and Shadows at LOW the card's fog DOES NOT
// RENDER — the frame IS the no-fog frame to within 0.2 % (ground visibility ≤464 uu → no collapse
// anywhere, castle legible at ~43,000 uu, a ≈93× lower bound). ⭐ AND A 2×2 ISOLATES THE
// MECHANISM TO ONE CVAR: EPIC + forced r.VolumetricFog 0 ⇒ fog GONE; LOW + forced
// r.VolumetricFog 1 ⇒ fog BACK ⇒ the effect tracks r.VolumetricFog and is INDEPENDENT of
// sg.ShadowQuality. Row D of that 2×2 is a hand-simulation of THIS FLOOR.
// ⇒ ⚖️ THIS FLOOR REACHES BOTH FOGS AND IT IS THE FIX FOR 🧑 HIS REPORT — the one cvar it pins
// governs the ambient froxel term AND the card's raymarched mesh. It is not merely defensive
// depth over a different fog.
// ⚠️ AND THE HONEST BOUND, SO THE REPAIR DOES NOT START A NEW UNMEASURED CLAIM (SC-§97): THE
// MECHANISM IS ISOLATED TO THE CVAR; THE COUPLING HAS NOT YET BEEN READ AT THE MATERIAL. The
// "neither material sets bUsedWithVolumetricFog" line above is a MEASURED NODE-CENSUS FACT and
// is now INTERESTING rather than dispositive — ⛔ it is a candidate nobody has opened, so do not
// write "because bUsedWithVolumetricFog" anywhere.
// ⛔ WHAT DOES NOT CHANGE: ask (A) still DOES NOT CLOSE when this ships (r.SceneColorFormat and
// r.TranslucencyLightingVolume individually, Shadows=MEDIUM, the shipped MENU path, a PACKAGED
// build, FRAME TIME, and the material-level WHY are all still untested) — 🧑 he closes it on
// TASK-1159, not this file.
// ⚠️ AND THE ROUTES THAT WOULD HAVE MADE IT THE FIX WERE SEARCHED FOR AND NOT FOUND (a
// positive-controlled name-table scan of all 27 vendor packages plus BP_SiegeFog): ZERO
// DetailMode overrides — so r.DetailMode=0 at [EffectsQuality@0] cannot strip the mesh, because
// ShouldComponentAddToScene is `DetailMode <= r.DetailMode` (SceneComponent.cpp:3552) and the
// default is DM_Low; ZERO MaterialExpressionQualitySwitch/FeatureLevelSwitch anywhere in the
// pack — so r.MaterialQualityLevel=0 selects no cheaper raymarch; ZERO draw-distance overrides.
// ~~⇒ NO MEASURED ROUTE by which a graphics setting deletes the card's fog. ⛔ That makes the
// EXISTENCE of the reported exploit an OPEN QUESTION rather than a settled fact — see
// handoffs/TASK-1147-programmer.md §0 for the decisive measurement, which needs the editor.~~
// 🚨⛔⛔ SUPERSEDED 2026-09-08 (TASK-1162), MEASURER TASK-1160 — the decisive measurement WAS
// TAKEN, in the editor, and the exploit is DEMONSTRATED rather than open. ⭐ THE SCAN ABOVE IS
// NOT WITHDRAWN AND IT WAS NOT WRONG: it enumerated DetailMode / QualitySwitch / draw-distance
// routes and correctly found NONE. THE ROUTE WAS A FOURTH KIND NOBODY ENUMERATED — r.VolumetricFog
// itself. ⇒ ⚖️ AN EXHAUSTIVE SEARCH OF AN INCOMPLETE LIST IS STILL AN INCOMPLETE SEARCH, AND ITS
// RIGOUR IS EXACTLY WHAT MAKES IT READ AS EXHAUSTIVE.
//
// ⚠️ SCOPE DELETION, MEASURED RATHER THAN ASSUMED (TASK-1146 §3.0), AND IT IS RECORDED HERE
// BECAUSE THE DAY IT STOPS BEING TRUE IS THE DAY SOMEBODY MUST RE-READ THIS: the graphics panel
// is reachable ONLY from WBP_MainMenu on /Game/Maps/L_MainMenu — a DIFFERENT MAP from L_Arena —
// and USettingsMenuWidget::CreateAndAddToViewport has ZERO call sites. ⇒ THE PLAYER CANNOT OPEN
// THE GRAPHICS MENU DURING A MATCH, so nothing here is built for a mid-match settings change.
// ⚠️ THOSE TWO FACTS ARE THE LOAD-BEARING ONES, AND THE ABSOLUTE SENTENCE THIS COMMENT USED TO
// CARRY WAS NOT EXACT (TASK-1148 NIT-2): there IS an in-match menu — USessionMenuWidget — it just
// holds no Settings/Graphics entry. ⛔ It is worth naming rather than hand-waving, because its
// Back button (SessionMenuWidget.cpp:123 → USiegeSessionSubsystem::LeaveMatch()) is the ABSOLUTE
// travel that made teardown a route this seam had to cover at all. See EndPlay.
// ⛔ THE DAY THIS CHANGES is the day that menu — or a pause/escape menu — ships a Settings entry.
// ⭐ The priority argument above already covers that case for free, so the deletion costs nothing
// if it is ever reversed — but a reader arriving after such a menu ships should confirm it, not
// assume. ⚠️ It is also the day the Shadows hint's first clause stops being true at every instant
// the player can read it (SiegeGraphicsMenuWidget.cpp, TASK-1148 WARN-6).

// ⛔⛔ THE ONE PLACE EACH OF THESE NAMES IS WRITTEN IN THE PROJECT — the
// `USiegeSettingsSubsystem::SettingsSlotName` house pattern, and the same split `FogVisualClassPath`
// already uses for the asset path. ⛔ Do NOT re-type one at a call site: `Tests/SiegeFogVisualTest.cpp`
// asserts that neither half of the pair contains a `r.VolumetricFog` literal of its own, so a
// hand-typed name goes RED here rather than silently flooring a variable that does not exist.
// ⛔ Their VALUES are the engine's, not ours: `BaseScalability.ini` writes the first at
// `[ShadowQuality@0]:143` / `@1:178` (0) and all three at `@2:213-215` (1 / 16 / 64).
const TCHAR* AFogVolume::FogRenderFloorCVarVolumetricFog = TEXT("r.VolumetricFog");
const TCHAR* AFogVolume::FogRenderFloorCVarGridPixelSize = TEXT("r.VolumetricFog.GridPixelSize");
const TCHAR* AFogVolume::FogRenderFloorCVarGridSizeZ = TEXT("r.VolumetricFog.GridSizeZ");

UExponentialHeightFogComponent* AFogVolume::FindHeightFogComponent(const UWorld* World)
{
	if (!World)
	{
		return nullptr; // no world (CDO/test context) — never a crash, and never a floor
	}

	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		const AExponentialHeightFog* const HeightFog = *It;
		if (IsValid(HeightFog))
		{
			// ⛔ FIRST VALID HIT, deliberately — the renderer reads Scene->ExponentialFogs[0] and
			// nothing else, so a level with two height fogs already has an ambiguity this lookup
			// cannot resolve and must not pretend to.
			if (UExponentialHeightFogComponent* const Component = HeightFog->GetComponent())
			{
				return Component;
			}
		}
	}

	return nullptr;
}

FFogRenderFloorPriorState AFogVolume::CaptureFogRenderFloorPriorState(const FFogRenderFloorPriorState& Prior, const FFogRenderFloorObservation& Observed)
{
	// ⭐⭐⭐ THE RE-ENTRANCE GUARD, AND IT IS THE ENTIRE FUNCTION.
	// ⛔⛔ THE DEFECT: the one reconciler runs on EVERY raise, EVERY refresh and EVERY timer
	// wake-up. A second capture would read the ALREADY-FLOORED machine and record THE FLOOR as
	// the player's own choice ⇒ the release would then "restore" the floor, i.e. become a
	// PERMANENT NO-OP, silently UPGRADING a Low-settings player's shadows for the rest of his
	// session — a graphics change nobody asked for, invisible in every log.
	// ⛔ Returning Prior UNCHANGED is what makes "capture twice" inexpressible rather than merely
	// discouraged, and it is the one line the row's named mutation deletes.
	if (IsFogRenderFloorEngaged(Prior))
	{
		return Prior;
	}

	FFogRenderFloorPriorState Captured;
	Captured.bEngaged = true;
	Captured.Observed = Observed;
	return Captured;
}

bool AFogVolume::IsFogRenderFloorEngaged(const FFogRenderFloorPriorState& State)
{
	// ⛔ THE ONE SPELLING of "the floor is currently held". Both halves of the pair ask THIS,
	// so they can never disagree — the same discipline that makes IsFogActive() the one predicate
	// the reconciler branches on. ⛔ Do not inline `.bEngaged` at a call site.
	return State.bEngaged;
}

FFogRenderFloorPriorState AFogVolume::ReleasedFogRenderFloorState()
{
	// ⛔ A default-constructed state IS the released one, and saying so through a named function
	// rather than an inline `= {}` is what lets a test assert it. ⭐ The assertion is not a
	// tautology: a release that forgot this line would leave the guard UP, and the NEXT match's
	// enforce would hit it and never floor at all — the exploit back, silently, one match later.
	return FFogRenderFloorPriorState();
}

void AFogVolume::EnforceFogRenderFloor()
{
	// ⛔ THE SWITCH GATES ENGAGING ONLY. ReleaseFogRenderFloor deliberately does NOT read it — see
	// the tunable's own declaration for why an honoured switch there would strand the console
	// variables at code priority forever.
	if (!bEnforceFogRenderFloor)
	{
		return;
	}

	IConsoleManager& Console = IConsoleManager::Get();
	IConsoleVariable* const VolumetricFogCVar = Console.FindConsoleVariable(FogRenderFloorCVarVolumetricFog);
	IConsoleVariable* const GridPixelSizeCVar = Console.FindConsoleVariable(FogRenderFloorCVarGridPixelSize);
	IConsoleVariable* const GridSizeZCVar = Console.FindConsoleVariable(FogRenderFloorCVarGridSizeZ);

	if (!VolumetricFogCVar || !GridPixelSizeCVar || !GridSizeZCVar)
	{
		// ⛔ LOUD, AND NOTHING IS CAPTURED. Capturing a partial observation here would let the
		// release "restore" values it never read. ⛔ The card is NOT refused and no clamp moves:
		// the fog mechanic keeps working, which is exactly the failure this row is about, so the
		// line says so in as many words rather than reporting a rendering hiccup.
		// 🚨⛔ REVERSED 2026-09-08 (TASK-1163), MEASURER TASK-1160. STRUCK LITERAL, KEPT VERBATIM
		// IN THIS COMMENT AND OUT OF THE LOG LINE (an operator cannot read a strike-through):
		//   ~~"⚠️ This does NOT govern the Fog card's own visual (BP_SiegeFog is a raymarched mesh
		//     outside the froxel grid, TASK-1151) — do not read this line as 'the fog is gone'."~~
		// ⛔ Same shape as the failure branch below, same repair: the variable this block could not
		// find is the one TASK-1160 measured driving BOTH fogs, so a build without it has no floor
		// over EITHER, and "the fog is gone" is a reading this line WARRANTS rather than rules out.
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] ⛔⛔ THE FOG INTEGRITY FLOOR COULD NOT BE ENGAGED — one of the volumetric-fog console variables ")
			TEXT("does not exist in this build ('%s' %s, '%s' %s, '%s' %s). ")
			TEXT("⛔ The AMBIENT volumetric fog's presence therefore stays a graphics option while the siege fog is up, ")
			TEXT("so a player at Shadows=Low or Medium loses a layer of concealment his opponent still pays for. ")
			TEXT("🚨 AND DO NOT READ THIS LINE AS 'only the ambient fog is exposed': the console variable this build ")
			TEXT("is missing is the one TASK-1160 measured driving the Fog card's own visual, BP_SiegeFog, as well — ")
			TEXT("with no floor engaged, 'the fog is gone' is a reading this line WARRANTS, not one it rules out ")
			TEXT("(reversed by TASK-1163). ")
			TEXT("Nothing was captured and nothing was written, so the release will correctly do nothing."),
			*GetNameSafe(this),
			FogRenderFloorCVarVolumetricFog, VolumetricFogCVar ? TEXT("found") : TEXT("MISSING"),
			FogRenderFloorCVarGridPixelSize, GridPixelSizeCVar ? TEXT("found") : TEXT("MISSING"),
			FogRenderFloorCVarGridSizeZ, GridSizeZCVar ? TEXT("found") : TEXT("MISSING"));
		return;
	}

	UExponentialHeightFogComponent* const HeightFog = FindHeightFogComponent(GetWorld());

	// ⛔ READ BEFORE ANY WRITE. This is the observation the release will restore from, and taking
	// it after even one write is the whole bug this row's guard exists to prevent.
	// ⚠️ COST, STATED EXACTLY (TASK-1148 NIT-3): the OBSERVATION above — one TActorIterator over
	// AExponentialHeightFog plus three FindConsoleVariable — runs on EVERY call, including every
	// re-entry, because the guard sits BELOW it on purpose. A short-circuit placed above it would
	// make the guard dead code, which is the SC-§36.1 shape this file exists to refuse. ⇒ "once
	// per fog window" bounds the WRITES and the LOG LINE, ⛔ not this read. On a non-ticking actor
	// that is a handful of iterations per match, and the trade is deliberate.
	FFogRenderFloorObservation Observed;
	Observed.VolumetricFogEnabled = VolumetricFogCVar->GetInt();
	Observed.GridPixelSize = GridPixelSizeCVar->GetInt();
	Observed.GridSizeZ = GridSizeZCVar->GetInt();
	Observed.bHeightFogFound = (HeightFog != nullptr);
	Observed.bHeightFogVolumetricEnabled = (HeightFog != nullptr) && HeightFog->bEnableVolumetricFog;

	// ⭐⭐⭐ THE TRANSITION, THROUGH THE ONE PURE FUNCTION A TEST CAN EXECUTE. On a re-entry it
	// returns the PREVIOUS state untouched, so the assignment below is a self-assignment and the
	// player's original values survive. ⛔ Delete that guard and this line records the floor.
	const FFogRenderFloorPriorState Previous = FogRenderFloorPriorState;
	FogRenderFloorPriorState = CaptureFogRenderFloorPriorState(Previous, Observed);

	// ⛔ THE WRITES HAPPEN ON THE TRANSITION ONLY. Re-writing an already-floored value would be
	// harmless to the machine and NOISY in the log — and a per-refresh log line is how a real
	// disagreement stops being noticed.
	if (IsFogRenderFloorEngaged(Previous))
	{
		return;
	}

	// ⭐ ECVF_SetByCode outranks ECVF_SetByScalability, so this survives every later quality apply
	// without a delegate. ⚠️ EXPECTED AND NOT A DEFECT: while the floor is held, each scalability
	// apply logs one `LogConsoleManager: Warning ... was ignored as it is lower priority` per
	// floored variable. That warning IS the floor working; it stops the moment the fog lifts.
	//
	// ⛔⛔ AND THE OTHER HALF OF THAT DISCLOSURE, WHICH THE FIRST DRAFT LEFT OUT (TASK-1148 WARN-3
	// — the omission cost a blocker): the warning is expected ⛔ ONLY WHILE A FOG WINDOW IS UP.
	// The SAME warning at the MAIN MENU means a release did not run, i.e. the floor is stranded
	// for the process and the player's Shadows slider is being refused on the one screen he can
	// reach it. ⇒ ⛔ WHERE it appears is the whole reading. Do not dismiss it by category.
	// ⭐ Since TASK-1147 loop 1 there is no known path to that state — EndPlay releases too — so a
	// main-menu occurrence is a REAL FINDING and the release's Error line should be beside it.
	//
	// ⚠️⚠️ AND IF YOU ARE HERE TO MEASURE WHETHER THE EXPLOIT EXISTS, READ THIS FIRST (WARN-2):
	// this line DEFEATS THAT MEASUREMENT. With the floor held, `sg.ShadowQuality 0` can no longer
	// drive the switch to 0, so a capture taken while a fog window is up shows the wash surviving
	// ⛔ WHETHER OR NOT the exploit exists — a false "no exploit" that looks like evidence. ⇒ the
	// floor must be defeated ⛔ DELIBERATELY: type `r.VolumetricFog 0` in the console
	// (ECVF_SetByConsole = 0x10000000 OUTRANKS ECVF_SetByCode = 0x0E000000), or run with
	// bEnforceFogRenderFloor = false on the CDO — ⛔ and say in the handoff WHICH one you did.
	VolumetricFogCVar->Set(FogRenderFloorVolumetricFogOn, ECVF_SetByCode);

	// ⛔ THE GRID GOES WITH THE SWITCH, AND IT IS A COST FIX RATHER THAN A RENDER ONE — see
	// FogRenderFloorGridPixelSize's declaration. Without these two lines an Epic→Low player keeps
	// the EPIC froxel grid, so the floor would make the CHEAPEST setting run the MOST EXPENSIVE
	// fog: correct picture, wrong bill, and precisely the inversion GFX-§12 forbids.
	GridPixelSizeCVar->Set(FogRenderFloorGridPixelSize, ECVF_SetByCode);
	GridSizeZCVar->Set(FogRenderFloorGridSizeZ, ECVF_SetByCode);

	// ⛔ TERM 6 OF THE SIX. The cvar is necessary and NOT sufficient; this reaches the map actor's
	// own switch, IN MEMORY. ⛔ SetVolumetricFog is a no-op when the value already matches, so on
	// today's L_Arena (authored true) this costs nothing and marks no render state dirty.
	if (HeightFog)
	{
		HeightFog->SetVolumetricFog(true);
	}

	// ⭐⭐⭐ NOW MEASURE WHAT WE GOT, ⛔ NEVER WHAT WE ASKED FOR (SC-§94 cl. B). SpawnFogVisual's
	// read-back is the house pattern ten screens up and this is a clone of it. ⛔ A log that
	// echoed the request would report a floor that a higher-priority console write had defeated.
	// ⚠️ DATE STAMP for the "view distance" line in the Error below (TASK-1148 NIT-4): L_Arena's
	// VolumetricFogDistance was re-measured LIVE at 6000 uu on 2026-09-08 (TASK-1151), so the
	// transcription this text calls stale is CURRENTLY ACCURATE and that falsifier has NOT fired.
	// ⛔ The next reader should re-measure rather than re-derive it from this comment.
	const int32 AchievedVolumetricFog = VolumetricFogCVar->GetInt();
	const int32 AchievedGridPixelSize = GridPixelSizeCVar->GetInt();
	const int32 AchievedGridSizeZ = GridSizeZCVar->GetInt();
	const bool bAchievedHeightFogVolumetric = (HeightFog != nullptr) && HeightFog->bEnableVolumetricFog;
	const float AchievedHeightFogDistance = (HeightFog != nullptr) ? HeightFog->VolumetricFogDistance : 0.f;

	// ⛔⛔ THE PRESENCE HALF. Any of these means the fog will NOT render while the clamp is still
	// blinding the army — i.e. the exact bug, still live, after a fix that logged success.
	if (AchievedVolumetricFog != FogRenderFloorVolumetricFogOn || !HeightFog || !bAchievedHeightFogVolumetric || AchievedHeightFogDistance <= 0.f)
	{
		// 🚨⛔⛔⛔ REVERSED 2026-09-08 (TASK-1163), MEASURER TASK-1160. THE STRUCK LITERAL, KEPT
		// VERBATIM HERE BECAUSE IT MUST NOT BE SILENTLY DELETED — but kept in a COMMENT, never in
		// the log line, because an operator reading a strike-through inside an Error string cannot
		// tell which half of it is live:
		//   ~~"⚠️ It does NOT mean the battlefield is clear: the Fog card's own visual is
		//     BP_SiegeFog, a raymarched mesh this floor does not reach (TASK-1151).
		//     ⛔ Report this as a FLOOR failure, never as 'no fog'."~~
		// ⛔ THAT WAS NOT MERELY A FALSE STATEMENT, IT WAS A FALSE INSTRUCTION, AND IT WAS EMITTED
		// ON THE ONE BRANCH WHERE IT DOES THE MOST DAMAGE: this block fires precisely when the
		// floor did NOT take, i.e. when r.VolumetricFog is UNPINNED and the exploit is LIVE.
		// TASK-1160 photographed the Fog card's own wash vanishing at Shadows=Low (the frame is the
		// no-fog frame to within a fifth of a percent) and its 2×2 isolated the mechanism to that
		// ONE console variable ⇒ "no fog" is EXACTLY what this line may mean, and the struck text
		// told the operator to rule it out at the moment of detection.
		// ⚠️ HONEST BOUND, CARRIED WITH THE REVERSAL: the mechanism is isolated to the cvar; the
		// coupling has NOT been read at the material. Nothing here says why, and ask (A) does not
		// close — seven conditions remain untested (FOG-§12.1, second correction).
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] ⛔⛔ THE FOG INTEGRITY FLOOR DID NOT TAKE — READ BACK from the machine, not from the request: ")
			TEXT("'%s' ACHIEVED %d (floor asks %d, set at SetByCode; the variable now reports SetBy%s), height-fog actor %s, ")
			TEXT("its volumetric switch ACHIEVED %s, its view distance ACHIEVED %.0f. ")
			TEXT("⛔ ShouldRenderVolumetricFog is a SIX-TERM conjunction — the console variable is ONE of them — so any ")
			TEXT("single line above reading wrong means NO AMBIENT VOLUMETRIC FOG RENDERS, i.e. the floor did not take. ")
			TEXT("🚨 AND IT MAY MEAN EXACTLY THAT THE BATTLEFIELD IS CLEAR: the Fog card's own visual, BP_SiegeFog, is ")
			TEXT("driven by this SAME console variable — measured on pixels (TASK-1160) — so an unpinned variable at ")
			TEXT("Shadows=Low deletes the CARD'S wash too. ⛔ Report this as a FLOOR failure AND as a live 'no fog' ")
			TEXT("exploit: LOOK AT THE CARD'S WASH before you rule 'no fog' out (reversed by TASK-1163). ")
			TEXT("A missing height-fog actor means the level has none (terms 5-7 unsatisfiable); a zero view distance ")
			TEXT("means L_Arena's ExponentialHeightFog was retuned and AFogVolume::FogVisualHorizontalMarginUU's ")
			TEXT("transcription of it is now stale too; a console override outranks SetByCode and cannot be fixed here."),
			*GetNameSafe(this),
			FogRenderFloorCVarVolumetricFog, AchievedVolumetricFog, FogRenderFloorVolumetricFogOn,
			GetConsoleVariableSetByName(static_cast<EConsoleVariableFlags>(VolumetricFogCVar->GetFlags() & ECVF_SetByMask)),
			HeightFog ? TEXT("FOUND") : TEXT("⛔ NOT FOUND"),
			bAchievedHeightFogVolumetric ? TEXT("ON") : TEXT("⛔ OFF"),
			AchievedHeightFogDistance);
	}

	// ⚠️ THE COST HALF, SEPARATED ON PURPOSE: a grid that did not take is a PERFORMANCE finding,
	// not an integrity one — the fog still renders, it just renders at whatever resolution the
	// player's last high setting left behind. ⛔ Warning, never Error: conflating the two would
	// teach the next reader that an Error here might be harmless.
	if (AchievedGridPixelSize != FogRenderFloorGridPixelSize || AchievedGridSizeZ != FogRenderFloorGridSizeZ)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] ⚠️ The fog integrity floor's FROXEL GRID did not take — READ BACK: '%s' ACHIEVED %d (floor asks %d), ")
			TEXT("'%s' ACHIEVED %d (floor asks %d). The fog still RENDERS (that is the switch, and it read back correctly); ")
			TEXT("what is wrong is the BILL. [ShadowQuality@0]/@1 never reset the grid, so a player who dropped from Epic ")
			TEXT("keeps the EPIC grid ⇒ the cheapest setting would run the most expensive fog."),
			*GetNameSafe(this),
			FogRenderFloorCVarGridPixelSize, AchievedGridPixelSize, FogRenderFloorGridPixelSize,
			FogRenderFloorCVarGridSizeZ, AchievedGridSizeZ, FogRenderFloorGridSizeZ);
	}

	// ⭐ ACHIEVED FIRST, the player's captured values in parentheses beside them — the same shape
	// the visual's spawn line uses, for the same reason: a human reading one line can see the
	// disagreement even if a predicate's tolerance were itself wrong.
	// ⛔ This fires ONCE per fog window, not once per refresh: the transition guard above is what
	// bounds it, so a floor that started logging every wake-up would mean the guard had failed.
	// 🚨⛔ REVERSED 2026-09-08 (TASK-1163), MEASURER TASK-1160. STRUCK LITERAL, KEPT VERBATIM HERE:
	//   ~~"⛔ SCOPE: this is the AMBIENT volumetric fog. The Fog card's own visual is BP_SiegeFog,
	//     a raymarched mesh outside the froxel grid, and this line says NOTHING about it
	//     (TASK-1151)."~~
	// ⛔ This one was false in the OTHER direction — it DISCLAIMED the very effect TASK-1160 later
	// measured, i.e. the floor's own value. Least dangerous of the three, still false, still
	// shipped. ⚠️ Same honest bound as above: mechanism isolated to the cvar, coupling not read at
	// the material, ask (A) does not close.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Fog INTEGRITY FLOOR engaged — ACHIEVED '%s'=%d, '%s'=%d, '%s'=%d, height fog volumetric=%s (view distance %.0f), ")
		TEXT("all read back off the machine after the write. The player's own pre-floor values were %d / %d / %d and volumetric=%s, ")
		TEXT("and those are what the release restores — the switch by Unset(SetByCode), so his LIVE slider choice wins even if he ")
		TEXT("changed it while the fog was up. ⭐ SCOPE: this covers BOTH fogs. The same console variable also drives ")
		TEXT("the Fog card's own visual, BP_SiegeFog — measured on pixels (TASK-1160) — so this line is ALSO the record ")
		TEXT("that the card's wash now survives Shadows=Low (reversed by TASK-1163)."),
		*GetNameSafe(this),
		FogRenderFloorCVarVolumetricFog, AchievedVolumetricFog,
		FogRenderFloorCVarGridPixelSize, AchievedGridPixelSize,
		FogRenderFloorCVarGridSizeZ, AchievedGridSizeZ,
		bAchievedHeightFogVolumetric ? TEXT("ON") : TEXT("OFF"), AchievedHeightFogDistance,
		Observed.VolumetricFogEnabled, Observed.GridPixelSize, Observed.GridSizeZ,
		Observed.bHeightFogVolumetricEnabled ? TEXT("ON") : TEXT("OFF"));
}

void AFogVolume::ReleaseFogRenderFloor()
{
	// ⛔⛔ NOT GATED ON bEnforceFogRenderFloor, AND THAT IS DELIBERATE — a release must always be
	// able to let go of something an earlier enforce took. Honouring the switch here would mean
	// that flipping it off mid-window STRANDS the console variables at code priority forever,
	// killing the Shadows slider's effect on fog for the rest of the session, silently.
	if (!IsFogRenderFloorEngaged(FogRenderFloorPriorState))
	{
		// Nothing was ever engaged. ⛔ Idempotent on purpose: every exit calls this
		// unconditionally, exactly like DestroyFogVisual, and a match that never saw fog must be
		// a no-op here.
		return;
	}

	const FFogRenderFloorObservation Prior = FogRenderFloorPriorState.Observed;

	// ⛔ THE COMPONENT RESTORES A CAPTURED VALUE because a map-actor property has NO priority
	// stack to fall back through — it is the one half of this pair that cannot use Unset.
	// ⛔ bHeightFogFound is what stops a component we never observed from being written: without
	// it the default-constructed `false` would look like a genuine reading and this line would
	// TURN OFF a fog nobody had turned on.
	if (Prior.bHeightFogFound)
	{
		// ⛔ AND WHY THIS IS SAFE ON THE TEARDOWN PATH, WRITTEN HERE RATHER THAN ONLY IN A HANDOFF
		// (TASK-1148 NIT-5): reached from EndPlay, the AExponentialHeightFog may ALREADY have run its
		// own EndPlay. TActorIterator skips pending-kill actors, so the lookup below simply returns
		// nullptr, and SetVolumetricFog on an unregistered component is harmless ⇒ the worst case is a
		// SKIPPED restore of a component whose world is dying, which costs nothing. ⭐ On today's
		// L_Arena it is a no-op either way (authored `true` == captured `true`).
		if (UExponentialHeightFogComponent* const HeightFog = FindHeightFogComponent(GetWorld()))
		{
			HeightFog->SetVolumetricFog(Prior.bHeightFogVolumetricEnabled);
		}
	}

	// ⭐⭐⭐ UNSET, NOT A CAPTURED-LITERAL WRITE-BACK. Unset removes ONLY our code layer and the
	// variable falls back through its own priority history to whatever SCALABILITY last wrote —
	// i.e. the player's LIVE choice. ⛔ A Set(prior, ECVF_SetByCode) restore would leave the
	// variable PINNED AT CODE PRIORITY FOREVER AFTER, so every later scalability apply would be
	// silently refused and his Shadows slider would stop affecting fog for the rest of the
	// session, with nothing in any log. ⛔ That trades one integrity bug for another.
	IConsoleManager& Console = IConsoleManager::Get();
	IConsoleVariable* const VolumetricFogCVar = Console.FindConsoleVariable(FogRenderFloorCVarVolumetricFog);
	IConsoleVariable* const GridPixelSizeCVar = Console.FindConsoleVariable(FogRenderFloorCVarGridPixelSize);
	IConsoleVariable* const GridSizeZCVar = Console.FindConsoleVariable(FogRenderFloorCVarGridSizeZ);

	if (VolumetricFogCVar)
	{
		VolumetricFogCVar->Unset(ECVF_SetByCode);
	}
	if (GridPixelSizeCVar)
	{
		GridPixelSizeCVar->Unset(ECVF_SetByCode);
	}
	if (GridSizeZCVar)
	{
		GridSizeZCVar->Unset(ECVF_SetByCode);
	}

	// ⭐⭐⭐ AND READ BACK WHAT THE RELEASE ACHIEVED (SC-§94 cl. B). ⛔ There is deliberately NO
	// expected VALUE to compare against here — the whole point is that the value is now the
	// player's, whatever it is. ⛔ What IS checkable is that OUR LAYER IS GONE: the variable must
	// no longer report SetByCode. A release that silently left the pin in place is the exact
	// failure this design chose Unset to avoid, and it would otherwise be invisible.
	const bool bStillPinnedByCode = VolumetricFogCVar
		&& (static_cast<EConsoleVariableFlags>(VolumetricFogCVar->GetFlags() & ECVF_SetByMask) == ECVF_SetByCode);

	if (!VolumetricFogCVar || bStillPinnedByCode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] ⛔⛔ THE FOG INTEGRITY FLOOR WAS NOT RELEASED — '%s' %s. ")
			TEXT("⛔ The floor is now PERMANENT for this session: the player's Shadows slider will no longer affect ")
			TEXT("volumetric fog, because every later scalability write loses to the SetByCode layer this release ")
			TEXT("failed to remove. That is a graphics change nobody asked for, and it is the failure mode Unset was ")
			TEXT("chosen over a captured-literal restore precisely to prevent."),
			*GetNameSafe(this),
			FogRenderFloorCVarVolumetricFog,
			VolumetricFogCVar ? TEXT("still reports SetByCode after Unset(SetByCode)") : TEXT("no longer exists"));
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Fog INTEGRITY FLOOR released — ACHIEVED '%s'=%d, now owned by SetBy%s (the player's live choice, ")
			TEXT("not the %d captured when the fog rose). The height fog's volumetric switch was restored to %s. ")
			TEXT("IsFogActive() is the one predicate that decided this, the same one that destroyed the visual."),
			*GetNameSafe(this),
			FogRenderFloorCVarVolumetricFog, VolumetricFogCVar->GetInt(),
			GetConsoleVariableSetByName(static_cast<EConsoleVariableFlags>(VolumetricFogCVar->GetFlags() & ECVF_SetByMask)),
			Prior.VolumetricFogEnabled,
			Prior.bHeightFogVolumetricEnabled ? TEXT("ON") : TEXT("OFF"));
	}

	// ⛔ LAST, so every line above could still name the captured values. ⭐ And it must happen on
	// BOTH branches: a release that reported a failure and then kept the guard UP would make the
	// NEXT match's enforce a no-op, turning one bad session into every session after it.
	FogRenderFloorPriorState = ReleasedFogRenderFloorState();
}
