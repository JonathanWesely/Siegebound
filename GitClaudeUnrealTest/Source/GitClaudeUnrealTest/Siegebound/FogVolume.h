// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"    // FTimerHandle — TASK-1068's visual-expiry WAKE-UP (⛔ NOT a second deadline; see the one-way-door paragraph)
#include "GameFramework/Actor.h"
#include "Siegebound/TeamId.h"     // ETeamId — the DURATION accessor samples the CASTER TEAM's hero (TASK-982 item 5a)
#include "UObject/SoftObjectPath.h" // FSoftClassPath — the return type of the ONE fog-visual class reference; the PATH itself lives in the .cpp (TASK-1068 cl. 6)
#include "FogVolume.generated.h"

/**
 *  Siegebound FOG STATE (TASK-998 + TASK-982; law `FOG-§6`, `FOG-§10.1`, `FOG-§10.3`,
 *  `FOG-§10.6`, `FOG-§10.7`) — the ONE authoritative answer to "is the battlefield fogged right
 *  now, is new fog PREVENTED right now, and until when in each case?".
 *
 *  ⛔⛔⛔ THIS ACTOR RENDERS **NOTHING**, AND THE DISTINCTION IS THE WHOLE REASON IT EXISTS.
 *  `AFogVolume` **THE STATE OBJECT** is not `AFogVolume` **THE RENDERED VOLUME**. Its own body
 *  holds TWO scalars and draws nothing: ⛔ NO mesh, ⛔ NO material, ⛔ NO decal, ⛔ NO Niagara,
 *  ⛔ NO collision, ⛔ NO component of any kind, ⛔ NO tick. The fog you can SEE is `TASK-841`'s
 *  and it ⛔ HAS SHIPPED: 2026-09-05, as `/Game/Blueprints/BP_SiegeFog` (integration-checked
 *  under `TASK-1043`, committed `ef2c901`) — a child of the ⛔ VENDOR `BP_FogArea`.
 *  The `TASK-836` measurement that once premise-blocked it is ✅ DONE (2026-09-03).
 *  ⛔⛔ CORRECTED 2026-09-05 (`TASK-1068`) — THE HEADING USED TO SAY **"STATE ONLY"** AND THE
 *  PARAGRAPH USED TO END *"a future reader looking for the visual should stop looking here — its
 *  absence is the design, not an omission"*. ⛔ BOTH WERE TRUE WHEN WRITTEN AND ⛔ BOTH ARE NOW
 *  FALSE, and the second one was ⛔ ACTIVELY HARMFUL: it sent the one reader who came here asking
 *  the right question ("who spawns the fog?") back out of the only file that could answer.
 *  ⇒ ⭐ THIS CLASS NOW OWNS THE VISUAL'S **LIFETIME** — ⛔ never its LOOK, ⛔ never its
 *  material, ⛔ never its density. It spawns `BP_SiegeFog` when fog rises and destroys it when
 *  fog ends, through the ONE reconciler `RefreshFogVisual()`. ⛔ The visual holds ⛔ NO state of
 *  its own (`list_variables` ⇒ `[]`, `TASK-841` §4), so "the volume is in the world" IS "fog is
 *  up", and there is still exactly ⛔ ONE source of truth for that: `FogActiveUntilTimeSeconds`.
 *  ⚠️ Why lifetime and not a Blueprint poll: see THE BLUEPRINT SEAM below — a Blueprint
 *  ⛔ CANNOT read fog state today, so C++ lifetime control was the only seam available.
 *  The fog the GAME can ask about is this, and the class had to exist for the deadline to have a
 *  home.
 *  ⛔⛔ CORRECTED 2026-09-05 (`TASK-1053`, from `qa/TASK-1051.md`), AND THE REASON IT LASTED IS
 *  THE REUSABLE PART: this sentence said the visual was *"still premise-blocked on `TASK-836`"*
 *  and ⛔ SURVIVED A DELIBERATE SWEEP OF THIS VERY FILE, because that sweep's predicate was the
 *  ⛔ CLAIM (*"sentences asserting the wrong parent"*) while this sentence was stale on a
 *  ⛔ DIFFERENT AXIS. ⇒ ⭐ `SC-§91`: sweep by ⛔ SUBJECT (*every sentence in this file*),
 *  ⛔ never by the shape of the error you already know about.
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
 *  ⚠️⚠️ NARROWED 2026-09-05 (`TASK-1068`), BECAUSE THAT ROW ADDED A TIMER HANDLE TO THIS CLASS
 *  AND THE SENTENCE ABOVE WOULD OTHERWISE READ AS ITS REFUTATION: *"there is no handler, and
 *  there is nothing to handle"* is a statement about **THE STATE MACHINE**, and it is ⛔ STILL
 *  TRUE OF IT — the three states are derived from two deadlines by a lazy comparison and need no
 *  expiry event at all. ⛔ A **VISUAL** does, because an actor does not despawn itself at a
 *  deadline nobody reads. ⇒ `FogVisualExpiryTimerHandle` is a ⛔ WAKE-UP, ⛔ NOT AN AUTHORITY:
 *  its fire-time is DERIVED from `FogActiveUntilTimeSeconds` (never from `FogDurationSeconds`,
 *  which is ⛔ never re-typed into a second call), it stores no deadline anybody reads back, and
 *  the function it wakes RE-ASKS `IsFogActive()` rather than acting on the fact that it fired.
 *  ⇒ ⭐ A timer that fires EARLY re-arms; one that fires LATE destroys a hair late. ⛔ Neither
 *  can make the machine answer differently in two places, which is the property the ban exists
 *  to protect.
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
 *  ⛔ THE NATIVE ACTOR IS SPAWNED AT RUNTIME AND IS NEVER LEVEL-PLACED BY US — ⚠️ a statement
 *  about how the ⛔ ONE instance ARRIVES, ⛔ NOT a guarantee that nothing of this class can be
 *  placed in a map; the two paragraphs below are about exactly that case (heading narrowed
 *  2026-09-05, `TASK-1053` from `qa/TASK-1051.md`: the old *"NEVER LEVEL-PLACED"* read as a
 *  guarantee sitting directly above them). `FindOrSpawn` creates the single instance the first
 *  time the `Fog` card resolves, so the card cannot be dead in a level nobody remembered to place
 *  a volume in — the exact failure mode `TASK-998`'s row names ("a green suite and a 50-gold fog
 *  card that renders and clamps nobody").
 *  ⛔⛔ CORRECTED 2026-09-05 (`TASK-1050`) — THE PREVIOUS SENTENCE HERE WAS FALSE, AND IT WAS FALSE
 *  IN THE DIRECTION THAT READS AS REASSURANCE. It said a level-placed `BP_SiegeFog` "would ALSO be
 *  found by `Find`, so placing one later is safe". `Find` iterates `TActorIterator<AFogVolume>`,
 *  which matches THIS CLASS AND ITS SUBCLASSES — and `BP_SiegeFog` is not one of them: its parent
 *  was read back LIVE as `BP_FogArea_C` (`TASK-1043`, committed `ef2c901`).
 *  ⇒ A level-placed `BP_SiegeFog` is INVISIBLE to `Find`. It cannot reach fog STATE — ⛔ and
 *  that is the NARROW claim, ⛔ about ONE FUNCTION, ⛔ not a licence to place one (narrowed
 *  2026-09-05, `TASK-1053` from `qa/TASK-1051.md`: the earlier *"harmless here"* read as *"fine
 *  to do"*, six lines below the paragraph naming *"a 50-gold fog card that renders and clamps
 *  nobody"* as the failure mode — and a level-placed visual with NO state actor behind it is
 *  that same failure mode inverted). ⛔ The mechanism is the OPPOSITE of the one this comment
 *  used to give: it is not "found too", it is never found at all. ⚠️ What WOULD be found is a
 *  level-placed Blueprint child of `AFogVolume` itself — see the `CoreRedirects` paragraph
 *  below, because that is the same fact wearing its other face.
 *
 *  ⚠️⚠️ SERIALISATION / `CoreRedirects` — DECLARED 2026-09-04, because this class is where the
 *  answer changes. BOTH deadlines (`FogActiveUntilTimeSeconds`, `FogPreventedUntilTimeSeconds`)
 *  are `Transient`: they are never written to a package, so retiring or renaming either can never
 *  orphan a saved value. ⛔⛔ THE HAZARD IS STATED AS A **PREDICATE**, ⛔ NOT AS A COUNT
 *  (`SC-§91`, re-shaped 2026-09-05 by `TASK-1068` — a count in a comment goes stale on the next
 *  edit, including its own, and the neighbouring paragraph was falsified by exactly that):
 *  ⭐ **EVERY `EditDefaultsOnly` PROPERTY ON THIS CLASS, whatever their number, IS serialised**
 *  — into this class's CDO and into any Blueprint child OF THIS CLASS. ⛔ RENAMING OR RETIRING
 *  ANY OF THEM AFTER SUCH A CHILD EXISTS NEEDS A `CoreRedirects` ENTRY, or a designer's saved
 *  override is silently dropped on load with no error anywhere.
 *  Today that predicate selects exactly these, NAMED so a reader can find them (⛔ the names are
 *  the useful part; ⛔ the arithmetic is not): `FogDurationSeconds`,
 *  `BrightSunBaseDurationSeconds`, `BrightSunBonusSecondsPerStep`, `BrightSunHeightStepUU`,
 *  `ArenaGroundReferenceZUU`. ⚠️ The hazard GREW with `TASK-982` (1 property ⇒ 5) and it is
 *  restated rather than assumed: four of those are new as of 2026-09-04.
 *  ✅ RE-CHECKED 2026-09-05 (`TASK-1068`, its clause 6a): that row added a class REFERENCE
 *  (`FogVisualClassPath()`, a plain static — ⛔ not a `UPROPERTY`), three geometry constants
 *  (`static constexpr` — ⛔ not `UPROPERTY`s), a `Transient` visual handle and a timer handle.
 *  ⛔ NONE of them is `EditDefaultsOnly`, so the list above is unchanged BY MEASUREMENT rather
 *  than by omission. ⭐ That was a deliberate design choice and its grounds are on
 *  `FogVisualClassPath()` — an `EditDefaultsOnly` class pointer would have ADDED a serialisation
 *  hazard to buy an override that, per the consequence paragraph below, has nowhere to live.
 *
 *  ⛔⛔ WHAT THE 2026-09-04 WORDING GOT WRONG — CORRECTED 2026-09-05 (`TASK-1050`), AND STATED
 *  BLUNTLY BECAUSE IT WAS FALSE RATHER THAN MERELY STALE: it named `/Game/Blueprints/BP_SiegeFog`
 *  as that Blueprint child. ⛔ IT IS NOT ONE. That asset's parent was read back LIVE via
 *  `get_parent` as `/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C` (`TASK-1043`) and committed
 *  in `ef2c901`, and a Blueprint has EXACTLY ONE parent.
 *  ⇒ MEASURED 2026-09-05: `AFogVolume` has ZERO Blueprint children in this project — ⛔ AND THE
 *  METHOD IS RECORDED HERE (`TASK-1053`, from `qa/TASK-1051.md`), because an unmethoded
 *  *"MEASURED"* carries the AUTHORITY of a measurement and the EVIDENCE of an opinion, and
 *  nobody re-derives it precisely BECAUSE it already says *"measured"*:
 *    • WHAT: a byte scan of ⛔ every `.uasset` and `.umap` under `Content/` for the FName
 *      `FogVolume` — the string a Blueprint child ⛔ MUST carry, because its parent class is an
 *      import (`/Script/GitClaudeUnrealTest.FogVolume`) and an UNCOOKED package stores its
 *      import names in a ⛔ PLAIN name table. ⇒ ⛔ NOTHING matched. (Nothing matched on the
 *      `.umap` side either, which corroborates the heading above from the map direction: no
 *      ⛔ NATIVE `AFogVolume` is level-placed anywhere either.)
 *    • ⭐ WHY THE INSTRUMENT IS BELIEVED — it was validated against the ⛔ FAILURE it detects,
 *      ⛔ not merely against success: `BP_HeroCharacter` DOES carry `HeroCharacter` and
 *      `BP_CommanderNpc` DOES carry `CommanderNpc` (native parents), and `BP_SiegeFog` DOES
 *      carry `BP_FogArea` (Blueprint parent). ⇒ A null result from THIS scan is ⛔ EVIDENCE,
 *      ⛔ not silence — which a bare text grep over `.uasset` files is not entitled to be.
 *    • WHEN: 2026-09-05, against the working tree at `c79bf5b`.
 *    • ⛔ WHAT WOULD FALSIFY IT: any asset under `Content/` — or under a NEW content root, e.g.
 *      a plugin, of which this project has none today — whose package bytes contain `FogVolume`.
 *    • ⛔ WHAT IT CANNOT SEE: the scan reads ⛔ DISK ONLY. A child created in an open editor and
 *      not yet saved is invisible to it, and so is anything outside `Content/`. ⛔ It is also not
 *      a live engine query — `SC-§78` still applies, and an MCP `get_parent` sweep would be the
 *      stronger instrument the next time the editor is open for another reason.
 *    ⛔ NOT re-run since. ⛔ Re-run it rather than trusting this line whenever the answer is
 *    load-bearing — which, per the ruling below, it is.
 *
 *  ⇒ ⭐⭐ THE CONSEQUENCE, WHICH IS THE HALF WORTH READING: WITH NO BLUEPRINT CHILD, A DESIGNER
 *  OVERRIDE OF THE FIVE CANNOT EXIST TODAY — there is nowhere to put one. `EditDefaultsOnly` is
 *  archetype-only by definition (⛔ NOT editable on a placed instance); the `.ini` route is
 *  barred by ⛔ EXACTLY ONE MISSING SPECIFIER, ⛔ not by two — `config=Engine` is declared on the
 *  very `AActor` `UCLASS` line the ruling below quotes, and it is ⛔ INHERITED, so this class
 *  ALREADY has a config home; what is absent is the ⛔ PER-PROPERTY `Config` specifier on each of
 *  the five (stated precisely 2026-09-05, `TASK-1053` from `qa/TASK-1051.md`, and it
 *  ⛔ STRENGTHENS the DORMANT ruling below rather than weakening it: that is a ⛔ SECOND live
 *  wire, ⛔ not a closed door); and the native CDO takes its values from the
 *  constructor, not from a package anybody edits. Blueprint class defaults are the ONLY writable
 *  home, and there is no Blueprint to hold them. ⇒ Nothing serialised exists for a rename to
 *  orphan, so the hazard CANNOT FIRE TODAY.
 *
 *  ⚖️⛔⛔ AND THE RULING, BECAUSE "MOOT" IS NOT AN ANSWER: THE HAZARD IS **DORMANT**, ⛔ NOT
 *  **RETIRED** (`TASK-1050` cl. 2). RETIRED would mean it can NEVER apply, which would require
 *  `AFogVolume` to be incapable of having a Blueprint child. ⛔ It is not: `AActor` is declared
 *  `UCLASS(BlueprintType, Blueprintable, …)`, `Blueprintable` is INHERITED by subclasses unless a
 *  subclass says `NotBlueprintable`, and this class says no such thing — it is a bare `UCLASS()`.
 *  ⇒ Right-click ⇒ Blueprint Class ⇒ `AFogVolume` SUCCEEDS TODAY, in one gesture, with no code
 *  change and no review. The instant that child is saved with any of the five changed, a serialised
 *  override exists and the full hazard is back.
 *  ⛔ AND NOTHING WOULD WARN THE PERSON WHO DOES IT: `Config/` contains ZERO `CoreRedirects`
 *  entries (measured 2026-09-05) and no test asserts on any of this.
 *  ⚠️⚠️ WORSE THAN ORDINARY DORMANCY — IT IS DORMANT WITH A LIVE WIRE ATTACHED: `Find` matches
 *  SUBCLASSES, and `FindOrSpawn` calls `Find` BEFORE it spawns. So a level-placed Blueprint child
 *  would not merely sit there holding overrides — it would be RETURNED AS THE ONE AUTHORITATIVE
 *  FOG-STATE ACTOR, its tuned five in force, and the native spawn would never happen.
 *  ⭐ Note what the wrong name cost, since it is the reusable lesson: an auditor asking "does a BP
 *  child exist yet?" would have found `BP_SiegeFog`, answered YES, and been wrong — in EITHER
 *  direction. Believing the hazard already live invites a pointless redirect; believing it already
 *  handled invites a free rename. ⛔ DO NOT DELETE THIS PARAGRAPH ON THE STRENGTH OF "no child
 *  exists". That absence IS the dormancy; it is not a refutation of it.
 *
 *  ⚠️ THE BLUEPRINT SEAM — MEASURED 2026-09-05 under `TASK-841` (artist) AND RE-COUNTED HERE.
 *  ⛔ A DATED FACT, ⛔ NOT A BAN: `AFogVolume` exposes NO `UFUNCTION` AT ALL. `Find`, `FindOrSpawn`,
 *  `IsFogActive`, `IsFogPrevented`, `GetFogPreventionSecondsRemaining` and the rest are plain C++,
 *  and `FSiegeFogStatics` is a non-reflected static library. (⛔ A grep for `UFUNCTION` in this
 *  header returns ⛔ PROSE ONLY — this paragraph and the comment on `BrightSunWindowSeconds`
 *  saying it is NOT one are among the hits — and ⛔ ZERO of them is a DECLARATION. ⛔ The zero is
 *  the whole finding. ⛔ Stated as a PREDICATE rather than as a COUNT on purpose: the wording
 *  that stood here before 2026-09-05 asserted a specific NUMBER of textual hits and was
 *  ⛔ FALSIFIED BY THE ACT OF WRITING IT — the same diff added further occurrences, one of them
 *  INSIDE the sentence itself (`qa/TASK-1051.md`; ⭐ `SC-§91`). ⛔ A count in a comment goes
 *  stale on the next edit, ⛔ including its own; a predicate does not.)
 *  ⇒ A BLUEPRINT CANNOT POLL FOG STATE TODAY, and C++ LIFETIME CONTROL — spawn the visual when fog
 *  rises, destroy it when fog expires — IS THE ONLY AVAILABLE SEAM. Corroborated from the other
 *  side: `list_variables` on `BP_SiegeFog` returned `[]`, so the visual holds no state either.
 *  ✅ THAT SEAM IS NOW **TAKEN**, ⛔ not merely available (`TASK-1068`, 2026-09-05): it is
 *  `RefreshFogVisual()` and the three writers that call it. ⭐ AND THE MEASUREMENT ABOVE IS WHAT
 *  CHOSE THE SHAPE — with no `UFUNCTION` to poll, a Blueprint-side "am I still up?" tick was
 *  never on the table, which is lucky: it would have put a SECOND opinion about fog liveness in
 *  an asset nobody reviews. ⚠️ If someone later adds a `UFUNCTION`, that does ⛔ NOT license one.
 *  ⛔ Read this as a measurement carrying a date: if someone later adds a `UFUNCTION`, this sentence
 *  EXPIRES rather than forbids. Until then it constrains every fog-visual row.
 *
 *  ⚖️✅ **RULED 2026-09-05 — THE QUESTION IS KEPT BECAUSE A RULING THAT ERASES ITS OWN QUESTION
 *  TEACHES NOBODY** (`FOG-§6a`, raised by `TASK-1050` cl. 4, ⛔ ANSWERED BY THE MANAGER on
 *  `TASK-1068` cl. 7).
 *  🚩 THE QUESTION, PRESERVED VERBATIM: is the TWO-OBJECT SPLIT (`AFogVolume` = C++ rules +
 *  lifetime · `BP_SiegeFog` = vendor visual) THE DESIGN, or is a real Blueprint child of
 *  `AFogVolume` MISSING?
 *  ⚖️ **THE ANSWER: THE TWO-OBJECT SPLIT IS THE DESIGN.** ⭐ THE GROUNDS ARE RECORDED SO IT CAN
 *  BE ARGUED WITH RATHER THAN MERELY OBEYED:
 *    (a) `BP_SiegeFog`'s ENTIRE VALUE **IS** its vendor parentage — the volumetric material, the
 *        box mesh and the noise all belong to `BP_FogArea`, and a Blueprint has ⛔ EXACTLY ONE
 *        parent ⇒ reparenting it to `AFogVolume` would ⛔ DISCARD THE VISUAL, which is the only
 *        thing it contributes;
 *    (b) `AFogVolume` must remain the ⛔ ONE state owner regardless, and a state owner that is
 *        ALSO a vendor visual is strictly harder to keep honest;
 *    (c) the split is exactly what makes *"the visual READS state, it never OWNS it"*
 *        enforceable ⛔ BY CONSTRUCTION rather than by review — the visual literally cannot hold
 *        a deadline, because it holds no variables at all.
 *  🧑 ⚠️ JONATHAN MAY OVERTURN THIS. It is the manager's ruling, ⛔ not his word, and the
 *  paragraph is left here (rather than deleted) so that overturning it starts from the argument
 *  instead of from scratch. ⚠️ It settles the SHAPE only: the `CoreRedirects` paragraph above
 *  still governs the hazard's STATUS, and the two answers are independent.
 *
 *  `FSiegeFogTuning` is deliberately NOT a member of this actor — see
 *  `handoffs/TASK-998-programmer.md` (its *"STILL NOT a serialized member"* section) for why,
 *  and for what that keeps open. ⛔ Named rather than left as *"the handoff"* (`TASK-1053`'s
 *  sweep): an unnamed cross-reference is a pointer the next reader cannot follow.
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
	 *  ⛔ The ONLY `SpawnActor<AFogVolume>` in the project. ⛔ CALLERS: ⛔ EVERY arm of
	 *  `USpellLibrary::ResolveSpell` that touches fog state comes through this door — today the
	 *  `FogCover` arm and the `FogClear` (`BrightSun`) arm, the latter through the WRITE door
	 *  ON PURPOSE because a pre-emptive `BrightSun` is legal with no fog up (`J-F17`) and may be
	 *  the first cast of the match. ⛔ Corrected 2026-09-05 by `TASK-1053`'s SUBJECT-shaped sweep:
	 *  this line named the `FogCover` arm ALONE — true when it was written, and it stopped being
	 *  true when `TASK-982` landed the second arm. A null world, or a spawn the world refuses,
	 *  answers nullptr
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
	 *
	 *  ⭐ THE VISUAL — `TASK-1068`, EXIT/ENTRY (i): the SUCCESS path calls `RefreshFogVisual()`
	 *  immediately after stamping the deadline, which spawns `BP_SiegeFog` and arms the expiry
	 *  wake-up. ⛔ The REFUSAL path calls ⛔ NOTHING, deliberately: *"nothing is written on this
	 *  path"* is a property this function's callers depend on, and a reconciler call there would
	 *  be a no-op that reads like an effect.
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
	 *
	 *  ⭐⭐ THE VISUAL — `TASK-1068`, EXIT (ii), and it is ⛔ REQUIRED RATHER THAN NICE: 🧑 his
	 *  `FogClear` card is LIVE. A visual that only cleared on natural EXPIRY would mean ⛔ PAYING
	 *  60 GOLD TO BURN OFF FOG THAT IS STILL ON SCREEN — the card would look broken while working
	 *  perfectly. The success path calls `RefreshFogVisual()` after zeroing the fog deadline, so
	 *  the box goes away in the same instant the mechanic says it did. ⛔ The `J-F18` refusal path
	 *  calls NOTHING: the stored expiry stays bit-identical, so there is nothing to reconcile.
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
	 *  ⭐ `TASK-1068` KEEPS THAT PROPERTY TOO: the visual's despawn is added ⛔ HERE, so
	 *  `ASiegeGameMode::PlayAgain` stays byte-unchanged AGAIN and still learns no fog policy —
	 *  it does not know a duration, a ceiling, a density, a window, ⛔ or an asset path.
	 *
	 *  ⭐⭐ THE VISUAL — `TASK-1068`, EXIT (iii). ⛔ MISS THIS AND A **FOG CORPSE SURVIVES INTO
	 *  THE NEXT MATCH**: the deadlines would be zeroed while a fully opaque box stayed in the
	 *  world with nothing left that would ever destroy it, i.e. a match-2 player blinded by
	 *  match-1 fog that the simulation believes is gone.
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
	 *  ⭐ CALLERS — ⛔ LIVE, ⛔ no longer merely boarded (`SC-§40` cl. 2): `TASK-989`'s
	 *  `Fog`-during-prevention refusal, and `TASK-991`'s sun-on-sun refusal (where this is the `X`
	 *  of "would reduce prevention from X to Y") — both in `SiegePlayerController.cpp`'s card-play
	 *  path. ⛔ Corrected 2026-09-05 by `TASK-1053`'s SUBJECT-shaped sweep: this block said the
	 *  callers were *"both boarded"* and that *"this row ships it with no caller of its own"* —
	 *  ⛔ true of the ROW that wrote it, ⛔ false of the FILE a reader is holding.
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

	//~ ─── ⭐⭐⭐ THE FOG **VISUAL** — `TASK-1068`, `SC-§36.1` instance 2 ─────────────────────
	//~ ⛔⛔ THE DEFECT THIS SECTION REPAIRS, NAMED SO IT IS NOT REPEATED: `BP_SiegeFog` shipped
	//~ CORRECT, integration-checked and COMMITTED (`ef2c901`) with ⛔ ZERO CALLERS. Every
	//~ `BP_SiegeFog` reference in all of `Source/` was ⛔ INSIDE A COMMENT and `L_Arena.umap`
	//~ held ⛔ ZERO occurrences ⇒ 🧑 he paid 50 gold, the army went 87.8% blind, and ⛔ NOTHING
	//~ APPEARED. ⭐ An asset with no caller is not a feature; it is a file.
	//~ ⛔ SPAWN AND DESPAWN ARE ⛔ ONE SEAM: a spawn-only build is ⛔ PERMANENT FOG, FOREVER,
	//~ with a green suite and nothing red anywhere — *"a seam that can be entered and not left
	//~ is half a seam"*.

	/**
	 *  ⭐⭐ THE **ONE** REFERENCE TO THE FOG VISUAL ASSET IN THE WHOLE PROJECT, and it is a
	 *  FUNCTION rather than a raw literal at a call site precisely so a test can hold it.
	 *
	 *  ⛔⛔ THE HAZARD IT IS SHAPED AGAINST: a hardcoded `/Game/` path is a CONTENT DEPENDENCY
	 *  ⛔ NO TEST CAN SEE BREAK. Rename the asset and every line still compiles, every test still
	 *  passes, and the fog silently stops appearing — which is ⛔ VERBATIM the failure this row
	 *  exists to repair, one layer up. ⇒ `Tests/SiegeFogVisualTest.cpp` calls THIS function and
	 *  asserts the package it names is really on disk, so the rename goes ⛔ RED.
	 *
	 *  ⛔ WHY NOT AN `EditDefaultsOnly` `TSoftClassPtr` (the obvious alternative, rejected on
	 *  measurement rather than taste): `EditDefaultsOnly` is ARCHETYPE-ONLY, `AFogVolume` has
	 *  ⛔ ZERO Blueprint children (measured — see the `CoreRedirects` paragraph), and the
	 *  per-property `Config` specifier is absent ⇒ ⛔ THERE IS NOWHERE FOR AN OVERRIDE TO LIVE.
	 *  It would have bought ⛔ nothing and ⛔ ADDED a `CoreRedirects` obligation to this class.
	 *  ⛔ WHY NOT A `Config` (`.ini`) PATH: an ini is a SECOND live wire no test reads and no
	 *  reviewer sees, on a class whose whole doctrine is one source of truth.
	 *  ⛔ WHY NOT DATA-DRIVEN (a `DT_Cards` column): the visual's lifetime is owned by the STATE,
	 *  not by the card — `ResetFog` and natural expiry have no card behind them at all — so a
	 *  card row would be the wrong owner for two of the three exits.
	 *
	 *  ⚠️ THE `_C` SUFFIX IS LOAD-BEARING: `/Game/Blueprints/BP_SiegeFog.BP_SiegeFog_C` is the
	 *  GENERATED CLASS. Without it the path resolves to the `UBlueprint` ASSET, which is not a
	 *  `UClass` and cannot be spawned — and it fails by returning null, ⛔ silently, which is why
	 *  the loader below logs at `Error` and the test pins the suffix.
	 */
	static const FSoftClassPath& FogVisualClassPath();

	/**
	 *  ⭐⭐ THE SPAWN TRANSFORM, ⛔ DERIVED — a pure static so the RELATION can be asserted
	 *  headlessly (the `BrightSunWindowSeconds` testability-seam precedent, followed on purpose).
	 *  ⛔ Plain C++ static, ⛔ NOT a `UFUNCTION`, ⛔ no defaulted parameters (`SC-§33`).
	 *
	 *  ⛔⛔ WHY THE BOX **OVERHANGS** THE ARENA, AND WHY A LITERAL HERE WOULD BE A DEFECT:
	 *  `TASK-841` §5.2 MEASURED it — sized to the arena EXACTLY, the fog was ⛔ NOT THERE AT THE
	 *  ARENA EDGE (mean luma `0.6249`, green grass and a crisp castle visible from
	 *  `(24000, 11000, 1200)`); with the overhang the same camera reads `0.6454`, a total
	 *  white-out. ⇒ ⛔ A PLAYER STANDING AT THE WALL WOULD HAVE BEEN THE ONLY ONE WHO COULD SEE.
	 *  The cause is the box mask's feather plus the froxel range: a camera near the boundary has
	 *  a near-field neighbourhood that is mostly OUTSIDE the dense core.
	 *
	 *  ⭐ WHAT IS DERIVED (`SC-§34`) — the arena half-extent comes from its ONE owner,
	 *  `USiegeScatterConfig::ArenaHalfExtent`, and the ground datum from `ArenaGroundReferenceZUU`
	 *  (`J-F13`). ⇒ resize the arena and the fog box follows; ⛔ a hand-typed `640` goes RED.
	 *  ⚠️ WHAT IS **NOT** DERIVABLE and is therefore a NAMED CONSTANT carrying its derivation
	 *  (⛔ never a bare number at a call site): the horizontal margin lives on a LEVEL ACTOR and
	 *  the ceiling is a TUNED CHOICE with no owner in code. Both are below.
	 */
	static FTransform FogVisualTransform(const FVector2D& ArenaHalfExtentUU, float GroundReferenceZUU);

	/**
	 *  ⭐⭐⭐ THE READBACK PREDICATE — *"did the engine actually give us the scale we asked for?"* —
	 *  and it is a pure static for exactly the reason `BrightSunWindowSeconds` and
	 *  `FogVisualTransform` are: ⛔ A COMPARISON WRITTEN INLINE AT THE CALL SITE IS A COMPARISON
	 *  ⛔ NO TEST CAN EXECUTE, and `TASK-1071` measured what an unexecutable check costs.
	 *
	 *  ⛔⛔⛔ THE DEFECT IT DETECTS, MEASURED (`TASK-1071` §1/§2) AND WORTH READING IN FULL,
	 *  BECAUSE 🧑 JONATHAN REPORTED *"NO FOG"* ⛔ THREE TIMES BEFORE IT WAS FOUND:
	 *  `SpawnFogVisual` asked for a scale of `(640, 360, 260)` and the engine handed back
	 *  `(20, 20, 5)` — the ⛔ VENDOR component template's ⛔ OWN scale, read live from
	 *  `/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C:Mesh_GEN_VARIABLE`.
	 *  ⚠️ THE MECHANISM, read from engine source rather than inferred: `BP_SiegeFog`'s root is the
	 *  ⛔ INHERITED SCS `Mesh` component and ⛔ NOT a native root, so
	 *  `AActor::PostSpawnInitialize`'s `FixupNativeActorComponents()` finds nothing and the root
	 *  transform is applied down the SCS path instead. `Actor.cpp:4360` — the ⛔ NON-DEFERRED spawn
	 *  path — calls `FinishSpawning(UserSpawnTransform, true)`, and `SCS_Node.cpp:147` then runs
	 *  `if (bIsDefaultTransform) { WorldTransform.SetScale3D(NewSceneComp->GetRelativeScale3D()); }`.
	 *  ⇒ his fog was a `2,000 × 2,000 × 500` uu slab `6,750` uu ⛔ ABOVE the field and ~`21,200` uu
	 *  from his hero, against `L_Arena`'s `6000` uu `VolumetricFogDistance` froxel grid — ⛔ IT
	 *  NEVER INTERSECTED THE GRID AT ANY CAMERA ANGLE, even looking straight up. Measured against a
	 *  zero-control at his own gameplay vantage: `+0.04 %` mean luma at the ACHIEVED scale (inside
	 *  the pixel noise floor) vs `+73 %` at the INTENDED one. ⛔ THAT IS NOT FAINT FOG; IT IS NO FOG.
	 *
	 *  ⚠️⚠️ THE TRAP FOR ANYONE *"SIMPLIFYING"* THE REPAIR, AND THE ARTIST CHECKED IT:
	 *  ⛔ `SpawnParams.TransformScaleMethod` ⛔ DOES NOT HELP. The `bIsDefaultTransform` block runs
	 *  ⛔ AFTER the `ESpawnActorScaleMethod` switch and overwrites the scale ⛔ REGARDLESS of which
	 *  method was chosen. `Tests/SiegeFogVisualTest.cpp` reds on that reach BY NAME.
	 *  ⚠️ AND WHY THE REPAIR IS AN EXPLICIT `SetActorScale3D` RATHER THAN `SpawnActorDeferred` +
	 *  `FinishSpawning` (which also dodges the clobber): the engine's own comment beside that line
	 *  says `bIsDefaultTransform` is ⛔ **`false` IN A COOKED BUILD**. ⇒ the deferred variant would
	 *  behave ⛔ DIFFERENTLY in the editor and in the packaged game, and this bug could come back
	 *  ⛔ ONLY IN THE SHIPPED PRODUCT, where nobody is looking. ⛔ The explicit set is correct in
	 *  ⛔ BOTH, and that symmetry is the whole reason for the form.
	 *
	 *  ⭐ IT IS A PREDICATE, ⛔ NOT AN ASSERT AND ⛔ NOT A FIX: the caller decides what a mismatch
	 *  means. Today the caller LOGS LOUDLY and carries on, because an art failure may ⛔ NEVER
	 *  refuse a 50-gold card or change one vision clamp.
	 *  ⛔ NaN on either side is a MISMATCH — a scale that cannot be compared has not been achieved.
	 */
	static bool FogVisualScaleMatches(const FVector& RequestedScale3D, const FVector& AchievedScale3D);

	/**
	 *  ⛔ THE MARGIN, AND IT IS **NOT** A DESIGN NUMBER — it is `L_Arena`'s `ExponentialHeightFog_0`
	 *  `VolumetricFogDistance`, read live as `6000` under `TASK-841` §3.2. That is the radius
	 *  within which the pack can render AT ALL, so it is exactly how far outside the play area the
	 *  dense core must start for a camera ON the boundary to be inside it.
	 *  ⛔ IT CANNOT BE DERIVED FROM CODE: its owner is an actor placed in a map this project may
	 *  ⛔ NEVER SAVE. ⇒ transcribed ONCE, here, with its source — ⛔ never at a call site.
	 *  ⚠️ WHAT FALSIFIES IT: anyone changing `VolumetricFogDistance` in `L_Arena`. The failure is
	 *  a CLEAR CORNER, and it is invisible from the centre of the map — which is where every
	 *  screenshot gets taken.
	 *
	 *  ⭐ IT IS ALSO THE **BELOW-GROUND DEPTH**, deliberately reusing one number rather than
	 *  inventing a second: the volume must start below the walk surface or a camera at ground
	 *  level sits on the box's own face, where the same feather thins it out.
	 */
	static constexpr float FogVisualHorizontalMarginUU = 6000.f;

	/**
	 *  ⛔ THE CEILING — a TUNED CHOICE with ⛔ NO OWNER IN CODE, `20000` uu above the ground
	 *  datum (`TASK-841` §2, read back live as the box's `max Z`). ⛔ Named rather than typed,
	 *  because a bare `20000` at a call site is the shape `SC-§34` exists to prevent.
	 *  ⚠️ CONSEQUENCE (`HIGH-§1`): this is the altitude above which the battlefield is clear.
	 *  ⛔ Lowered under the hero's reachable height, a player on a ×2 Watch Tower (~2,400 uu)
	 *  pokes out of the fog and sees the whole map while everyone below is blind — an
	 *  ⛔ ASYMMETRY the fog mechanic is explicitly not allowed to have. ⛔ Raised without limit it
	 *  costs froxel resolution for volume nobody can ever occupy.
	 */
	static constexpr float FogVisualCeilingAboveGroundUU = 20000.f;

	/**
	 *  ⛔ `/Engine/BasicShapes/Cube` is `100` uu on an edge, so a component scale of `1` spans
	 *  ±50. ⇒ `Scale = FullExtent / 100`. ⛔ An ENGINE fact, not a design one; it is named so the
	 *  division at the call site reads as a unit conversion rather than as a magic constant.
	 *  ⚠️ The mesh is INHERITED from the vendor `BP_FogArea` and cannot be overridden on the child
	 *  through MCP (`TASK-841` §2's declared tooling gap), which is exactly why the arena scale
	 *  rides the SPAWN TRANSFORM instead of living in the asset.
	 */
	static constexpr float FogVisualUnitCubeEdgeUU = 100.f;

	/**
	 *  ⛔ THE TOLERANCE `FogVisualScaleMatches` COMPARES WITH, and it is deliberately ⛔ TIGHT
	 *  rather than generous. The defect it exists to catch is a scale wrong by a factor of ⛔ 32
	 *  on X, ⛔ 18 on Y and ⛔ 52 on Z, so a loose epsilon buys ⛔ NOTHING and a tight one cannot
	 *  hide it — while a generous one would be the first place a future *"the test is flaky"*
	 *  edit went, and would quietly restore the exact silence this class just came out of.
	 *  ⚠️ It is an ABSOLUTE tolerance on a COMPONENT SCALE — ⛔ not a world distance, hence ⛔ no
	 *  `UU` suffix — sized only to absorb the float round-trip a scale takes through
	 *  `USceneComponent::UpdateComponentToWorld`. Against the smallest shipped axis it is a
	 *  relative slack of well under a thousandth of a percent.
	 */
	static constexpr float FogVisualScaleTolerance = 0.01f;

	/**
	 *  ⛔⛔ TEARDOWN — THE FOURTH WAY OUT OF FOGGED, AND IT IS NOT A STATE TRANSITION.
	 *  `TASK-1068` cl. 3a enumerates THREE exits from the fog STATE (expiry · `BrightSun` ·
	 *  `ResetFog`) and that enumeration is complete. This is not a fourth one: the state does not
	 *  change here, ⛔ THE STATE'S OWNER CEASES TO EXIST.
	 *  ⭐ WHY IT IS WORTH THE OVERRIDE EVEN THOUGH A WORLD TEARDOWN IS FREE: `EndPlay` also fires
	 *  for `Destroyed` and `LevelTransition`, where the WORLD SURVIVES. Without this, destroying
	 *  the state actor would leave the visual behind with ⛔ NOBODY LEFT HOLDING THE REFERENCE —
	 *  a fog box no code can ever find again, i.e. ⛔ permanent fog with a green suite, which is
	 *  the exact failure this row exists to prevent, arriving through the back door.
	 *  ⛔ It is ⛔ NOT relying on `SpawnParams.Owner`: UE does not cascade `Destroy()` to owned
	 *  actors, and building on that belief is how the orphan happens.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	 *  (`ASummonedUnit::UnitEngagementRadiusUU` in `SummonedUnit.h` — ⛔ LOCATE BY TEXT; the old
	 *  `SummonedUnit.h:895` pointer had drifted off the declaration by 2026-09-05, so `TASK-1053`'s
	 *  sweep replaced the line number with the symbol), and ⭐⭐⭐ `FOG-§9.11` retired the
	 *  notice==firing identity, so the
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
	 *  derivation is quoted here: `CONVENTIONS.md`'s ⛔ **Terrain** bullet (⛔ LOCATE BY TEXT —
	 *  the old `CONVENTIONS:131` pointer had already drifted off its line by 2026-09-05, so
	 *  `TASK-1053`'s sweep replaced the line number with the anchor) pins `SM_ArenaTerrain` as
	 *  *"placed at (0,0,0) it reproduces the old ArenaGround slab's WALK SURFACE at Z=0"*. That
	 *  walk surface IS the flat grass terrain of his sentence.
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

	//~ ─── ⭐⭐⭐ THE VISUAL'S LIFETIME — `TASK-1068` ──────────────────────────────────────────

	/**
	 *  ⭐⭐⭐ **THE ONE RECONCILER.** ⛔ It makes the WORLD agree with `IsFogActive()`, and it is
	 *  the ⛔ ONLY function in the project that spawns or destroys the fog visual.
	 *
	 *  ⛔⛔ IT READS STATE; IT NEVER OWNS IT. It stores ⛔ NO deadline, keeps ⛔ NO `bool`, and
	 *  ⛔ NEVER re-types `FogDurationSeconds` — its wake-up delay is
	 *  `FogActiveUntilTimeSeconds − now`, i.e. DERIVED from the one scalar every time.
	 *  ⭐ `IsFogActive()` stays the ⛔ ONE predicate: this function ASKS it rather than
	 *  remembering what it was told, which is what makes an early or late timer harmless.
	 *
	 *  ⭐⭐ WHY A RECONCILER RATHER THAN A "SPAWN HERE / DESPAWN THERE" PAIR — this is the
	 *  design decision of the row and it is written down so nobody "simplifies" it back:
	 *  ⛔ a pair has to be CORRECT AT EVERY CALL SITE and there are three of them plus a timer;
	 *  ⭐ a reconciler is IDEMPOTENT, so every writer of the deadline can call the SAME line
	 *  unconditionally, immediately after it writes, and a FOURTH writer added later gets the
	 *  visual right by copying one call rather than by understanding the mechanic.
	 *
	 *  ⛔ CALLERS — and this list is the whole safety argument, so it is exhaustive by
	 *  construction: the three functions that write either deadline (`RaiseFog`,
	 *  `ApplyBrightSun`, `ResetFog`) and the expiry wake-up. ⛔ There is ⛔ NO `Tick` and
	 *  ⛔ NO poll; `PrimaryActorTick.bCanEverTick` stays `false`.
	 */
	void RefreshFogVisual();

	/**
	 *  Loads `FogVisualClassPath()` and spawns it at `FogVisualTransform(...)`.
	 *  ⛔⛔ A MISSING OR FAILED CLASS LOAD IS **LOUD** — `Error`, naming the exact path AND the
	 *  function that produced it — ⛔ NEVER silently null. 🧑 The cards' own VFX spawn is
	 *  null-safe, and that is ⛔ PRECISELY why his missing spell VFX were INVISIBLE rather than
	 *  ERRORING (`TASK-1025`). ⛔ Do not reproduce that failure one layer up.
	 *  ⚖️⛔ AND THE BOUNDARY OF *"LOUD"*: ⛔ LOUD IN THE LOG, ⛔ NEVER IN THE GAMEPLAY. A visual
	 *  that fails to load must ⛔ NOT refuse the card, ⛔ not consume-and-abort, and ⛔ not change
	 *  one clamp — the fog MECHANIC keeps working with no visual, because an art failure may
	 *  ⛔ never brick a 50-gold card.
	 */
	void SpawnFogVisual();

	/**
	 *  Destroys the visual if one is up, and clears the handle. ⛔ Idempotent and safe to call
	 *  when nothing was ever spawned — which is why every exit can call it unconditionally.
	 */
	void DestroyFogVisual();

	/**
	 *  ⭐⭐ THE SPAWNED VISUAL — ⛔ **NOT A STATE DUPLICATE**, and the distinction is exactly the
	 *  one `FogVolume.h`'s *"do not add a companion `bool bFogActive`"* ban is about.
	 *  ⛔ A flag is a SECOND ANSWER to "is fog up?" that can disagree with the first. This is not
	 *  an answer to anything — it is ⛔ THE ACTOR'S OWN PRESENCE, the handle you need in order to
	 *  `Destroy()` the thing you spawned. ⭐ Nothing ever BRANCHES on fog state by reading it:
	 *  the only question asked of it is *"is there an actor to destroy / do I need to make one?"*,
	 *  and `IsFogActive()` remains the sole predicate above it.
	 *  ⛔ REQUIRED, not optional: `BP_SiegeFog` is ⛔ NOT an `AFogVolume` subclass (parent
	 *  `BP_FogArea_C`, read back LIVE under `TASK-1043`) ⇒ `TActorIterator<AFogVolume>` will
	 *  ⛔ NEVER see it and a `Find`-style sweep returns ⛔ NOTHING, ⛔ SILENTLY. ⇒ ⛔ HOLD the
	 *  reference; ⛔ do NOT re-find it later.
	 *  `Transient` for the same reason both deadlines are: per-match runtime state that must
	 *  never be written into a package.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Fog", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> FogVisualActor;

	/**
	 *  ⭐ THE EXPIRY WAKE-UP. ⛔ NOT a second deadline and ⛔ not a second source of truth — see
	 *  the one-way-door paragraph in the class doc, which states the whole argument. Its rate is
	 *  computed from `FogActiveUntilTimeSeconds` at every arming, it is read back by nobody, and
	 *  the function it calls re-asks `IsFogActive()` instead of trusting that it fired.
	 *  ⛔ It exists because natural expiry is the ⛔ ONE exit with ⛔ NO WRITER: the deadline
	 *  simply passes, and an actor does not despawn itself at an instant nobody reads.
	 */
	FTimerHandle FogVisualExpiryTimerHandle;
};
