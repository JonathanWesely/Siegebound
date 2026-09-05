// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Siegebound/TeamId.h" // ETeamId — the DURATION accessor samples the CASTER TEAM's hero (TASK-982 item 5a)
#include "FogVolume.generated.h"

/**
 *  Siegebound FOG STATE (TASK-998 + TASK-982; law `FOG-§6`, `FOG-§10.1`, `FOG-§10.3`,
 *  `FOG-§10.6`, `FOG-§10.7`) — the ONE authoritative answer to "is the battlefield fogged right
 *  now, is new fog PREVENTED right now, and until when in each case?".
 *
 *  ⛔⛔⛔ THIS ACTOR IS **STATE ONLY**, AND THE DISTINCTION IS THE WHOLE REASON IT EXISTS.
 *  `AFogVolume` **THE STATE OBJECT** is not `AFogVolume` **THE RENDERED VOLUME**. It holds TWO
 *  scalars and nothing else: ⛔ NO mesh, ⛔ NO material, ⛔ NO decal, ⛔ NO Niagara, ⛔ NO
 *  collision, ⛔ NO component of any kind, ⛔ NO tick. The fog you can SEE is `TASK-841`'s and it
 *  is still premise-blocked on `TASK-836`; the fog the GAME can ask about is this, and the class
 *  had to exist for the timer to have a home. ⚠️ A future reader looking for the visual should
 *  stop looking here — its absence is the design, not an omission.
 *
 *  ⭐⭐ WHY A SEPARATE ACTOR AND NOT A FIELD ON `ASiegeGameState` OR A SUBSYSTEM: `FOG-§10.1`
 *  pins the state to "the SAME ONE authoritative fog-state object (`AFogVolume`)", and
 *  `TASK-982` added its SECOND scalar ("fog PREVENTED until T", the `BrightSun` window) to THIS
 *  object rather than to a second one. `TASK-839` was asked to land the timer without this class
 *  and correctly REFUSED to invent a different home — four of its five items had nowhere to live.
 *  ⛔ Do not "simplify" this onto the game state later; that is three laws and a migration.
 *  ⛔ There is no second state actor, no second `ReadFogState`, and ⛔ no per-actor fog flag.
 *
 *  ─── ⭐⭐ THE THREE-STATE MACHINE, IN FULL (`FOG-§10.3`) ───────────────────────────────────
 *      ┌──────────┐   Fog (50g)      ┌──────────┐
 *      │  CLEAR   │ ───────────────► │  FOGGED  │  FogDurationSeconds = 300 s ⇒ expires ⇒ CLEAR
 *      │          │ ◄─────────────── │          │
 *      └────┬─────┘  Fog expires OR  └────┬─────┘
 *           │        BrightSun            │ BrightSun (60g) ⇒ CLEARS the fog AND shields
 *  BrightSun│                             ▼
 *   (60g)   └───────────────────────► ┌──────────┐  120 + 60 × floor(H / 1524)
 *                                     │ SHIELDED │  ⇒ `Fog` is REFUSED (no gold, card kept)
 *                                     │          │  ⇒ expires ⇒ CLEAR
 *                                     └──────────┘
 *
 *  ⛔⛔ THERE ARE **EXACTLY THREE** STATES, AND THE FOURTH IS UNREACHABLE **BY CONSTRUCTION**
 *  RATHER THAN BY A RULE SOMEBODY HAS TO REMEMBER. The two scalars could in principle both be
 *  live at once ("fogged AND shielded"), which is the fourth state `FOG-§10.3` exists to forbid.
 *  It cannot happen, and there are only two ways in:
 *    (a) SHIELD ARRIVES DURING FOG — `ApplyBrightSun` ZEROES the fog deadline in the same block
 *        that stamps the shield, so entering SHIELDED always exits FOGGED. ⛔ It does not "pause"
 *        the fog and it does not remember a remainder;
 *    (b) FOG ARRIVES DURING A SHIELD — `RaiseFog` REFUSES and returns false (`J-F19`), so FOGGED
 *        can never be re-entered from SHIELDED.
 *  Those two functions plus `ResetFog` (which zeroes BOTH) are the ONLY writers of either
 *  deadline in the project. ⛔ Do not add a third writer, and ⛔ do not add a companion
 *  `bool bFogPrevented` / `bool bFogCleared`: the state is derived from the two deadlines and
 *  a stored duplicate is how a state machine starts answering differently in two places
 *  (`qa/TASK-1011.md` NIT-5).
 *
 *  ⛔⛔⛔ THE ONE-WAY DOOR — HIS OWN SENTENCE, AND THE EASIEST THING IN THIS FILE TO GET WRONG:
 *  *"Even when the 'bright sun' fog prevention timer ends, the fog that was cleared STILL REMAINS
 *  CLEAR."* ⇒ **`SHIELDED` expires to `CLEAR`, ⛔ NEVER back to `FOGGED`.** That is not enforced
 *  by an expiry handler — there is no handler, and there is nothing to handle: `ApplyBrightSun`
 *  ZEROED the fog deadline, so after the shield lapses `IsFogActive()` compares the clock against
 *  `0.0` and answers false forever. ⛔ There is NO suspended fog, NO paused timer and NO
 *  remembered remainder anywhere in this class, and a "resume the fog" implementation would be a
 *  FAIL against his words rather than a missing feature.
 *
 *  ⭐ REFRESH, NEVER STACK — AND IT IS TRUE **BY CONSTRUCTION**, NOT BY A GUARD. Jonathan's
 *  ruling `J-F16`: *"If fog is played during fog then the timer is reset to 5 minutes."*
 *  `RaiseFog()` ASSIGNS the deadline (`=`), it never accumulates onto it (`+=`), so a second cast
 *  cannot stack even if someone forgets the rule. There is no `if (IsFogActive())` branch to get
 *  wrong, because the correct behaviour is what a plain assignment already does.
 *  ⚠️ `ApplyBrightSun` is the ONE place in this class where the correct behaviour genuinely IS a
 *  branch (`J-F18` is CONDITIONAL — longer resets, shorter refuses), which is exactly why that
 *  branch lives in the ONE writer rather than at a call site where a second caller could get it
 *  backwards.
 *
 *  ⚖️ NET RELEVANCY TIER: **A — REPLICATED WHEN M8 LANDS** (declared per the CONVENTIONS NET
 *  RELEVANCY LAW declaration duty; `FOG-§6`'s M8 clause names this scalar explicitly: *"the
 *  ceiling is derived from ONE replicated 'fog active until T' scalar, never from per-actor
 *  visibility"*). ⛔ TODAY it ships NOT replicated: `bReplicates` stays at the `AActor` default
 *  (false) and is never set, exactly like `AAncientGround`/`ACommanderNpc`. ⭐ WHAT M8 ADDS, so
 *  nobody re-derives it: `Replicated` on `FogActiveUntilTimeSeconds` **and on
 *  `FogPreventedUntilTimeSeconds`** (⚠️ BOTH — updated 2026-09-04 with the second scalar; a
 *  replicated fog deadline beside an unreplicated shield deadline would let a client believe fog
 *  is raisable while the server refuses it), a `GetLifetimeReplicatedProps` registration, and
 *  `bReplicates = true` in the constructor — and NOTHING else, because fog is symmetric and
 *  world-global, so it leaks nothing.
 *  ⚠️ Both deadlines are WORLD-CLOCK stamps, so M8 must publish them on the SERVER clock the way
 *  `ASiegeGameState::ClockBaseServerTime` already does — a raw `GetTimeSeconds()` value
 *  replicated verbatim would expire at a different wall-clock instant on every client.
 *
 *  ⛔ SPAWNED AT RUNTIME, NEVER LEVEL-PLACED. `FindOrSpawn` creates the single instance the first
 *  time the `Fog` card resolves, so the card cannot be dead in a level nobody remembered to place
 *  a volume in — the exact failure mode `TASK-998`'s row names ("a green suite and a 50-gold fog
 *  card that renders and clamps nobody"). ⛔ A level-placed `BP_SiegeFog` (`FOG-§6`'s BP child)
 *  would ALSO be found by `Find`, so placing one later is safe and changes nothing here.
 *
 *  ⚠️⚠️ SERIALISATION / `CoreRedirects` — DECLARED 2026-09-04, because this class is where the
 *  answer changes. BOTH deadlines (`FogActiveUntilTimeSeconds`, `FogPreventedUntilTimeSeconds`)
 *  are `Transient`: they are never written to a package, so retiring or renaming either can never
 *  orphan a saved value. The FIVE tunables — `FogDurationSeconds`, `BrightSunBaseDurationSeconds`,
 *  `BrightSunBonusSecondsPerStep`, `BrightSunHeightStepUU`, `ArenaGroundReferenceZUU` — are
 *  `EditDefaultsOnly` and therefore ARE serialised, into this class's CDO and into any Blueprint
 *  child (`/Game/Blueprints/BP_SiegeFog`). ⛔ RENAMING OR RETIRING ANY OF THE FIVE AFTER A BP
 *  CHILD EXISTS NEEDS A `CoreRedirects` ENTRY, or a designer's saved override is silently dropped
 *  on load with no error anywhere. ⚠️ The hazard GREW with `TASK-982` (1 property ⇒ 5) and it is
 *  restated rather than assumed: four of the five are new as of 2026-09-04.
 *  `FSiegeFogTuning` is deliberately NOT a member of this actor — see the handoff for why, and
 *  for what that keeps open.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AFogVolume : public AActor
{
	GENERATED_BODY()

public:

	AFogVolume();

	/**
	 *  THE READ DOOR — finds the one fog-state actor, and ⛔ NEVER creates one.
	 *
	 *  ⛔⛔ READ-ONLY IS LOAD-BEARING, NOT A STYLE NOTE. Its caller is
	 *  `FSiegeCombatStatics::ReadFogState`, which runs inside the acquisition funnel on every
	 *  gather — a finder that spawned would mutate the world from inside a query, on a 0.25 s
	 *  poll, forever. ⇒ NO fog volume in the world is a perfectly good answer and it means
	 *  exactly "no fog": nullptr here degrades to the pre-fog game, never to a crash and never
	 *  to a blind field.
	 *
	 *  Takes a `const UWorld*` because the acquisition seam has one; `TActorIterator` accepts it.
	 *  Exactly one instance exists by construction (`FindOrSpawn` is the only spawn site), so the
	 *  first hit wins and there is no nearest/best tier to resolve.
	 */
	static AFogVolume* Find(const UWorld* World);

	/**
	 *  THE WRITE DOOR — finds the one fog-state actor, creating it if this is the first cast.
	 *
	 *  ⛔ The ONLY `SpawnActor<AFogVolume>` in the project. Its caller is the `FogCover` arm of
	 *  `USpellLibrary::ResolveSpell`. A null world, or a spawn the world refuses, answers nullptr
	 *  and the card REFUSES (the caller refunds) rather than reporting a fog nobody can see —
	 *  that refusal is `TASK-839`'s loud arm, INVERTED rather than deleted: it survives as the
	 *  exceptional path instead of the only path.
	 */
	static AFogVolume* FindOrSpawn(UWorld* World);

	/**
	 *  Raises the fog for `FogDurationSeconds` from now — ⭐ REFRESH, ⛔ NEVER STACK (`J-F16`).
	 *  Calling it while fog is already up RESETS the full 5 minutes, which is his ruling
	 *  verbatim. ⛔ There is no "extend" and no "add"; see the class doc for why that is a
	 *  property of the assignment rather than a rule someone has to remember.
	 *
	 *  ⛔⛔ RETURNS FALSE — AND CHANGES NOTHING — WHILE THE `BrightSun` WINDOW IS UP (`J-F19`,
	 *  `FOG-§10.6`). Jonathan: *"prevention … will not allow any new fog to come in."* The caller
	 *  (`USpellLibrary::ResolveSpell`'s `FogCover` arm) propagates the false, and the SHIPPED
	 *  refusal doctrine does the rest: `ASiegePlayerController` refunds the full cost and never
	 *  reaches `ConfirmInstantDraw`, so ⛔ ZERO gold moves AND ⛔ the card stays in hand. Those are
	 *  ⛔ TWO separable properties, never one — a build that refunded the gold but ate the card
	 *  would satisfy exactly half his ruling.
	 *  ⭐ The refusal lives HERE, in the one writer, rather than at the call site: it is also what
	 *  makes `FOG-§10.3`'s fourth state (fogged AND shielded) unreachable by construction.
	 *  ⚠️ The player-facing message carrying the LIVE seconds remaining is ⛔ NOT this function's
	 *  and ⛔ NOT this file's — it is `TASK-989`, in `SiegePlayerController.cpp`, and it reads
	 *  `GetFogPreventionSecondsRemaining()` at CLICK time.
	 *
	 *  ⚠️ SIGNATURE CHANGED 2026-09-04 (`void` ⇒ `bool`, `TASK-982`). Structural probes that
	 *  extract this body by signature were moved in the same diff.
	 */
	bool RaiseFog();

	/**
	 *  ⭐⭐ `BrightSun` (60 g) — CLEARS the fog and opens the PREVENTION WINDOW (`FOG-§10.3`,
	 *  `FOG-§10.7` (A), rulings `J-F13`/`J-F15`/`J-F17`/`J-F18`). The window is
	 *  `GetBrightSunWindowSeconds(CasterTeam)` — sampled ⛔ ONCE, ⛔ HERE, at the instant of the
	 *  cast (`J-F15`, verbatim: *"the height is sampled at the time that the card is cast"*).
	 *
	 *  ⛔⛔ `J-F18` IS CONDITIONAL, AND IT IS A BRANCH — ⛔ not `max`, ⛔ not a refresh, ⛔ not a
	 *  blanket refuse. 📌 His words: *"the timer gets RESET to whatever the new time would be
	 *  under the new cast, UNLESS that new time would be LESS than the current time, then the
	 *  player … is basically prevented from playing the card."*
	 *    • the new window is LONGER (or EQUAL) ⇒ RESET to it, return TRUE (gold spent, card
	 *      consumed, a normal cast);
	 *    • the new window is STRICTLY SHORTER than what is left ⇒ return FALSE, ⛔ changing
	 *      NOTHING — the stored expiry stays bit-identical and zero gold moves.
	 *  ⚠️ THE BOUNDARY IS A DECLARED DEFAULT, NOT HIS WORD: he wrote *"LESS than"*, so EQUAL
	 *  RESETS (a legal, if pointless, cast). A float-equal window is unreachable in practice.
	 *  ⭐ WHY THE DELTA IS SMALLER THAN IT LOOKS, said so nobody over-builds it: `max(remaining,
	 *  new)` and "reset if longer" produce the IDENTICAL remaining time in the longer case. They
	 *  diverge ONLY in the shorter case — where `max` would have silently kept the timer while
	 *  BILLING 60 gold and EATING the card. The old default's arithmetic was right and its
	 *  economics were wrong, and ⛔ no test of the resulting DURATION could ever have caught that.
	 *  ⚠️ The two-value refusal MESSAGE ("would reduce prevention from X to Y") is ⛔ NOT this
	 *  function's — it is `TASK-991`, in `SiegePlayerController.cpp`, and it computes `Y` by
	 *  calling `GetBrightSunWindowSeconds` rather than by casting the card to find out.
	 */
	bool ApplyBrightSun(ETeamId CasterTeam);

	/**
	 *  `Play Again` / match reset ⇒ CLEAR, ⛔ BOTH timers zeroed (`FOG-§10.3`'s reset clause).
	 *  Called from `ASiegeGameMode::PlayAgain`, the one match-reset path in the project, alongside
	 *  the shipped `ResetCastle` / `ResetCaptureZone` loops it is deliberately named after.
	 *  ⛔ Zeroes the deadlines outright rather than letting them expire: a match-2 player must
	 *  never inherit match-1 fog (or match-1 immunity to it), and "wait 4 more minutes" is not a
	 *  reset.
	 *
	 *  ⭐⭐ AND THE PROPERTY THE CALLER DEPENDS ON, PRESERVED DELIBERATELY (`SC-§62` exception,
	 *  granted to `TASK-998`): ⛔ THE GAME MODE LEARNS **NO** FOG POLICY. It may tell the volume
	 *  to clear; it may not know a duration, a ceiling, a density or a window. `TASK-982` added a
	 *  second timer and the game mode's call site is ⛔ byte-unchanged — which is the whole point
	 *  of resetting BOTH scalars from in here rather than exposing a second reset entry point.
	 */
	void ResetFog();

	/**
	 *  True while the battlefield is fogged. Compared against `UWorld::GetTimeSeconds`, so it
	 *  stops with a paused world exactly as the rest of the game does (the shipped
	 *  `DeferredIntentExpiryTime` / `LadderClimbWatchdogDeadlineSeconds` idiom).
	 *  ⭐ `0.0` is an unambiguous "clear": the deadline is only ever written as
	 *  `now + FogDurationSeconds` with both terms positive, so a live deadline is always > 0.
	 */
	bool IsFogActive() const;

	/**
	 *  True while the `BrightSun` prevention window is up — i.e. the machine is in `SHIELDED`.
	 *  The exact sibling of `IsFogActive()`, over the second scalar, with the same strict `<` and
	 *  the same fail-toward-`CLEAR` behaviour on a missing world.
	 *  ⛔ It is NOT a stored flag and there is no stored flag: SHIELDED is derived from the one
	 *  deadline, exactly as FOGGED is derived from the other.
	 */
	bool IsFogPrevented() const;

	/**
	 *  ⭐⭐ THE LIVE REMAINDER — seconds until the `BrightSun` window lapses; ⛔ `0` whenever the
	 *  machine is not `SHIELDED` (`FOG-§10.6`). ⛔ NEVER negative.
	 *
	 *  ⛔⛔ IT RECOMPUTES FROM THE CLOCK ON **EVERY CALL**, AND THAT IS THE REQUIREMENT RATHER
	 *  THAN AN IMPLEMENTATION DETAIL. 📌 Jonathan: *"a message telling them bright sun is still up
	 *  for 'x' amount of seconds, where the 'x' is the ACTUAL amount of time left."* A remainder
	 *  captured when `BrightSun` was PLAYED would be stale by exactly the elapsed duration, so the
	 *  message would count down from the wrong number or never change at all. ⚠️ `SC-§37`: a
	 *  single-click test cannot tell a live read from a cached one — the gate needs TWO refusals
	 *  separated in time.
	 *
	 *  ⭐ CALLERS (both boarded, so this is not dead surface — `SC-§40` cl. 2): `TASK-989` (the
	 *  `Fog`-during-prevention refusal, one value) and `TASK-991` (the sun-on-sun refusal, where
	 *  it is the `X` of "would reduce prevention from X to Y"). ⛔ This row ships it with no caller
	 *  of its own, on purpose.
	 */
	float GetFogPreventionSecondsRemaining() const;

	/**
	 *  ⭐⭐ THE DURATION ACCESSOR (`TASK-982` item 5a; `FOG-§10.7` (A)) — *"what prevention window
	 *  WOULD a `BrightSun` cast produce RIGHT NOW?"*, in seconds, for `CasterTeam`'s hero.
	 *
	 *  ⛔⛔ PUBLIC, `const`, SIDE-EFFECT-FREE AND CALLABLE **OUTSIDE THE CAST PATH** — and that is
	 *  a SEAM REQUIREMENT, not a style choice. `TASK-991` must compute a full window just to
	 *  EXPLAIN a refusal (his `Y` is *"the new fog prevention time under the current height
	 *  calculation"*). A duration computed inside `ApplyBrightSun` would force that row either to
	 *  DUPLICATE the formula — two copies that drift until the message starts lying — or to CAST
	 *  THE CARD TO FIND OUT WHETHER TO CAST IT. ⛔ Both are defects; this accessor is the fix.
	 *
	 *  ⛔ IT SAMPLES HEIGHT ON EVERY CALL, live: the answer CHANGES as the hero climbs, which is
	 *  exactly what makes it usable for a refusal message. `ApplyBrightSun` is the only place the
	 *  answer is ever FROZEN, and it freezes it once, at the cast (`J-F15`).
	 *
	 *  Degrades to `BrightSunBaseDurationSeconds` (the 2-minute floor, ⛔ never to a bonus) when
	 *  there is no world and when `CasterTeam` has no living hero to measure — the bot's case,
	 *  and the same fail-toward-the-base direction the rest of this file uses.
	 */
	float GetBrightSunWindowSeconds(ETeamId CasterTeam) const;

	/**
	 *  ⭐⭐ THE FORMULA, AS A PURE FUNCTION (`FOG-§10.3`, `TASK-982` item 3):
	 *
	 *      Window = BaseSeconds + BonusSecondsPerStep × floor(max(0, HeroZUU − GroundReferenceZUU) / HeightStepUU)
	 *
	 *  ⛔ `floor`, ⛔ never round — a rounded rule would grant the next minute HALF a step early,
	 *  and the step boundaries are the only place a player can feel this mechanic at all.
	 *  ⛔ Height clamps at 0 steps: below the datum there is no malus, only no bonus (the
	 *  `HeightAdvantageMultiplier` `FMath::Max(0.f, …)` precedent — he asked for a bonus, and
	 *  inventing a low-ground penalty is inventing a mechanic).
	 *  ⛔ ⭐ UNCAPPED (`J-F14`, ruled). At the shipped 50-ft step a ×2 Watch Tower (~2,400 uu) is
	 *  `floor(2400 / 1524)` = ⭐ ONE step ⇒ 2 minutes becomes 3, which is why uncapped is
	 *  comfortable rather than alarming here.
	 *
	 *  ⛔ Public, plain C++ static, ⛔ NOT a `UFUNCTION`, ⛔ no defaulted parameters (`SC-§33`).
	 *  No world access and no actor access — every tunable is a parameter, which is the
	 *  `ASummonedUnit::HeightAdvantageMultiplier` testability-seam precedent, followed on purpose
	 *  so the step boundaries can be asserted headlessly.
	 *  A non-positive (or NaN) `HeightStepUU` yields 0 steps, i.e. exactly `BaseSeconds` — total,
	 *  never a divide by zero (the same `!(X > 0.f)` shape, which also catches NaN).
	 */
	static float BrightSunWindowSeconds(float HeroZUU, float GroundReferenceZUU, float BaseSeconds, float BonusSecondsPerStep, float HeightStepUU);

protected:

	/**
	 *  ⛔⛔ HIS NUMBER: *"fog is up for exactly 5 minutes when the card is played"* ⇒
	 *  5 min × 60 s/min = **300 s** (`FOG-§9.4`, ruling `J-F16` — and `J-F16`'s own sentence
	 *  *"the timer is reset to 5 minutes"* re-derives the same 300 from a SECOND sentence of his,
	 *  which is the strongest confirmation a tunable ever gets here).
	 *
	 *  ⚠️ THE CONSEQUENCE, WRITTEN BESIDE THE NUMBER PER `HIGH-§1`, BECAUSE A NUMBER WHOSE
	 *  CONSEQUENCE IS NOT WRITTEN NEXT TO IT GETS RETUNED BY SOMEONE WHO DOES NOT KNOW WHAT THEY
	 *  ARE CHANGING: this is the entire duration a 50-gold card buys, and while it runs
	 *  ⛔ **EVERY** unit on BOTH sides — ⛔ melee included, ⛔ not only the ranged ones — is cut to
	 *  `FogVisionCeilingUU` (609.6 uu) from `ASummonedUnit::UnitEngagementRadiusUU` = `5000`,
	 *  an ⛔ **87.8%** reduction in acquisition reach (`1 − 609.6 / 5000 = 87.808%`), symmetric and
	 *  world-global. ⛔ Lowering it toward zero makes the card cost 50 gold for nothing; raising it
	 *  past a match length makes the fog permanent and half the roster ornamental.
	 *  `EditDefaultsOnly` so his next sentence retunes it with no code change — that is the point
	 *  of the property, not a side effect.
	 *
	 *  ⛔⛔ CORRECTED 2026-09-04 (`TASK-982` item 0a), AND THE STRUCK NUMBERS ARE NAMED RATHER THAN
	 *  DELETED SO A READER WHO REMEMBERS THEM FINDS THEIR REPLACEMENT (`SC-§53` cl. 3): this block
	 *  said ⛔ ~~*"every RANGED unit … cut to 609.6 uu from `2000` — a 69.5% reduction"*~~. All
	 *  three parts were false. 🧑 `J-F28` raised the universal notice radius to `5000`
	 *  (`SummonedUnit.h:895`), and ⭐⭐⭐ `FOG-§9.11` retired the notice==firing identity, so the
	 *  cut applies to every class rather than to the ranged ones. `69.5%` is `1 − 609.6 / 2000`,
	 *  i.e. the arithmetic of the retired default. Authority: ⛔ **`qa/TASK-1014.md`** Ruling B.
	 *  ⛔⛔⛔ AND THE TRAP, RECORDED BECAUSE IT IS THE DANGEROUS PART: `qa/TASK-1011.md` NIT-2
	 *  graded this very block *"TRUE TODAY"*. ⛔ That verdict is ⛔ SUPERSEDED — its premise was
	 *  wrong on BOTH clauses — and it must ⛔ NEVER travel forward as a clearance for these
	 *  sentences. ⛔ Cite `qa/TASK-1014.md`; ⛔ never NIT-2. A stale VERDICT is worse than a stale
	 *  claim, because it launders the claim into the record as verified.
	 *
	 *  ⛔ `EffectDuration` on the `Fog` card row (`cards.csv`, `TASK-840`) MUST match this. It is
	 *  the ONLY cross-file agreement this constant has, and until `TASK-840` lands there is
	 *  nothing on the other side of it.
	 *  ⚠️ THAT LAST SENTENCE IS ALSO STALE and is ⛔ DELIBERATELY LEFT STANDING: the `Fog` row HAS
	 *  landed (`cards.csv`, `EffectDuration = 300`), and `qa/TASK-1014.md` routed this ONE claim
	 *  to ⭐ `TASK-1016` item (3) rather than here, because `TASK-1016` is the row that makes the
	 *  cell AUTHORITATIVE — repairing the prose here while the cell stays inert would describe a
	 *  wiring that still does not exist. ⛔ Reported, ⛔ not swept.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Fog", meta = (ClampMin = "1.0"))
	float FogDurationSeconds = 300.f;

	//~ ─── ⭐⭐ `BrightSun` — THE PREVENTION WINDOW'S FOUR TUNABLES (`TASK-982`, `FOG-§10`) ───
	//~ ⛔ All four `EditDefaultsOnly` with their `HIGH-§1` consequence written beside them, for the
	//~ reason `HIGH-§1` exists: a number whose consequence is not next to it gets retuned by
	//~ somebody who does not know what they are changing.

	/**
	 *  ⛔ HIS NUMBER: the `BrightSun` window's FLOOR — *"base … is 2 minutes"* ⇒ 2 × 60 = **120 s**,
	 *  earned by a cast from the ground with no height bonus at all (`FOG-§10.1` also pins the
	 *  card row's `EffectDuration` at `120`, the same number from a second sentence of his).
	 *
	 *  ⚠️ CONSEQUENCE (`HIGH-§1`): this is the whole of what 60 gold buys a player standing on
	 *  flat grass. ⛔ At 0 the card does nothing unless he climbs, which turns a 60-gold spell into
	 *  a tower-only spell; ⛔ raised past `FogDurationSeconds` (300) it makes a ground-level
	 *  `BrightSun` strictly better than the `Fog` it counters, from anywhere, forever.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|BrightSun", meta = (ClampMin = "0.0"))
	float BrightSunBaseDurationSeconds = 120.f;

	/**
	 *  ⛔ HIS NUMBER: the window *"increases by 1 minute"* per height step ⇒ **60 s** per step,
	 *  added ⛔ per COMPLETED step (`floor`), ⛔ ADDITIVE and ⛔ UNCAPPED (`J-F14`, ruled).
	 *
	 *  ⚠️ CONSEQUENCE (`HIGH-§1`): this is the entire reward for climbing before casting, and it
	 *  is the ONLY thing tying this card to the elevation game. ⛔ At 0 the height mechanic is
	 *  silently deleted while every test that only checks the base still passes; ⛔ raised, it
	 *  compounds with an UNCAPPED step count, so a tall enough perch buys an arbitrarily long
	 *  window. ⭐ The pairing that keeps it sane is the 50-ft step below, ⛔ not a cap.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|BrightSun", meta = (ClampMin = "0.0"))
	float BrightSunBonusSecondsPerStep = 60.f;

	/**
	 *  ⛔⛔ HIS NUMBER, AMENDED BY HIM ON 2026-09-04 FROM 20 ft TO **50 ft**: one height step is
	 *  50 ft × 30.48 cm/ft = **1524 uu**. ⛔ `1524`, ⛔ NOT `609.6`.
	 *
	 *  ⛔⛔⛔ IT IS A **SEPARATE CONSTANT** FROM `FSiegeFogTuning::FogVisionCeilingUU`, AND THAT IS
	 *  LOAD-BEARING RATHER THAN INCIDENTAL (`FOG-§9.5`). The two were `609.6` together under the
	 *  old 20-ft reading and it would have been tempting to reference the ceiling here. ⛔ Doing so
	 *  would couple this card to a constant somebody else may retune for an unrelated reason —
	 *  and `J-F12`'s own named remedy for over-strong fog is *"lower the ceiling"*, which would
	 *  then SILENTLY re-tune `BrightSun`'s height reward as a side effect of a vision decision.
	 *  ⛔ Referencing `FogVisionCeilingUU` from this file is an automatic fail.
	 *
	 *  ⚠️ CONSEQUENCE (`HIGH-§1`): this is the price of one extra minute. ⛔ Halving it doubles
	 *  every window earned from every perch in the game at a stroke; ⛔ at 0 (or negative) the
	 *  formula degrades TOTALLY to the base 120 s rather than dividing by zero. ⭐ At the shipped
	 *  50 ft a ×2 Watch Tower (~2,400 uu) earns `floor(2400 / 1524)` = ONE step — the stacked-tower
	 *  combo is modest rather than dominant, which is a real reason 50 is the better figure and is
	 *  recorded here because he may not have derived it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|BrightSun", meta = (ClampMin = "0.0"))
	float BrightSunHeightStepUU = 1524.f; // FOG-§10: 50 ft × 30.48 cm/ft

	/**
	 *  ⭐⭐ THE ARENA GROUND DATUM — the Z that "above the ground" is measured FROM, and ✅ `J-F13`
	 *  CLOSED IT: it is a ⛔ FLAT CONSTANT. 📌 His words: *"The ground is simply the elevation of
	 *  the flat grass terrain, NOT INCLUDING THE HILLS, therefore that ground height number should
	 *  be the SAME ANYWHERE ON THE MAP."*
	 *
	 *  ⛔⛔⛔ THERE IS NO TRACE. ⛔ No trace channel, ⛔ no ignore list, ⛔ no missed-trace degrade
	 *  path — anywhere in this class. The pre-ruling default WAS a downward trace to world-static
	 *  terrain with buildings ignored, and it is ⛔ REFUTED: under his ruling a hero on a HILL
	 *  ⛔ EARNS the height, exactly as he earns the elevation DAMAGE bonus there.
	 *
	 *  ⛔ `0` IS NOT A GUESS AND MUST NOT BE "TIDIED" — a bare `0.f` reads as a placeholder, so the
	 *  derivation is quoted here: `CONVENTIONS:131` pins `SM_ArenaTerrain` as *"placed at (0,0,0)
	 *  it reproduces the old ArenaGround slab's WALK SURFACE at Z=0"*. That walk surface IS the
	 *  flat grass terrain of his sentence.
	 *
	 *  ⭐⭐ ONE CONSTANT, TWO CONSUMERS, AND THE SECOND ONE IS AN **ASSERTION** RATHER THAN A CALL
	 *  (`FOG-§10.7` (D) rules 3 + 4). His sentence joined two numbers that were never joined:
	 *  *"it should be the SAME HEIGHT in which ranged units' damage is at 1 times their damage."*
	 *    • CONSUMER 1 (runtime): this card's height zero — `BrightSunWindowSeconds` above.
	 *    • CONSUMER 2 (test-only): `Tests/SiegeBrightSunTest.cpp` pins
	 *      `ASummonedUnit::HeightAdvantageMultiplier(ArenaGroundReferenceZUU, ArenaGroundReferenceZUU, …)
	 *      == 1.0` exactly, and `(+ HeightBonusStepUU, …) == 1.10`. That is his sentence made
	 *      executable, and it goes RED if EITHER lane's datum moves.
	 *  ⛔⛔⛔ THE DAMAGE FORMULA IS **NOT** CHANGED, NOT ONE LINE. `ComputeOutputDamage` passes the
	 *  TARGET's own Z as its zero (`SummonedUnit.cpp`, `HIGH-§2` row `R-1`: *"above the target, NOT
	 *  absolute world Z"*), so the damage lane's ×1.0 is a RELATIVE condition true at any absolute
	 *  elevation — there is no elevation-damage zero constant to reuse. Converting that call to
	 *  read this constant would be a game-wide rebalance and would contradict his own earlier
	 *  ruling. ⛔ The tie is an assertion, ⛔ never a call.
	 *
	 *  ⚠️ A THIRD SITE HOLDS THIS SAME NUMBER AND IS ⛔ DELIBERATELY NOT SWEPT:
	 *  `FSiegeAssistantSnapshot::MarkPlaceGroundZ = 0.f`. Its own comment licenses it to be retuned
	 *  for hill-accurate marks — i.e. it may legitimately stop being the flat-grass datum — which
	 *  is the exact `FOG-§9.5` coupling trap. ⛔ Reported, ⛔ never referenced from here.
	 *
	 *  ⚠️ CONSEQUENCE (`HIGH-§1`): every window in the game is measured from this line. ⛔ Raising
	 *  it by one step's worth deletes the first step of reward for every cast on the map; lowering
	 *  it hands out a free step from flat grass.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|BrightSun")
	float ArenaGroundReferenceZUU = 0.f;

private:

	/**
	 *  ⭐⭐ **THE ONE SCALAR** — world time at which the fog lifts; `0.0` means CLEAR.
	 *  This is `FOG-§6`'s M8 *"fog active until T"* value, and there is exactly one of it in the
	 *  project. ⛔ Do NOT add a companion `bool bFogActive`: two representations of one fact is
	 *  how a state machine starts answering differently in two places, and `IsFogActive()` already
	 *  derives the boolean for free.
	 *
	 *  `Transient` because it is per-match runtime state that must never survive into a package
	 *  (see the class doc's serialisation note). `double` and initialised in-class, the shipped
	 *  deadline convention. ⚠️ `TASK-982` added the SECOND scalar beside this one; it does not
	 *  replace it and it does not fold into it — `FOG-§10.3` has THREE states and collapsing them
	 *  onto one number is the defect that section exists to prevent.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Fog", meta = (AllowPrivateAccess = "true"))
	double FogActiveUntilTimeSeconds = 0.0;

	/**
	 *  ⭐⭐ **THE SECOND SCALAR** (`TASK-982`; `FOG-§10.1`: *"`BrightSun` adds a SECOND SCALAR
	 *  ('fog prevented until T') to it, NEVER a second state object"*) — world time at which the
	 *  `BrightSun` prevention window lapses; `0.0` means "not shielded".
	 *
	 *  ⛔ It lives HERE, beside the fog deadline, on the SAME one actor. ⛔ Never a second state
	 *  actor, ⛔ never a second `ReadFogState`, ⛔ never a per-actor flag (`FOG-§6`'s M8 clause).
	 *  ⛔ And ⛔ NEVER folded into `FogActiveUntilTimeSeconds` as a sign or a sentinel: three states
	 *  need two independent deadlines, and the one thing a single number could not express is the
	 *  distinction between *"the fog ran out"* and *"the fog was BURNED OFF and cannot return"*.
	 *
	 *  ⚠️ IT IS NOT A REMEMBERED FOG. When `ApplyBrightSun` writes this, it also ZEROES the fog
	 *  deadline — so when THIS one lapses there is nothing left to resume, which is precisely how
	 *  the one-way door in the class doc is enforced (`SHIELDED` ⇒ `CLEAR`, ⛔ never ⇒ `FOGGED`).
	 *
	 *  `Transient` and `double`, matching the scalar above exactly — same package rule, same
	 *  clock, same convention.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|BrightSun", meta = (AllowPrivateAccess = "true"))
	double FogPreventedUntilTimeSeconds = 0.0;
};
