// TASK-410 [LLM-1a] -- THE GO / NO-GO SPIKE HARNESS.
//
// ============================================================================
// THIS IS THROWAWAY CODE BY CONSTRUCTION. TASK-423 DELETES IT.
// ============================================================================
// Its ONLY job is to produce the six numbers that decide whether the AI
// Commander is built at all (approved plan, section "Do the spike before
// writing anything else"). It is deliberately NOT the production subsystem:
// no UGameInstanceSubsystem, no FRunnable, no tiering policy, no game-lane
// type, no Siegebound include. The plugin still knows nothing about Siegebound
// -- the snapshot text here is a hand-written FIXTURE, never a Capture() call,
// and the evaluation corpus is loaded BY PATH at runtime, never #included
// (CONVENTIONS section 11).
//
// Everything is registered as FAutoConsoleCommandWithWorldAndArgs in the
// Siege.Llama.* namespace, in this ONE plugin-private file. NEVER a
// UFUNCTION(exec) on a shipped class: USiegeCheatManager and
// ASiegePlayerController are untouched by this batch, which is what keeps the
// whole inference lane genuinely new-files-only (CONVENTIONS section 5).
//
// M8 DECLARATION DUTY: adds no replicated property, no new replicated class,
// no new relevancy tier.
//
// ---------------------------------------------------------------------------
// THE SIX BARS AND WHERE EACH ONE IS MEASURED
// ---------------------------------------------------------------------------
//   #1 frame-time hitch histogram (WORST + p99, NEVER the mean), three tiers
//                                              -> Siege.Llama.SpikeBench
//   #2 TTFT + wall clock for ~60 constrained tokens, warm prefix
//                                              -> Siege.Llama.SpikeBench
//   #3 KV-prefix reuse, prefill tokens turn 1 vs turn 2
//                                              -> Siege.Llama.SpikeBench
//   #4 peak VRAM + process RSS with the game at ITS peak
//                                              -> Siege.Llama.SpikeBench
//   #5 accuracy on the SEALED corpus, exact match on {intent,kinds,counts,where}
//                                              -> Siege.Llama.SpikeEval
//   #6 does a focused UEditableTextBox starve Enhanced Input of WASD?
//                                              -> TASK-411, NOT this file
//
// ---------------------------------------------------------------------------
// FIVE THINGS A READER OF THE NUMBERS MUST KNOW (all repeated in the handoff)
// ---------------------------------------------------------------------------
//  (a) INFERENCE RUNS ON A BACKGROUND THREAD AT TPri_BelowNormal, via
//      AsyncThread -- NOT an FRunnable class, and NOT on the game thread. A
//      game-thread-synchronous spike cannot measure bar #1 at all: it would
//      report ONE multi-second frame and manufacture a NO-GO that the shipped
//      design (a below-normal worker) would never have hit. The spec's "no
//      FRunnable" means "do not build the production worker"; it cannot mean
//      "make bar #1 meaningless".
//  (b) THE HITCH NUMBERS ARE REPORTED AGAINST A BASELINE WINDOW captured on
//      the SAME map, in the SAME session, with the model already resident and
//      NOTHING running. L_Arena with a fielded army hitches on its own; an
//      absolute number without that control is not evidence.
//  (c) VRAM IS SAMPLED FROM THE WORKER THREAD ONLY, at phase boundaries and
//      every SpikeVramSampleTokenStride decoded tokens. ggml_backend_dev_memory
//      is a driver query and reports DEVICE-WIDE free bytes, not this process's
//      usage -- so "peak VRAM" here is a FREE-VRAM LOW-WATER MARK for the whole
//      machine. The model's own footprint is the before/after-load delta, which
//      is logged separately. It is never sampled from the game thread, because
//      a per-frame driver query would pollute the very histogram in (b).
//      ⚠️ THE OFFLOAD DEVICE IS PINNED AND NAMED. This machine enumerates an
//      iGPU AND a dGPU, and llama's default is "all available devices" with a
//      LAYER SPLIT across them -- which would let the weights straddle both
//      cards while the sampler reported the free-VRAM mark of ONE of them, so
//      bar #4's "does it fit 8 GB" answer would be understated in the direction
//      that manufactures a PASS. EnsureModelLoaded therefore sets an explicit
//      one-entry `devices` list + LLAMA_SPLIT_MODE_NONE + main_gpu, prints EVERY
//      device before and after the load, and prints WHICH device every VRAM
//      figure is about (vram_dev=). Choose it with gpu=<index|name>.
//  (d) THE ROSTER, THE PLACES, ZONE B AND ZONE C ARE A FIXTURE, not live state.
//      The BYTE LAYOUT mirrors USiegeAssistantSnapshot exactly (that is what
//      makes bar #3 meaningful), but the values are hand-authored. THERE ARE TWO
//      OF THEM -- t0 and t1, the same board a few seconds apart -- because bar
//      #3 measured with ONE constant fixture only ever diverges in the `order:`
//      line at the very END of Zone C, which is a best case the shipped path can
//      never reach. The bench reports BOTH BOUNDS and prints where the KV reuse
//      point actually landed. See section 7a.
//  (e) THE GBNF AND THE ZONE-A FEW-SHOTS ARE TRANSCRIPTIONS of the landed
//      game-lane code, because the plugin may not include it. CONVENTIONS
//      section 9c makes the LANDED GRAMMAR the authority; if this file and
//      SiegeAssistantGrammar.cpp ever disagree, THIS FILE IS WRONG. QA diffs
//      them at TASK-412 -- Siege.Llama.SpikeGrammar exists to make that a diff
//      instead of a reading.

#include "CoreMinimal.h"

#include "SiegeLlamaLog.h"
#include "SiegeLlamaModule.h"
#include "SiegeLlamaSettings.h"

#include "Async/Async.h"
#include "Containers/StringConv.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/CoreDelegates.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include <excpt.h>
#endif

THIRD_PARTY_INCLUDES_START
#include "ggml-backend.h"
#include "llama.h"
THIRD_PARTY_INCLUDES_END

namespace SiegeLlamaSpike
{

// ===========================================================================
// 0. TUNABLES AND CONSTANTS
// ===========================================================================

/** Context size. CONVENTIONS section 10 pins ContextTokens = 2048. */
static constexpr int32 SpikeDefaultContextTokens = 2048;

/**
 *  Physical (micro) batch size. SMALL ON PURPOSE -- CONVENTIONS section 10 and
 *  the plan's risk list both name a small n_ubatch as the primary frame-time
 *  contention mitigation: prefill is submitted to the GPU in slices this big,
 *  so a large ubatch is exactly what produces a single multi-hundred-ms hitch.
 *  Exposed as ubatch= so bar #1 can be measured against it rather than assumed.
 */
static constexpr int32 SpikeDefaultUBatch = 64;

/** Logical batch ceiling. Must be >= n_ubatch; one prefill submits in ubatch slices. */
static constexpr int32 SpikeDefaultBatch = 512;

/** CONVENTIONS section 10: MaxOutputTokens = 96. */
static constexpr int32 SpikeDefaultMaxOutputTokens = 96;

/** CONVENTIONS section 10: HardTimeoutSeconds = 10.0 (the ABORT timeout, not the soft UX one). */
static constexpr double SpikeHardTimeoutSeconds = 10.0;

/** Frames of quiet control data captured before any inference starts. See note (b). */
static constexpr int32 SpikeDefaultBaselineFrames = 180;

/** Decoded tokens between worker-side VRAM queries. See note (c). */
static constexpr int32 SpikeVramSampleTokenStride = 8;

/** Ticker samples between process-RSS queries -- GetStats() every frame would pollute the histogram. */
static constexpr int32 SpikeRssSampleFrameStride = 30;

/** Hard ceiling on retained frame samples (roughly 10 minutes at 120 Hz). */
static constexpr int32 SpikeMaxFrameSamples = 72000;

/** CONVENTIONS section 9: SiegeAssistantMaxSelectionKinds. Transcribed -- the plugin may not include the game header. */
static constexpr int32 SpikeMaxSelectionKinds = 3;

/** CONVENTIONS section 1 / section 10: GrammarCountMax = 30, and it is NOT the live roster max. */
static constexpr int32 SpikeGrammarCountMin = 1;
static constexpr int32 SpikeGrammarCountMax = 30;

/**
 *  MIRRORS USiegeAssistantSnapshot::MaxSnapshotChars. Transcribed, because the
 *  plugin may not include the game header.
 *
 *  ⚠️ CORRECTED FROM MEASUREMENT 2026-08-03 (TASK-413): 1440 -> 1085, derived as
 *  400 tokens x the 2.71 chars/token this very command measured for Zone B + C.
 *  It was a bare literal in the SPIKE_TOKENS line before, and a bare literal is
 *  exactly what goes stale when the constant it mirrors moves -- the re-run would
 *  then have printed a `headroom` computed against a cap the game no longer uses.
 *  ⚠️ MIRROR DUTY: if the game-lane constant moves again, move this one.
 */
static constexpr int32 SpikeMaxSnapshotChars = 1085;

/**
 *  repeats= CEILING. GENEROUS BUT BOUNDED, for the same reason deadline= is:
 *  the argument exists so an experiment can be run long, but an unbounded value
 *  turns a typo into an hours-long job that looks like a hang. 25 repeats over
 *  the 25-row dev split is 625 generations, which is already a deliberate act.
 *
 *  ⚠️ AN OUT-OF-RANGE VALUE IS REJECTED LOUDLY AND FALLS BACK TO 1 -- IT IS
 *  NEVER CLAMPED QUIETLY. A silently clamped repeats= would produce a SINGLE
 *  pass that the operator believes was N passes, and reporting a single run as
 *  a repeat-satisfying measurement is precisely the failure CONVENTIONS section
 *  12h (THE REPEAT LAW) was written about -- it is how the 19/25 -> 20/25
 *  confusion happened. Same argument as deadline=, one lane over.
 */
static constexpr int32 SpikeMaxEvalRepeats = 25;

/** Every measurement line carries one of these tags so the whole run greps out of the log. */
static const TCHAR* const TagRun      = TEXT("SPIKE_RUN");
static const TCHAR* const TagLoad     = TEXT("SPIKE_LOAD");
static const TCHAR* const TagHitch    = TEXT("SPIKE_HITCH");
static const TCHAR* const TagLatency  = TEXT("SPIKE_LATENCY");
static const TCHAR* const TagPrefill  = TEXT("SPIKE_PREFILL");
static const TCHAR* const TagMem      = TEXT("SPIKE_MEM");
static const TCHAR* const TagTokens   = TEXT("SPIKE_TOKENS");
static const TCHAR* const TagEvalRow  = TEXT("SPIKE_EVAL_ROW");
static const TCHAR* const TagEvalScore = TEXT("SPIKE_EVAL_SCORE");
/**
 *  ⚠️ A SEPARATE TAG, NOT A NEW FIELD ON SPIKE_EVAL_SCORE, AND THAT IS THE POINT.
 *  Every line carrying this tag is emitted ONLY when repeats= > 1, so a reader
 *  grepping SPIKE_EVAL_SCORE out of a repeat run gets exactly the same line shape
 *  they have always got, N times, and the DISTRIBUTION is a separate stream they
 *  opt into. It also makes the additive claim checkable by grep: zero
 *  SPIKE_EVAL_REPEAT lines in a log == the default path ran.
 */
static const TCHAR* const TagEvalRepeat = TEXT("SPIKE_EVAL_REPEAT");
/**
 *  ⚠️ TWO MORE OPT-IN STREAMS, ON EXACTLY THE REASONING ABOVE. Both are emitted
 *  by Siege.Llama.SpikePrompt and ONLY when out= or ids= is actually typed, so a
 *  log taken from a command line that passes neither contains ZERO lines carrying
 *  either tag. That makes CONVENTIONS section 16's additive claim checkable by
 *  grep rather than by reading a diff -- which is the standard TASK-476 set for
 *  this file and the standard every later flag here is held to.
 */
static const TCHAR* const TagPromptOut = TEXT("SPIKE_PROMPT_OUT");
static const TCHAR* const TagPromptIds = TEXT("SPIKE_PROMPT_IDS");
static const TCHAR* const TagWarn     = TEXT("SPIKE_WARN");

// ===========================================================================
// 1. THE PROMPT FIXTURE -- ZONE A / ZONE B / ZONE C
// ===========================================================================
//
// ⚠️ THE BYTE LAYOUT IS TRANSCRIBED FROM THE LANDED
//    USiegeAssistantSnapshot::BuildZoneA / BuildZoneB / BuildZoneC
//    (Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp).
//    Line for line, in the same order, with the same trailing newlines. That
//    correspondence is what makes bar #3 (KV-prefix reuse) transferable to the
//    shipped path: measure a different layout and the number means nothing.
//
// ⚠️ THE SYNONYM BLOCK IS THE OUTPUT OF USiegeAssistantVocabulary's C++
//    CONSTRUCTOR DEFAULTS, run through BuildSynonymTable()'s normalisation
//    (per row: lower-cased, de-duped, canonical removed, sorted by string;
//    rows sorted by canonical; empty rows dropped). It is transcribed
//    VERBATIM AND UNIMPROVED, including the one entry QA has already ruled
//    against:
//
//      ⚠️ FLAG FOR THE READER OF BAR #5 -- `ancient_ground_near <- ancient
//         ground, ...` is present here because it is present on disk today.
//         qa/TASK-419.md WARN-6 orders TASK-421 to REMOVE it, because the bare
//         alias quietly resolves the exact ambiguity DEV-06 was built on. So
//         this fixture is PESSIMISTIC on those rows relative to what will ship.
//         It was left wrong on purpose: silently "fixing" the prompt in the
//         direction that raises my own score is precisely what the sealed
//         corpus exists to prevent. Once DA_AssistantVocabulary lands, re-run
//         with prompt=<path> holding the real BuildZoneA output.
//
// ⚠️ ZONE A IS DELIBERATELY UNBOUNDED (CONVENTIONS section 8, corrected from
//    measurement). The <= 400-token cap governs ZONE B + ZONE C ONLY. Trimming
//    Zone A would make bar #3 WORSE, not better -- it is the static prefix.

static void AppendZoneA(FString& Out)
{
	Out.Reserve(Out.Len() + 6144);

	Out += TEXT("[RULES]\n");
	Out += TEXT("Turn ONE Siegebound order into ONE JSON command. Output the JSON object only: no prose, no explanation.\n");
	Out += TEXT("\n");

	// THE MIRROR OF THE LANDED GBNF (CONVENTIONS section 9c). Key order
	// intent/who/where/when; the per-pair count key is "n", NOT "count" --
	// "count" is the GBNF RULE name and they differ. A few-shot the grammar
	// would reject is a BLOCKER, not a warning.
	Out += TEXT("schema (a command):\n");
	Out += TEXT("{\"intent\":INTENT,\"who\":WHO,\"where\":WHERE,\"when\":WHEN}\n");
	Out += TEXT("INTENT = send | guard | ambush | follow | charge | fallback | rally\n");
	Out += TEXT("WHO    = [{\"kind\":KIND,\"n\":COUNT}] with 1 to 3 entries, or \"all\", or \"none\"\n");
	Out += TEXT("KIND   = a unit symbol from roster in [FORCES]\n");
	Out += TEXT("COUNT  = 1 to 30, or \"all\"\n");
	Out += TEXT("WHERE  = a place symbol from places in [FORCES], or \"none\"\n");
	Out += TEXT("WHEN   = \"now\", or {\"kind\":KIND,\"at_least\":1 to 30}\n");
	Out += TEXT("\n");

	Out += TEXT("schema (a question, when the order cannot be translated):\n");
	Out += TEXT("{\"ask\":ASK}\n");
	Out += TEXT("ASK = which_unit | which_place | how_many | which_intent | unsupported\n");
	Out += TEXT("\n");

	Out += TEXT("intents:\n");
	Out += TEXT("send = move the selected units to a place\n");
	Out += TEXT("guard = station them at a place and hold it\n");
	Out += TEXT("ambush = station them at a place and let them chase kills\n");
	Out += TEXT("follow = they follow the hero\n");
	Out += TEXT("charge = whole army attacks; who and where are \"none\"\n");
	Out += TEXT("fallback = whole army defends home; who and where are \"none\"\n");
	Out += TEXT("rally = hero rallies units near him; who and where are \"none\"\n");
	Out += TEXT("\n");

	// The seven pinned place symbols and their descriptions, in the pinned
	// order (CONVENTIONS section 9a + the snapshot's PlaceVocabulary table).
	// IT IS own_castle, NOT my_castle -- the sealed HOLD-06 asserts it and the
	// holdout is frozen, so a drift here fails the gate for a NAMING reason
	// rather than a MODEL reason, which is the worst way to fail it.
	Out += TEXT("places (fixed vocabulary; only those listed in [FORCES] exist this match):\n");
	Out += TEXT("own_castle = the player's castle\n");
	Out += TEXT("enemy_castle = the enemy castle\n");
	Out += TEXT("mid = the capturable centre zone\n");
	Out += TEXT("ancient_ground_near = the ancient ground on the player's side\n");
	Out += TEXT("ancient_ground_far = the ancient ground on the enemy side\n");
	Out += TEXT("nearest_mine = the best gold mine for the player now\n");
	Out += TEXT("hero = where the player's hero stands\n");
	Out += TEXT("\n");

	Out += TEXT("rules:\n");
	Out += TEXT("- Symbols only. Never a coordinate, distance, direction or actor name.\n");
	Out += TEXT("- Ask for the count the player said even if the roster holds fewer; the game reports the shortfall.\n");
	Out += TEXT("- One order in, one command out. You never see an earlier turn.\n");
	Out += TEXT("- If the order is not one of the seven intents, return a question instead of guessing.\n");
	// The five accuracy-ladder rules, transcribed from the landed BuildZoneA at
	// ladder loop 2 (2 refusal, 1 selection/intent, 1 quantity, 1 place). Reasoning
	// lives in the shipped file: a MIRROR that argues its own case invites drift.
	Out += TEXT("- If the unit named is not a kind in [FORCES], answer {\"ask\":\"unsupported\"}. Never write a kind the player did not name.\n");
	Out += TEXT("- Gold, buying and card play are the player's, never yours: {\"ask\":\"unsupported\"}.\n");
	Out += TEXT("- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.\n");
	Out += TEXT("- No number said: a plural = \"all\", a singular = 1. Never copy a count from the roster.\n");
	Out += TEXT("- Choose a place by its noun - ancient ground, mine, castle, centre. near, nearest and far only say which one.\n");
	Out += TEXT("\n");

	Out += TEXT("synonyms:\n");
	Out += TEXT("SYNONYMS\n");
	Out += TEXT("[units]\n");
	Out += TEXT("archer <- archers, bowman, bowmen, bows, shooters\n");
	Out += TEXT("cavalry <- cav, horse, horseman, horsemen, rider, riders\n");
	Out += TEXT("cleric <- clerics, healer, healers, medic, priest, priests\n");
	Out += TEXT("footman <- foot, footmen, infantry, soldier, soldiers, swordsmen\n");
	Out += TEXT("knight <- heavies, heavy, knights\n");
	Out += TEXT("longbowman <- longbow, longbowmen, longbows\n");
	Out += TEXT("militiamob <- militia, mob, peasants, rabble\n");
	Out += TEXT("miner <- miners, worker, workers\n");
	Out += TEXT("ogre <- brute, brutes, giant, giants, ogres\n");
	Out += TEXT("pikeman <- pikemen, pikes, spearman, spearmen, spears\n");
	Out += TEXT("sapper <- bomber, bombers, demolition, sappers\n");
	Out += TEXT("sorcerer <- ritualist, ritualists, sorcerers\n");
	Out += TEXT("wizard <- wizards\n");
	Out += TEXT("[places]\n");
	Out += TEXT("ancient_ground_far <- far ancient ground, far runes, the far ground, their ancient ground\n");
	Out += TEXT("ancient_ground_near <- ancient ground, near ancient ground, near runes, nearest ancient ground, our ancient ground, the runes\n");
	Out += TEXT("enemy_castle <- enemy base, enemy castle, red castle, their base, their castle\n");
	Out += TEXT("hero <- my hero, my position, where i am\n");
	Out += TEXT("mid <- center, centre, middle, the capture zone, the middle\n");
	// The bare alias `gold` is GONE (see SiegeAssistantVocabulary.cpp): it mapped
	// an economy word onto a place symbol and is the mechanism by which an order
	// about spending gold came back as a mining order.
	Out += TEXT("nearest_mine <- gold mine, mine, the mine, the mines\n");
	Out += TEXT("own_castle <- base, home, my castle, our base, our castle, the keep\n");
	Out += TEXT("[intents]\n");
	Out += TEXT("ambush <- hide, lie in wait, set a trap, trap, waylay\n");
	Out += TEXT("charge <- all in, all out attack, everyone attack, full attack, push everything, rush\n");
	Out += TEXT("fallback <- everyone back, fall back, pull back, regroup, retreat\n");
	Out += TEXT("follow <- come with me, escort me, follow me, on me, stay with me, with me\n");
	Out += TEXT("guard <- garrison, hold, protect, station, watch\n");
	Out += TEXT("rally <- banner, call to arms, rally up\n");
	Out += TEXT("send <- advance, go, march, move, push, take\n");
	Out += TEXT("[notes]\n");
	Out += TEXT("wizard != sorcerer. wizard = ranged fire caster. sorcerer = ritualist, cannot attack, empowers friendlies on an ancient ground. mage / caster / spellcaster = ambiguous -> ask which_unit.\n");
	Out += TEXT("archer != longbowman. bow, bows, bowman, bowmen = archer. only a long- word = longbowman.\n");
	Out += TEXT("send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend = ambiguous between guard and fallback -> ask which_intent.\n");
	// Loop 2 DELETED the per-mine line and the 2nd sentence below (see the vocabulary).
	Out += TEXT("use only the place symbols listed in the state block.\n");
	Out += TEXT("\n");

	// ⚠️ THE SEVEN FEW-SHOTS ARE TRANSCRIBED FROM THE LANDED BuildZoneA AND ARE
	// DISJOINT FROM ALL THREE CORPUS FILES (CONVENTIONS section 11). The original
	// three were authored by TASK-416 before any eval ran and are NOT reactive.
	// ⚠️ THE FOUR NEW ONES ARE REACTIVE AND THAT IS DECLARED, NOT HIDDEN (section
	// 11's reactive-coverage clause): they were written against the four failure
	// MECHANISMS the spent generation-1 holdout and the dev split both showed.
	// They contain none of the burned surface forms and no corpus sentence.
	// ⚠️ ORDER IS LOAD-BEARING (two minimal pairs, refusals in the middle, block
	// ends on a command) -- see the shipped BuildZoneA for why. Do not resort.
	Out += TEXT("examples:\n");
	Out += TEXT("order: send ten footmen with a sorcerer to the ancient ground on our side\n");
	Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":10},{\"kind\":\"sorcerer\",\"n\":1}],\"where\":\"ancient_ground_near\",\"when\":\"now\"}\n");
	Out += TEXT("order: all archers guard the middle\n");
	Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":\"all\"}],\"where\":\"mid\",\"when\":\"now\"}\n");
	Out += TEXT("order: the archer guards our castle\n");
	Out += TEXT("{\"intent\":\"guard\",\"who\":[{\"kind\":\"archer\",\"n\":1}],\"where\":\"own_castle\",\"when\":\"now\"}\n");
	Out += TEXT("order: send werewolves to the middle\n");
	Out += TEXT("{\"ask\":\"unsupported\"}\n");
	Out += TEXT("order: get two more pikemen with our gold\n");
	Out += TEXT("{\"ask\":\"unsupported\"}\n");
	Out += TEXT("order: i want the footmen to rush\n");
	Out += TEXT("{\"intent\":\"send\",\"who\":[{\"kind\":\"footman\",\"n\":\"all\"}],\"where\":\"none\",\"when\":\"now\"}\n");
	Out += TEXT("order: everyone attack\n");
	Out += TEXT("{\"intent\":\"charge\",\"who\":\"none\",\"where\":\"none\",\"when\":\"now\"}\n");
}

/** One fixture roster row -- mirrors FSiegeAssistantRosterEntry's printed form, not its struct. */
struct FSpikeRosterRow
{
	const TCHAR* Kind;
	int32 Total;
	int32 Orderable;
	int32 Followable;
};

/**
 *  THE FIXTURE ROSTER.
 *
 *  ⚠️ WHY ALL THIRTEEN COMMANDABLE KINDS ARE PRESENT, AND WHY THAT DEVIATES
 *     FROM THE SHIPPED MaxRosterKinds = 8 -- THIS IS A DECLARED DEVIATION, NOT
 *     AN OVERSIGHT, AND IT IS ALSO A FINDING AGAINST THE GAME LANE:
 *
 *     USiegeAssistantSnapshot caps the PRINTED roster at 8 kinds and collapses
 *     the tail into `other_kinds: N kinds, M units`, while the GRAMMAR's `kind`
 *     alternatives come from GetUnitKinds(), which is NOT capped. On a board
 *     with more than 8 kinds alive, the sampler can therefore emit a symbol the
 *     prompt never showed the model -- the same silent Zone-A/grammar seam
 *     CONVENTIONS section 9c is about, one level down. Flagged for TASK-416, not
 *     fixed here (that constant is not mine to change).
 *
 *     For the spike the practical consequence is decisive: in DT_Cards row order
 *     the first eight kinds are Footman, Archer, Knight, Miner, MilitiaMob,
 *     Pikeman, Sapper, Cavalry -- which collapses Cleric, Ogre, Wizard AND
 *     Sorcerer, the four kinds six sealed dev rows are built on. Honouring the
 *     cap would convert six MODEL results into six HARNESS ARTEFACTS and read as
 *     "the model is bad" when the model was never asked. So all thirteen print.
 *
 *     ⚠️ RE-CHECKED AGAINST THE CORRECTED CAP (TASK-413, 1440 -> 1085): Zone B +
 *     Zone C measures 955 chars, so all thirteen kinds STILL FIT, with 130 chars
 *     of headroom instead of 485. The deviation survives the tightening and bar
 *     #5's six caster/ogre rows are still model results rather than artefacts --
 *     but the margin is now one busier order line wide, not four. If a future
 *     fixture grows, re-read Siege.Llama.SpikePrompt's headroom before assuming
 *     it still holds; the command prints it so the claim stays checkable rather
 *     than asserted.
 *
 *  Order = Docs/Data/cards.csv row order (what the shipped Capture() sorts to).
 *  Counts are hand-authored so the sealed corpus's stated situations are TRUE:
 *  footman is 8 so DEV-03's "you asked for 10 and 8 exist" is a real shortfall;
 *  knight is 3 so DEV-11's deferred trigger is not already satisfied.
 *  Orderable / followable follow the SHIPPED eligibility split -- Cleric is
 *  follow-only, Ogre and Sapper take no zone orders, Miner takes zone orders but
 *  does not auto-follow.
 */
static const FSpikeRosterRow SpikeRoster[] =
{
	{ TEXT("footman"),    8, 8, 8 },
	{ TEXT("archer"),     6, 6, 6 },
	{ TEXT("knight"),     3, 3, 3 },
	{ TEXT("miner"),      3, 3, 0 },
	{ TEXT("militiamob"), 4, 4, 4 },
	{ TEXT("pikeman"),    5, 5, 5 },
	{ TEXT("sapper"),     2, 0, 2 },
	{ TEXT("cavalry"),    4, 4, 4 },
	{ TEXT("longbowman"), 3, 3, 3 },
	{ TEXT("cleric"),     2, 0, 2 },
	{ TEXT("ogre"),       1, 0, 1 },
	{ TEXT("wizard"),     2, 2, 2 },
	{ TEXT("sorcerer"),   1, 1, 1 }
};

/**
 *  THE SAME BOARD A FEW SECONDS LATER -- the t1 roster.
 *
 *  ⚠️ WHY THIS EXISTS AT ALL (qa/TASK-412.md BLOCKER-1). With ONE constant
 *     fixture, turn 1 and turn 2 differ ONLY in the `order:` line, which is the
 *     LAST line of Zone C -- so the KV reuse point lands in Zone C's tail and
 *     bar #3 reports ~97-98 % against a shipped ~78-82 %, while bar #2's "warm"
 *     TTFT is measured hundreds of tokens short of what ships. That is a
 *     measurement the shipped path can never reach, and it errs in the
 *     flattering direction. This second fixture makes turn 2 diverge at the
 *     START OF ZONE B -- the shipped WORST case, where the whole snapshot moved
 *     between turns -- so the bench can report both bounds instead of one
 *     number that is only true of a paused game.
 *
 *  ⚠️ THE KIND SET IS IDENTICAL TO t0's, DELIBERATELY -- BUT THE REASON CHANGED
 *     AT TASK-478 AND THE OLD ONE IS GONE, SO IT IS RESTATED RATHER THAN LEFT
 *     TO ROT (CONVENTIONS section 22: a stale comment is believed over the code).
 *
 *     IT USED TO SAY: "the GBNF's `kind` alternatives are generated from
 *     SpikeRoster, so a kind in a fixture but not in SpikeRoster is a symbol the
 *     prompt shows and the sampler forbids." THAT MECHANISM NO LONGER EXISTS --
 *     BuildSpikeGrammar now derives `kind` from THE FIXTURE IT IS GENERATING FOR,
 *     so a fixture can no longer disagree with its own grammar.
 *
 *     THE REASON THAT SURVIVES IS t1's OWN JOB, and it is independent of the
 *     grammar: t1 is "the same board a few seconds later", so bar #3's worst case
 *     is about the NUMBERS moving between turns. If t1 also DROPPED a kind, turn 1
 *     and turn 2 would run under two DIFFERENT grammars and the bound would be
 *     measuring two changes at once -- a confound in the one bar the feature's
 *     latency budget is judged on. Only the NUMBERS move here: units died, a miner
 *     was lost, a wizard went down.
 *
 *     ⇒ The kinds are still CHECKED, not trusted -- VerifyFixtureKindParity()
 *     still rejects any kind that is not in SpikeRoster (the game's vocabulary),
 *     and the static_assert below still binds the two row counts.
 */
static const FSpikeRosterRow SpikeRosterT1[] =
{
	{ TEXT("footman"),    6, 6, 6 },
	{ TEXT("archer"),     5, 5, 5 },
	{ TEXT("knight"),     3, 3, 3 },
	{ TEXT("miner"),      2, 2, 0 },
	{ TEXT("militiamob"), 3, 3, 3 },
	{ TEXT("pikeman"),    5, 5, 5 },
	{ TEXT("sapper"),     2, 0, 2 },
	{ TEXT("cavalry"),    3, 3, 3 },
	{ TEXT("longbowman"), 3, 3, 3 },
	{ TEXT("cleric"),     2, 0, 2 },
	{ TEXT("ogre"),       1, 0, 1 },
	{ TEXT("wizard"),     1, 1, 1 },
	{ TEXT("sorcerer"),   1, 1, 1 }
};

// ⚠️ THE MESSAGE WAS REWRITTEN AT TASK-478 AND THE ASSERT WAS KEPT. Its old text
// ("the kinds the GBNF was generated from") named a mechanism that no longer
// exists -- the GBNF is now generated from each fixture. Its NEW reason is the one
// above: t0 and t1 must differ in NUMBERS ONLY, or bar #3's SHIPPED_WORST_CASE
// bound compares two turns taken under two different grammars.
//
// ⚠️ AND IT IS LOAD-BEARING FOR A SECOND THING NOW: it is what makes the seam
// readout below PROVABLY SILENT on t1 at COMPILE TIME rather than by my reading.
// (It binds the COUNT; VerifyFixtureKindParity binds the symbols.)
static_assert(UE_ARRAY_COUNT(SpikeRosterT1) == UE_ARRAY_COUNT(SpikeRoster),
	"t0 and t1 must differ in NUMBERS ONLY -- a t1 that dropped a kind would give turn 1 and turn 2 different grammars and confound bar #3.");

/** The seven pinned place symbols, in the pinned order (CONVENTIONS section 9a). */
static const TCHAR* const SpikePlaces[] =
{
	TEXT("own_castle"),
	TEXT("enemy_castle"),
	TEXT("mid"),
	TEXT("ancient_ground_near"),
	TEXT("ancient_ground_far"),
	TEXT("nearest_mine"),
	TEXT("hero")
};

/** Signed counts, so no loop below compares an int32 against an unsigned sizeof expression. */
static constexpr int32 SpikeRosterNum = static_cast<int32>(UE_ARRAY_COUNT(SpikeRoster));
static constexpr int32 SpikePlacesNum = static_cast<int32>(UE_ARRAY_COUNT(SpikePlaces));

/**
 *  ONE COMPLETE ZONE B + ZONE C WORLD STATE.
 *
 *  Every value is a BAND or a symbol, exactly as USiegeAssistantSnapshot emits
 *  them -- MidOwner is one of the landed BuildZoneB's four values
 *  (ours | theirs | neutral | none) and HeroState one of BuildZoneC's three
 *  (alive | down | none). Anything else here would be a byte the shipped path
 *  cannot produce, and bar #3 would be measuring a layout that does not exist.
 */
struct FSpikeWorldFixture
{
	const TCHAR* Label;

	// --- Zone B ---
	int32 OwnCastleHpBand;
	int32 EnemyCastleHpBand;
	const TCHAR* MidOwner;
	int32 GoldBand;

	// --- Zone C ---
	const FSpikeRosterRow* Roster;
	int32 RosterNum;
	int32 StanceFree;
	int32 StanceFollowing;
	int32 StanceHolding;
	int32 StanceAmbushing;
	const TCHAR* HeroState;
};

/**
 *  t0 -- THE SEALED CORPUS'S BOARD. ⚠️ ITS BYTES ARE FROZEN.
 *  The dev and holdout rows were authored against exactly these numbers
 *  (DEV-03's "you asked for 10 footmen and 8 exist" is a real shortfall only
 *  while footman is 8; DEV-11's deferred trigger is unsatisfied only while
 *  knight is 3), and QA has verified Zone B = 68 chars and Zone C = 887 chars
 *  against it. THE EVAL JOB MUST ALWAYS USE t0. Changing a number here silently
 *  rewrites the corpus's premises without touching the corpus.
 */
static const FSpikeWorldFixture SpikeFixtureT0 =
{
	TEXT("t0"),
	80, 60, TEXT("ours"), 120,
	SpikeRoster, SpikeRosterNum,
	20, 12, 8, 4,
	TEXT("alive")
};

/**
 *  t1 -- THE SAME BOARD A FEW SECONDS LATER. Used ONLY by the bench, and only to
 *  make turn 2 diverge at the START of Zone B the way the shipped path does
 *  (qa/TASK-412.md BLOCKER-1). Every Zone B key moved and the roster shrank, so
 *  the FIRST differing token is inside the first Zone B line -- there is no
 *  arrangement of a live board that diverges EARLIER than this, which is what
 *  makes it the worst-case bound.
 */
static const FSpikeWorldFixture SpikeFixtureT1 =
{
	TEXT("t1"),
	70, 45, TEXT("neutral"), 165,
	SpikeRosterT1, static_cast<int32>(UE_ARRAY_COUNT(SpikeRosterT1)),
	14, 15, 7, 2,
	TEXT("down")
};

/**
 *  THE FIXTURE REGISTRY -- ONE ROW PER SELECTABLE FIXTURE. ⚠️ THIS TABLE IS THE
 *  "CAPABILITY, NOT CONTENT" HALF OF TASK-478 MADE CONCRETE: Stage D lands tA-tF by
 *  adding ROWS HERE, not by growing another `bUseT1 ? A : B` ternary at every call
 *  site (which is how a two-fixture ternary becomes a six-fixture bug farm).
 *
 *  ⛔ ADDING A ROW IS NOT ENOUGH TO SCORE THE CORPUS AGAINST IT, AND THAT IS
 *     DELIBERATE. The eval lane hardcodes t0 and MUST keep doing so -- the sealed
 *     rows were authored against t0's numbers, so scoring them on another board
 *     silently rewrites the corpus's premises. See the comment in RunOneSplit.
 */
struct FSpikeFixtureEntry
{
	const TCHAR* Name;
	const FSpikeWorldFixture* Fixture;
};

static const FSpikeFixtureEntry SpikeFixtureRegistry[] =
{
	{ TEXT("t0"), &SpikeFixtureT0 },
	{ TEXT("t1"), &SpikeFixtureT1 }
};

/**
 *  Resolves a fixture= value. ⚠️ AN UNRECOGNISED NAME IS REPORTED, NEVER SILENTLY
 *  TREATED AS t0: a typo that quietly hands back the default is how an operator ends
 *  up believing they measured a board they never loaded. Same posture as ApplyTier's
 *  unknown-tier warning and repeats='s out-of-range refusal.
 *
 *  @param OutRecognised  false when Name was non-empty and matched nothing.
 *  @return the named fixture, or t0 when Name is empty or unmatched.
 */
static const FSpikeWorldFixture& ResolveSpikeFixture(const FString& Name, bool& OutRecognised)
{
	OutRecognised = true;

	if (Name.IsEmpty())
	{
		return SpikeFixtureT0;
	}

	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(SpikeFixtureRegistry)); ++Index)
	{
		if (Name.Equals(SpikeFixtureRegistry[Index].Name, ESearchCase::IgnoreCase))
		{
			return *SpikeFixtureRegistry[Index].Fixture;
		}
	}

	OutRecognised = false;
	return SpikeFixtureT0;
}

/** Every registered fixture name, for an error message that lists what WOULD have worked. */
static FString ListSpikeFixtureNames()
{
	FString Out;
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(SpikeFixtureRegistry)); ++Index)
	{
		if (!Out.IsEmpty())
		{
			Out += TEXT("|");
		}
		Out += SpikeFixtureRegistry[Index].Name;
	}
	return Out;
}

/**
 *  ⛔ SUBSET PARITY (TASK-478). RELAXED IN EXACTLY ONE DIRECTION; TIGHTENED IN
 *     THREE. ⚠️ A PARITY CHECK THAT PERMITS ANYTHING HAS BEEN REMOVED, NOT FIXED.
 *
 *  WHAT IT USED TO ENFORCE: RosterNum == 13 AND Roster[i].Kind == SpikeRoster[i]
 *  for every i. That is fixture-IS-SpikeRoster, not fixture-is-VALID -- and it is
 *  the structural reason the CONVENTIONS section 9c seam (Zone A demonstrates
 *  `footman`/`sorcerer`/`archer` on a board that may not hold them) was
 *  UNMEASURABLE: no legal fixture could ever omit a kind, so the seam could not be
 *  reproduced by the only instrument that exists.
 *
 *  WHAT IT ENFORCES NOW -- the fixture's kinds must be a SUBSET of SpikeRoster,
 *  in SpikeRoster's own relative order, with no repeats and at least one row:
 *
 *   (a) ⛔ EVERY KIND MUST EXIST IN SpikeRoster. THIS IS THE DIRECTION THAT MAY
 *       NEVER BE RELAXED, AND DERIVING THE GRAMMAR FROM THE FIXTURE MADE IT MORE
 *       LOAD-BEARING, NOT LESS. Before, SpikeRoster generated the grammar, so a
 *       typo'd fixture kind was a symbol the sampler FORBADE -- loud by the model
 *       failing every row that needed it. Now the fixture generates the grammar,
 *       so a typo'd kind would be ADDED to the grammar as a legal alternative: the
 *       sampler could emit a unit the game does not have, and the shipped executor
 *       is the only thing left between that and a valid-shaped wrong command.
 *       ⇒ SpikeRoster remains the VOCABULARY authority even though it is no longer
 *       the GRAMMAR authority, and this check is the whole of that guarantee.
 *   (b) NO DUPLICATES. The old index-identity check forbade them for free; a naive
 *       "is it in the list" subset test would not. A repeated kind emits a repeated
 *       GBNF alternative and a repeated `roster:` line in Zone C -- a board the
 *       shipped Capture() cannot produce.
 *   (c) SpikeRoster's RELATIVE ORDER IS KEPT (relaxed from index-identity, not
 *       dropped). ⚠️ This is deliberately stricter than "a subset": SpikeRoster's
 *       order IS Docs/Data/cards.csv row order, which is what the shipped
 *       USiegeAssistantSnapshot::Capture() sorts to. A fixture that reordered its
 *       rows would print a `roster:` block no live board can produce, so bar #3's
 *       byte-level prefix claims would be measuring a layout that does not ship.
 *       Declared as a judgement call in the handoff: the spec said "permit a
 *       subset", and a subset in ARBITRARY order is a different, larger relaxation.
 *   (d) AT LEAST ONE ROW. An empty roster generates `kind ::= ` with no
 *       alternatives; llama.cpp refuses the WHOLE grammar, llama_sampler_init_grammar
 *       returns NULL, and every generation then runs UNCONSTRAINED (the exact
 *       failure CONVENTIONS section 9c records as having reached a measurement run).
 *
 *  ⚠️ NOTE WHAT THIS CANNOT SEE, because a caller must not mistake it for the whole
 *     seam: it compares the fixture against SpikeRoster only. Whether ZONE A shows
 *     the model a kind the fixture omits is a DIFFERENT question, on a DIFFERENT
 *     artifact -- see ReportZoneAKindSeam below.
 *
 *  @return true when the fixture's kinds are a duplicate-free, in-order, non-empty
 *          subset of SpikeRoster.
 */
static bool VerifyFixtureKindParity(const FSpikeWorldFixture& Fixture, FString& OutFirstMismatch)
{
	if (Fixture.Roster == nullptr || Fixture.RosterNum <= 0)
	{
		OutFirstMismatch = FString::Printf(
			TEXT("the fixture carries %d roster rows -- an EMPTY roster generates `kind ::= ` with NO alternatives, llama.cpp refuses the whole grammar, and EVERY generation then runs UNCONSTRAINED"),
			Fixture.RosterNum);
		return false;
	}

	// Which SpikeRoster kinds this fixture has already claimed, so a DUPLICATE is
	// distinguished from an OUT-OF-ORDER row instead of the two sharing one message.
	bool bSeen[SpikeRosterNum] = {};
	int32 PreviousRosterIndex = INDEX_NONE;

	for (int32 RowIndex = 0; RowIndex < Fixture.RosterNum; ++RowIndex)
	{
		const TCHAR* const Kind = Fixture.Roster[RowIndex].Kind;

		int32 RosterIndex = INDEX_NONE;
		for (int32 Candidate = 0; Candidate < SpikeRosterNum; ++Candidate)
		{
			if (Kind != nullptr && FCString::Strcmp(Kind, SpikeRoster[Candidate].Kind) == 0)
			{
				RosterIndex = Candidate;
				break;
			}
		}

		if (RosterIndex == INDEX_NONE)
		{
			OutFirstMismatch = FString::Printf(
				TEXT("row %d names '%s', WHICH IS NOT A KIND IN SpikeRoster -- the grammar is generated from THIS fixture, so that symbol would become a legal GBNF alternative and the sampler could emit a unit the game does not have"),
				RowIndex, Kind != nullptr ? Kind : TEXT("<null>"));
			return false;
		}

		if (bSeen[RosterIndex])
		{
			OutFirstMismatch = FString::Printf(
				TEXT("row %d repeats the kind '%s' -- a duplicate emits a duplicate GBNF alternative and a duplicate `roster:` line, which no live board can produce"),
				RowIndex, SpikeRoster[RosterIndex].Kind);
			return false;
		}

		if (RosterIndex < PreviousRosterIndex)
		{
			OutFirstMismatch = FString::Printf(
				TEXT("row %d is '%s' (SpikeRoster index %d) but follows '%s' (index %d) -- a fixture may OMIT kinds, never REORDER them; SpikeRoster's order is cards.csv row order, which is what the shipped Capture() sorts to"),
				RowIndex, SpikeRoster[RosterIndex].Kind, RosterIndex,
				SpikeRoster[PreviousRosterIndex].Kind, PreviousRosterIndex);
			return false;
		}

		bSeen[RosterIndex] = true;
		PreviousRosterIndex = RosterIndex;
	}

	return true;
}

/** true when this fixture's roster names Kind. The grammar's `kind` alternatives are exactly these. */
static bool FixtureHasKind(const FSpikeWorldFixture& Fixture, const TCHAR* Kind)
{
	if (Kind == nullptr || Fixture.Roster == nullptr)
	{
		return false;
	}

	for (int32 RowIndex = 0; RowIndex < Fixture.RosterNum; ++RowIndex)
	{
		if (Fixture.Roster[RowIndex].Kind != nullptr
			&& FCString::Strcmp(Fixture.Roster[RowIndex].Kind, Kind) == 0)
		{
			return true;
		}
	}

	return false;
}

/**
 *  Collects the unit symbols a Zone A text DEMONSTRATES in a few-shot answer, by
 *  scanning for the literal `"kind":"` and reading to the closing quote.
 *
 *  ⚠️ SCANNED OUT OF THE ACTUAL TEXT RATHER THAN TRANSCRIBED INTO A SECOND LIST,
 *     AND THAT IS THE POINT (CONVENTIONS section 9c's own lesson: a method that
 *     compares an artifact to another artifact of the same authorship cannot detect
 *     an error they share). A hardcoded { footman, sorcerer, archer } would be a
 *     third copy of Zone A's content, free to drift from it silently -- and it would
 *     be flatly WRONG for a prompt= override, which is the case that matters most.
 *
 *  `"kind":"` appears in exactly two places in the landed schema: inside a `who`
 *  entry and inside the `when` at_least form. Both are demonstrations of a symbol
 *  the model is being taught to emit, which is what this is measuring.
 */
static void CollectZoneAFewShotKinds(const FString& ZoneAText, TArray<FString>& OutKinds)
{
	OutKinds.Reset();

	const FString KindKey = TEXT("\"kind\":\"");

	int32 SearchFrom = 0;
	while (SearchFrom < ZoneAText.Len())
	{
		const int32 KeyStart = ZoneAText.Find(KindKey, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
		if (KeyStart == INDEX_NONE)
		{
			break;
		}

		const int32 ValueStart = KeyStart + KindKey.Len();
		const int32 ValueEnd = ZoneAText.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, ValueStart);
		if (ValueEnd == INDEX_NONE)
		{
			break;
		}

		const FString Symbol = ZoneAText.Mid(ValueStart, ValueEnd - ValueStart);
		if (!Symbol.IsEmpty())
		{
			OutKinds.AddUnique(Symbol);
		}

		SearchFrom = ValueEnd + 1;
	}
}

/**
 *  ⭐ THE INSTRUMENT D3 WAS MISSING -- IT REPORTS THE ZONE A <-> GRAMMAR KIND SEAM
 *     FOR ONE FIXTURE, AND ITS WHOLE VALUE IS THAT THE SEAM STOPS BEING SILENT.
 *
 *  CONVENTIONS section 9c: "what makes it dangerous is an ABSENCE, not an event:
 *  THERE IS NO LOG LINE. Nothing throws, nothing refuses, nothing warns, and the
 *  JSON that does come back is well-formed... the cost arrives as diffuse accuracy
 *  loss with no cause attached, spread across rows that have nothing to do with the
 *  missing kind, and read as 'the model is bad'." That sentence is a specification
 *  for this function: on a fixture that omits a kind Zone A still teaches, the run
 *  now SAYS SO, so a score drop is attributable instead of mysterious.
 *
 *  ⛔ IT IS PROVABLY SILENT ON EVERY MEASUREMENT ON RECORD, AND THAT IS MECHANICAL,
 *     NOT A PROMISE (CONVENTIONS section 16 freeze #2 -- a new line on a default
 *     path moves the instrument every existing number was taken with):
 *       - it emits nothing unless some SpikeRoster kind that Zone A names is ABSENT
 *         from the fixture;
 *       - t0's Roster IS SpikeRoster, by construction, so nothing can be absent;
 *       - t1's row count is bound to SpikeRoster's BY static_assert, and its symbols
 *         by VerifyFixtureKindParity, so nothing can be absent there either.
 *     ⇒ On t0 and t1 -- the only fixtures that exist today -- this function cannot
 *     emit a line. It first speaks when a Stage D subset fixture arrives, which is
 *     exactly when D3 becomes measurable.
 *
 *  TWO SETS ARE REPORTED, because they are different strengths of evidence:
 *    NAMED       -- the symbol appears ANYWHERE in Zone A (the synonym table names
 *                   all thirteen kinds, so this is the wide set).
 *    DEMONSTRATED-- the symbol appears inside a few-shot ANSWER as `"kind":"X"`.
 *                   This is section 9c's actual claim and the stronger teaching
 *                   signal: the model is shown that token being EMITTED.
 *
 *  ⚠️ THE `NAMED` TEST IS A SUBSTRING TEST, STATED PLAINLY. No SpikeRoster kind is a
 *     substring of another, so the sets cannot cross-contaminate; but a Zone A that
 *     merely mentions a kind in prose counts as naming it. That is the intended
 *     reading -- the question is "is this symbol in front of the model" -- and it is
 *     a POSITIVE structural fact about the prompt bytes rather than a search for an
 *     absence (CONVENTIONS section 14).
 *
 *  @param bReportWhenClosed  true only on the inspection command, where "the seam is
 *                            CLOSED" is the result worth printing. The measurement
 *                            lanes pass false, so their default output cannot move.
 *  @return the number of SpikeRoster kinds Zone A names that this fixture forbids.
 */
static int32 ReportZoneAKindSeam(const FString& ZoneAText, const FSpikeWorldFixture& Fixture,
	const TCHAR* LaneLabel, bool bReportWhenClosed)
{
	TArray<FString> DemonstratedKinds;
	CollectZoneAFewShotKinds(ZoneAText, DemonstratedKinds);

	FString ForbiddenNamed;
	FString ForbiddenDemonstrated;
	int32 NamedCount = 0;
	int32 ForbiddenNamedCount = 0;
	int32 ForbiddenDemonstratedCount = 0;

	for (int32 Index = 0; Index < SpikeRosterNum; ++Index)
	{
		const TCHAR* const Kind = SpikeRoster[Index].Kind;
		if (!ZoneAText.Contains(Kind, ESearchCase::CaseSensitive))
		{
			continue;
		}

		++NamedCount;
		if (FixtureHasKind(Fixture, Kind))
		{
			continue;
		}

		++ForbiddenNamedCount;
		if (!ForbiddenNamed.IsEmpty())
		{
			ForbiddenNamed += TEXT(",");
		}
		ForbiddenNamed += Kind;
	}

	for (const FString& Symbol : DemonstratedKinds)
	{
		if (FixtureHasKind(Fixture, *Symbol))
		{
			continue;
		}

		++ForbiddenDemonstratedCount;
		if (!ForbiddenDemonstrated.IsEmpty())
		{
			ForbiddenDemonstrated += TEXT(",");
		}
		ForbiddenDemonstrated += Symbol;
	}

	if (ForbiddenNamedCount == 0 && ForbiddenDemonstratedCount == 0)
	{
		if (bReportWhenClosed)
		{
			UE_LOG(LogSiegeLlama, Display,
				TEXT("%s ZONE_A_KIND_SEAM lane=%s fixture=%s CLOSED -- every one of the %d SpikeRoster kinds this Zone A names, and all %d it DEMONSTRATES in a few-shot answer, are admissible in this fixture's grammar. (This is the positive control: the check ran and found nothing, which is a result, not a silence.)"),
				TagRun, LaneLabel, Fixture.Label, NamedCount, DemonstratedKinds.Num());
		}
		return 0;
	}

	UE_LOG(LogSiegeLlama, Warning,
		TEXT("%s: ZONE_A_KIND_SEAM OPEN lane=%s fixture=%s fixture_kinds=%d/%d zoneA_named=%d forbidden_named=%d [%s] forbidden_DEMONSTRATED=%d [%s] -- THIS RUN REPRODUCES THE CONVENTIONS section 9c SEAM, AND ON A SUBSET FIXTURE THAT IS BY CONSTRUCTION, NOT A DEFECT. Zone A shows the model these unit symbols while this fixture's GBNF forbids them, so constrained decoding fights the prompt on every token that reaches for one. A score taken here is a measurement OF THE SEAM (divergence D3), never of the model, and it may NOT be compared against a full-roster score as though the two differed only in board contents. The DEMONSTRATED set is the sharp one: those symbols appear inside a few-shot ANSWER, i.e. the model was shown that exact token being emitted."),
		TagWarn, LaneLabel, Fixture.Label, Fixture.RosterNum, SpikeRosterNum,
		NamedCount, ForbiddenNamedCount, ForbiddenNamed.IsEmpty() ? TEXT("none") : *ForbiddenNamed,
		ForbiddenDemonstratedCount, ForbiddenDemonstrated.IsEmpty() ? TEXT("none") : *ForbiddenDemonstrated);

	return ForbiddenNamedCount;
}

/** Zone B -- four fixed keys, always all four, always this order, every value a BAND. */
static void AppendZoneB(FString& Out, const FSpikeWorldFixture& Fixture)
{
	Out += TEXT("[MATCH]\n");
	Out.Appendf(TEXT("own_castle_hp: %d%%\n"), Fixture.OwnCastleHpBand);
	Out.Appendf(TEXT("enemy_castle_hp: %d%%\n"), Fixture.EnemyCastleHpBand);
	Out.Appendf(TEXT("mid: %s\n"), Fixture.MidOwner);
	Out.Appendf(TEXT("gold: %d\n"), Fixture.GoldBand);
}

/** Zone C -- fixed key order: places, roster, other_kinds, stances, hero, pending, order. */
static void AppendZoneC(FString& Out, const FSpikeWorldFixture& Fixture,
	const FString& Utterance, const FString& PendingLine)
{
	Out += TEXT("[FORCES]\n");
	Out += TEXT("places: ");
	for (int32 PlaceIndex = 0; PlaceIndex < SpikePlacesNum; ++PlaceIndex)
	{
		if (PlaceIndex > 0)
		{
			Out += TEXT(", ");
		}
		Out += SpikePlaces[PlaceIndex];
	}
	Out += TEXT("\n");

	Out += TEXT("roster:\n");
	for (int32 RowIndex = 0; RowIndex < Fixture.RosterNum; ++RowIndex)
	{
		const FSpikeRosterRow& Row = Fixture.Roster[RowIndex];
		Out.Appendf(TEXT("- %s: %d total, %d orderable, %d followable\n"),
			Row.Kind, Row.Total, Row.Orderable, Row.Followable);
	}

	// ALWAYS EMITTED, `none` when nothing was collapsed -- the fixed-key law.
	// Nothing collapses in EITHER fixture by construction (see the roster
	// comment): both print all thirteen kinds, so the tail is always empty.
	Out += TEXT("other_kinds: none\n");

	Out.Appendf(TEXT("stances: free %d, following %d, holding %d, ambushing %d\n"),
		Fixture.StanceFree, Fixture.StanceFollowing, Fixture.StanceHolding, Fixture.StanceAmbushing);
	Out.Appendf(TEXT("hero: %s\n"), Fixture.HeroState);

	// The pending line is GAME-AUTHORED: it is how a clarification turn carries
	// context forward WITHOUT ever feeding the model its own previous output
	// (CONVENTIONS section 1 -- a multi-turn model loop is a QA FAIL).
	Out.Appendf(TEXT("pending: %s\n"), PendingLine.IsEmpty() ? TEXT("none") : *PendingLine);

	Out += TEXT("[ORDER]\n");
	Out.Appendf(TEXT("order: %s\n"), Utterance.IsEmpty() ? TEXT("none") : *Utterance);
}

/** Flattens newlines and runs of whitespace -- the prompt layout is line-oriented, so a stray newline could forge a key. */
static FString SanitizeForPrompt(const FString& In)
{
	static constexpr int32 MaxUtteranceChars = 240;

	FString Out;
	Out.Reserve(FMath::Min(In.Len(), MaxUtteranceChars) + 1);

	bool bPreviousWasSpace = true;
	for (const TCHAR Character : In)
	{
		if (Out.Len() >= MaxUtteranceChars)
		{
			break;
		}

		const bool bIsSpace = (Character == TEXT('\n')) || (Character == TEXT('\r'))
			|| (Character == TEXT('\t')) || (Character == TEXT(' '));
		if (bIsSpace)
		{
			if (!bPreviousWasSpace)
			{
				Out.AppendChar(TEXT(' '));
				bPreviousWasSpace = true;
			}
			continue;
		}

		Out.AppendChar(Character);
		bPreviousWasSpace = false;
	}

	Out.TrimEndInline();
	return Out;
}

// ===========================================================================
// 2. THE GBNF -- A TRANSCRIPTION OF USiegeAssistantGrammar::Build
// ===========================================================================
//
// ⚠️ THE LANDED GRAMMAR IS THE AUTHORITY (CONVENTIONS section 9c). This is a
//    line-for-line mirror of SiegeAssistantGrammar.cpp for the FIXTURE's kinds
//    and places, written as a GENERATOR rather than a pasted string so the 30
//    `count` alternatives cannot be mistyped, and so QA diffs behaviour rather
//    than proof-reads a wall of quotes. Dump it with Siege.Llama.SpikeGrammar.
//
// ⚠️ `count` IS 1..30, NOT 1..live-max -- DELIBERATELY (CONVENTIONS section 1).
//    Capping it at the live roster would silently emit 8 for a request of 10 and
//    make the clarification UNDETECTABLE. A later task "tightening" it is
//    introducing the defect, not fixing one.

static FString GbnfTerminal(const FString& Chars)
{
	FString Escaped;
	Escaped.Reserve(Chars.Len() + 8);
	for (const TCHAR Char : Chars)
	{
		if (Char == TEXT('\\') || Char == TEXT('"'))
		{
			Escaped.AppendChar(TEXT('\\'));
		}
		Escaped.AppendChar(Char);
	}
	return FString(TEXT("\"")) + Escaped + TEXT("\"");
}

/** A GBNF terminal producing a JSON STRING -- two escaping layers, in this order. */
static FString GbnfJsonString(const FString& Value)
{
	FString JsonEscaped;
	JsonEscaped.Reserve(Value.Len() + 8);
	for (const TCHAR Char : Value)
	{
		if (Char == TEXT('\\') || Char == TEXT('"'))
		{
			JsonEscaped.AppendChar(TEXT('\\'));
		}
		JsonEscaped.AppendChar(Char);
	}
	return GbnfTerminal(FString(TEXT("\"")) + JsonEscaped + TEXT("\""));
}

static FString JsonObjectOpen(const TCHAR* Key)
{
	return GbnfTerminal(FString(TEXT("{\"")) + Key + TEXT("\":"));
}

static FString JsonNextKey(const TCHAR* Key)
{
	return GbnfTerminal(FString(TEXT(",\"")) + Key + TEXT("\":"));
}

// ⛔ THE GBNF RULE-NAME CHARSET. MIRRORS SiegeAssistantGrammar.cpp's
//    IsLegalGbnfRuleName / CollectRuleReferences / AppendRule -- and the reason
//    it is duplicated rather than shared is the same reason the generator is:
//    the plugin may not include the game header.
//
//    llama.cpp reads a rule name as a run of [a-zA-Z0-9-] and STOPS at the first
//    character outside it, so `at_least ::= ...` parses as the name `at`, the
//    parser then demands `::=`, finds `_least`, and REJECTS THE WHOLE GRAMMAR:
//
//        parse: error parsing grammar: expecting ::= at _least ::= "1" | "2" ...
//
//    ⚠️ THIS FILE SHIPPED THAT DEFECT AND SO DID THE GAME LANE. Every generation
//    of all six bench runs logged llama_sampler_init_grammar returning NULL, and
//    the model emitted <think> prose instead of JSON on every iteration -- which
//    is why bars #2 and #5 came back NOT MEASURED. The dump this file produces
//    was diffed against the shipped generator and blessed; the two matched
//    because THEY WERE IDENTICALLY UNPARSEABLE. A dump is not a parse, and
//    string-comparing two generators can never prove either one is valid.

static bool IsLegalGbnfRuleName(const FString& Identifier)
{
	if (Identifier.IsEmpty())
	{
		return false;
	}

	for (const TCHAR Char : Identifier)
	{
		const bool bLegal =
			(Char >= TEXT('a') && Char <= TEXT('z')) ||
			(Char >= TEXT('A') && Char <= TEXT('Z')) ||
			(Char >= TEXT('0') && Char <= TEXT('9')) ||
			Char == TEXT('-');

		if (!bLegal)
		{
			return false;
		}
	}

	return true;
}

/**
 *  Collects the rule REFERENCES from a right-hand side -- every identifier
 *  outside a terminal literal. The defect had TWO halves (`at_least` was a
 *  definition AND a reference), so checking only definitions would have left the
 *  reference to rot. Terminals are skipped as whole units, honouring the
 *  backslash escape, rather than splitting on whitespace -- a symbol is allowed
 *  to contain a space and a split would tear it in half and report the fragment.
 */
static void CollectRuleReferences(const FString& Rhs, TArray<FString>& OutReferences)
{
	FString Identifier;
	bool bInTerminal = false;
	bool bEscaped = false;

	for (const TCHAR Char : Rhs)
	{
		if (bInTerminal)
		{
			if (bEscaped)
			{
				bEscaped = false;
			}
			else if (Char == TEXT('\\'))
			{
				bEscaped = true;
			}
			else if (Char == TEXT('"'))
			{
				bInTerminal = false;
			}

			continue;
		}

		if (Char == TEXT('"'))
		{
			bInTerminal = true;
			continue;
		}

		const bool bIsSeparator = (Char == TEXT(' ')) || (Char == TEXT('\t')) || (Char == TEXT('|'));
		if (bIsSeparator)
		{
			if (!Identifier.IsEmpty())
			{
				OutReferences.Add(Identifier);
				Identifier.Reset();
			}

			continue;
		}

		Identifier.AppendChar(Char);
	}

	if (!Identifier.IsEmpty())
	{
		OutReferences.Add(Identifier);
	}
}

// Every rule name and every rule reference passes through here, which is what
// makes this the one place the charset can be enforced for the whole generator.
// The offending rule is still EMITTED, not dropped: llama.cpp refuses the
// grammar either way, and a faithful dump is what makes the failure diagnosable.
// Tagged SPIKE_WARN so it greps out of the run log beside the sampler warning it
// would otherwise be the invisible cause of.
static void AppendRule(FString& Grammar, const TCHAR* RuleName, const FString& Rhs)
{
	const bool bLegalRuleName = IsLegalGbnfRuleName(RuleName);
	if (!bLegalRuleName)
	{
		UE_LOG(LogSiegeLlama, Error,
			TEXT("%s: ILLEGAL GBNF RULE NAME '%s'. llama.cpp rule names are [a-zA-Z0-9-] only, so the WHOLE grammar will fail to parse and every generation will run UNCONSTRAINED. Rename the RULE to kebab-case; do NOT rename the JSON key it carries."),
			TagWarn, RuleName);
	}
	ensureAlwaysMsgf(bLegalRuleName, TEXT("Illegal GBNF rule name '%s' -- the grammar will not parse."), RuleName);

	TArray<FString> References;
	CollectRuleReferences(Rhs, References);

	for (const FString& Reference : References)
	{
		const bool bLegalReference = IsLegalGbnfRuleName(Reference);
		if (!bLegalReference)
		{
			UE_LOG(LogSiegeLlama, Error,
				TEXT("%s: rule '%s' REFERENCES the illegal identifier '%s'. llama.cpp rule names are [a-zA-Z0-9-] only, so the WHOLE grammar will fail to parse and every generation will run UNCONSTRAINED."),
				TagWarn, RuleName, *Reference);
		}
		ensureAlwaysMsgf(bLegalReference,
			TEXT("Illegal GBNF rule reference '%s' in rule '%s' -- the grammar will not parse."), *Reference, RuleName);
	}

	Grammar += RuleName;
	Grammar += TEXT(" ::= ");
	Grammar += Rhs;
	Grammar += TEXT("\n");
}

/**
 *  ⛔ THE `kind` ALTERNATIVES COME FROM THE FIXTURE, NOT FROM SpikeRoster (TASK-478).
 *
 *  ⚠️ THIS IS A CORRECTION TOWARD THE LANDED AUTHORITY, NOT A DIVERGENCE FROM IT.
 *     CONVENTIONS section 9c: "the landed grammar is the authority". The shipped
 *     USiegeAssistantGrammar::Build already takes the LIVE roster's kinds -- the
 *     spike was the one generating from a fixed thirteen-symbol list, so the two
 *     lanes disagreed on the one property section 1 calls the grounding
 *     ("a small model PHYSICALLY CANNOT name a unit that does not exist").
 *
 *  ⚠️ AND IT IS WHY DIVERGENCE D3 IS MEASURABLE AT ALL. With the alternatives pinned
 *     to all thirteen kinds, no fixture could express a board that LACKS a kind, so
 *     the seam where Zone A's few-shots name footman/sorcerer/archer on a board
 *     without them could not be reproduced by the only instrument that exists.
 *
 *  ⛔ WHAT DID **NOT** CHANGE, SAID EXPLICITLY BECAUSE IT IS THE ADJACENT MISTAKE:
 *     the identity/quantity split of CONVENTIONS section 1 is untouched. `count`
 *     and `at-least` still run SpikeGrammarCountMin..SpikeGrammarCountMax (1..30)
 *     and are NOT capped at the fixture's live totals. "Constrain identity hard,
 *     leave quantity soft" -- a grammar capped at the live 8 would silently emit 8
 *     for a player who asked for 10 and make the clarification UNDETECTABLE. Only
 *     the IDENTITY side (`kind`) is fixture-derived, which is exactly the side
 *     section 1 says to constrain hard.
 *
 *  ⚠️ t0 AND t1 BOTH CARRY ALL THIRTEEN KINDS IN SpikeRoster'S ORDER, so for every
 *     fixture that exists today this function returns a CHARACTER-IDENTICAL grammar
 *     to the one it returned before this change. No number on record moves.
 */
static FString BuildSpikeGrammar(const FSpikeWorldFixture& Fixture)
{
	FString Grammar;
	Grammar.Reserve(4096);

	AppendRule(Grammar, TEXT("root"), TEXT("command | question"));

	{
		TArray<FString> Parts;
		Parts.Add(JsonObjectOpen(TEXT("intent")));
		Parts.Add(TEXT("intent"));
		Parts.Add(JsonNextKey(TEXT("who")));
		Parts.Add(TEXT("who"));
		Parts.Add(JsonNextKey(TEXT("where")));
		Parts.Add(TEXT("where"));
		Parts.Add(JsonNextKey(TEXT("when")));
		Parts.Add(TEXT("when"));
		Parts.Add(GbnfTerminal(TEXT("}")));
		AppendRule(Grammar, TEXT("command"), FString::Join(Parts, TEXT(" ")));
	}

	{
		TArray<FString> Parts;
		Parts.Add(JsonObjectOpen(TEXT("ask")));
		Parts.Add(TEXT("ask"));
		Parts.Add(GbnfTerminal(TEXT("}")));
		AppendRule(Grammar, TEXT("question"), FString::Join(Parts, TEXT(" ")));
	}

	// The five SiegeAssistantAsk codes, in the fixed order the game lane emits.
	{
		const TCHAR* const AskCodes[] =
		{
			TEXT("which_unit"), TEXT("which_place"), TEXT("how_many"),
			TEXT("which_intent"), TEXT("unsupported")
		};

		TArray<FString> Alternatives;
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(AskCodes)); ++Index)
		{
			Alternatives.Add(GbnfJsonString(AskCodes[Index]));
		}
		AppendRule(Grammar, TEXT("ask"), FString::Join(Alternatives, TEXT(" | ")));
	}

	// The seven emittable intents, in ESiegeAssistantIntent declaration order
	// with None excluded (the game lane derives these by reflection).
	{
		const TCHAR* const IntentSymbols[] =
		{
			TEXT("send"), TEXT("guard"), TEXT("ambush"), TEXT("follow"),
			TEXT("charge"), TEXT("fallback"), TEXT("rally")
		};

		TArray<FString> Alternatives;
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(IntentSymbols)); ++Index)
		{
			Alternatives.Add(GbnfJsonString(IntentSymbols[Index]));
		}
		AppendRule(Grammar, TEXT("intent"), FString::Join(Alternatives, TEXT(" | ")));
	}

	// THE GROUNDING: every alternative is a unit that exists on THIS fixture's
	// board. Anything else -- "catapult", say -- is unreachable for the sampler.
	//
	// ⛔ AN EMPTY ROSTER IS REPORTED AND THEN EMITTED ANYWAY, WHICH IS THIS FILE'S
	//    ESTABLISHED IDIOM AND NOT AN OVERSIGHT (see AppendRule's illegal-rule-name
	//    path: "the offending rule is still EMITTED, not dropped: llama.cpp refuses
	//    the grammar either way, and a faithful dump is what makes the failure
	//    diagnosable"). ⚠️ Substituting SpikeRoster's full list here would be far
	//    worse than the crash it prevents: the run would proceed under a grammar
	//    NOBODY ASKED FOR and report a clean number for a board it never used --
	//    a confident green describing something else. VerifyFixtureKindParity
	//    rejects this fixture up front; this is the second line of defence, sited
	//    where the damage would actually be done.
	{
		if (Fixture.Roster == nullptr || Fixture.RosterNum <= 0)
		{
			UE_LOG(LogSiegeLlama, Error,
				TEXT("%s: fixture '%s' has %d roster rows, so `kind` is being emitted with NO alternatives. llama.cpp will REFUSE the whole grammar, llama_sampler_init_grammar will return NULL, and every generation in this run will be UNCONSTRAINED -- do not report any number from it as a constrained result."),
				TagWarn, Fixture.Label != nullptr ? Fixture.Label : TEXT("<unnamed>"), Fixture.RosterNum);
		}

		// The null test is repeated in the loop guard on purpose: the log above
		// REPORTS the condition, it does not prevent the dereference.
		const int32 KindCount = (Fixture.Roster != nullptr) ? FMath::Max(0, Fixture.RosterNum) : 0;

		TArray<FString> Alternatives;
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			Alternatives.Add(GbnfJsonString(Fixture.Roster[Index].Kind));
		}
		AppendRule(Grammar, TEXT("kind"), FString::Join(Alternatives, TEXT(" | ")));
	}

	// "none" is ALWAYS an alternative: the army-wide verbs have no destination.
	{
		TArray<FString> Alternatives;
		for (int32 Index = 0; Index < SpikePlacesNum; ++Index)
		{
			Alternatives.Add(GbnfJsonString(SpikePlaces[Index]));
		}
		Alternatives.Add(GbnfJsonString(TEXT("none")));
		AppendRule(Grammar, TEXT("where"), FString::Join(Alternatives, TEXT(" | ")));
	}

	{
		TArray<FString> Alternatives;
		for (int32 Quantity = SpikeGrammarCountMin; Quantity <= SpikeGrammarCountMax; ++Quantity)
		{
			Alternatives.Add(GbnfTerminal(FString::FromInt(Quantity)));
		}
		Alternatives.Add(GbnfJsonString(TEXT("all")));
		AppendRule(Grammar, TEXT("count"), FString::Join(Alternatives, TEXT(" | ")));
	}

	// Same numeric range as `count` but WITHOUT "all": "wait until I have all
	// footmen" is a condition that can never become true, so it is made
	// unsayable rather than left for the parser to reject.
	//
	// ⛔ THE RULE IS `at-least` (kebab-case, GBNF charset). THE JSON KEY IT
	//    CARRIES IS `at_least` (snake_case, wire format, asserted by the sealed
	//    corpus and by the game lane's parser). Neither may be "made consistent"
	//    with the other -- see IsLegalGbnfRuleName above.
	{
		TArray<FString> Alternatives;
		for (int32 Quantity = SpikeGrammarCountMin; Quantity <= SpikeGrammarCountMax; ++Quantity)
		{
			Alternatives.Add(GbnfTerminal(FString::FromInt(Quantity)));
		}
		AppendRule(Grammar, TEXT("at-least"), FString::Join(Alternatives, TEXT(" | ")));
	}

	{
		TArray<FString> Parts;
		Parts.Add(JsonObjectOpen(TEXT("kind")));
		Parts.Add(TEXT("kind"));
		Parts.Add(JsonNextKey(TEXT("n")));
		Parts.Add(TEXT("count"));
		Parts.Add(GbnfTerminal(TEXT("}")));
		AppendRule(Grammar, TEXT("item"), FString::Join(Parts, TEXT(" ")));
	}

	// ⚠️ THE CAP LIVES HERE, IN THE GRAMMAR -- a BOUNDED ALTERNATION of exactly
	// 1, 2 and 3 pairs, not a repetition rule. An unbounded repetition is
	// precisely what a small model rambles into, and the bounded form makes
	// "a 4-kind selection is unreachable" visible by reading the grammar.
	{
		TArray<FString> Alternatives;
		for (int32 ItemCount = 1; ItemCount <= SpikeMaxSelectionKinds; ++ItemCount)
		{
			TArray<FString> Parts;
			Parts.Add(GbnfTerminal(TEXT("[")));
			for (int32 Index = 0; Index < ItemCount; ++Index)
			{
				if (Index > 0)
				{
					Parts.Add(GbnfTerminal(TEXT(",")));
				}
				Parts.Add(TEXT("item"));
			}
			Parts.Add(GbnfTerminal(TEXT("]")));
			Alternatives.Add(FString::Join(Parts, TEXT(" ")));
		}
		AppendRule(Grammar, TEXT("selection"), FString::Join(Alternatives, TEXT(" | ")));
	}

	{
		TArray<FString> Alternatives;
		Alternatives.Add(TEXT("selection"));
		Alternatives.Add(GbnfJsonString(TEXT("all")));
		Alternatives.Add(GbnfJsonString(TEXT("none")));
		AppendRule(Grammar, TEXT("who"), FString::Join(Alternatives, TEXT(" | ")));
	}

	{
		TArray<FString> Alternatives;
		Alternatives.Add(GbnfJsonString(TEXT("now")));

		// ⛔ TWO DIFFERENT STRINGS ON PURPOSE, ONE LINE APART: the JSON KEY is
		//    "at_least" (wire format), the RULE REFERENCE is "at-least" (GBNF
		//    charset).
		TArray<FString> Parts;
		Parts.Add(JsonObjectOpen(TEXT("kind")));
		Parts.Add(TEXT("kind"));
		Parts.Add(JsonNextKey(TEXT("at_least")));
		Parts.Add(TEXT("at-least"));
		Parts.Add(GbnfTerminal(TEXT("}")));
		Alternatives.Add(FString::Join(Parts, TEXT(" ")));

		AppendRule(Grammar, TEXT("when"), FString::Join(Alternatives, TEXT(" | ")));
	}

	return Grammar;
}

// ===========================================================================
// 3. A MINIMAL READER FOR THE CONSTRAINED OUTPUT
// ===========================================================================
//
// ⚠️ WHY THIS IS NOT ParseSiegeAssistantCommand: that function lives in the GAME
//    module and the plugin is architecturally forbidden to include it. This is a
//    deliberately small mirror of the SHAPE ONLY -- it exists to score bar #5,
//    it is never the runtime path, and TASK-423 deletes it with the rest of the
//    file. It is strict on purpose: a shape it does not recognise is a row
//    failure, never a silent pass.

struct FSpikeParsed
{
	bool bIsQuestion = false;
	FString Ask;

	FString Intent;
	TArray<FString> Kinds;
	TArray<int32> Counts;   // 0 == "all"
	bool bWhoAll = false;
	bool bWhoNone = false;
	FString Where;          // the literal symbol, including "none"
	FString TriggerKind;    // empty == "now"
	int32 TriggerAtLeast = 0;
};

struct FSpikeJsonCursor
{
	const FString* Text = nullptr;
	int32 Pos = 0;

	bool AtEnd() const { return Pos >= Text->Len(); }
	TCHAR Peek() const { return AtEnd() ? TEXT('\0') : (*Text)[Pos]; }

	void SkipWhitespace()
	{
		while (!AtEnd())
		{
			const TCHAR Char = (*Text)[Pos];
			if (Char == TEXT(' ') || Char == TEXT('\t') || Char == TEXT('\n') || Char == TEXT('\r'))
			{
				++Pos;
				continue;
			}
			break;
		}
	}

	bool Take(TCHAR Expected)
	{
		SkipWhitespace();
		if (Peek() != Expected)
		{
			return false;
		}
		++Pos;
		return true;
	}
};

static bool SpikeReadString(FSpikeJsonCursor& Cursor, FString& Out)
{
	Cursor.SkipWhitespace();
	if (!Cursor.Take(TEXT('"')))
	{
		return false;
	}

	Out.Reset();
	while (!Cursor.AtEnd())
	{
		const TCHAR Char = (*Cursor.Text)[Cursor.Pos++];
		if (Char == TEXT('"'))
		{
			return true;
		}
		if (Char == TEXT('\\'))
		{
			if (Cursor.AtEnd())
			{
				return false;
			}
			Out.AppendChar((*Cursor.Text)[Cursor.Pos++]);
			continue;
		}
		Out.AppendChar(Char);
	}
	return false;
}

static bool SpikeReadInt(FSpikeJsonCursor& Cursor, int32& Out)
{
	Cursor.SkipWhitespace();

	const int32 Start = Cursor.Pos;
	while (!Cursor.AtEnd())
	{
		const TCHAR Char = (*Cursor.Text)[Cursor.Pos];
		if (Char >= TEXT('0') && Char <= TEXT('9'))
		{
			++Cursor.Pos;
			continue;
		}
		break;
	}

	if (Cursor.Pos == Start)
	{
		return false;
	}

	Out = FCString::Atoi(*Cursor.Text->Mid(Start, Cursor.Pos - Start));
	return true;
}

/** `count` is either a bare integer or the quoted sentinel "all" (which maps to 0). */
static bool SpikeReadCount(FSpikeJsonCursor& Cursor, int32& Out)
{
	Cursor.SkipWhitespace();
	if (Cursor.Peek() == TEXT('"'))
	{
		FString Word;
		if (!SpikeReadString(Cursor, Word) || !Word.Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			return false;
		}
		Out = 0;
		return true;
	}
	return SpikeReadInt(Cursor, Out);
}

static bool SpikeReadWho(FSpikeJsonCursor& Cursor, FSpikeParsed& Out, FString& OutError)
{
	Cursor.SkipWhitespace();

	if (Cursor.Peek() == TEXT('"'))
	{
		FString Word;
		if (!SpikeReadString(Cursor, Word))
		{
			OutError = TEXT("bad_who");
			return false;
		}
		if (Word.Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			Out.bWhoAll = true;
			return true;
		}
		if (Word.Equals(TEXT("none"), ESearchCase::IgnoreCase))
		{
			Out.bWhoNone = true;
			return true;
		}
		OutError = FString::Printf(TEXT("bad_who:%s"), *Word);
		return false;
	}

	if (!Cursor.Take(TEXT('[')))
	{
		OutError = TEXT("bad_who:not_array");
		return false;
	}

	for (;;)
	{
		if (!Cursor.Take(TEXT('{')))
		{
			OutError = TEXT("bad_item");
			return false;
		}

		FString Key;
		if (!SpikeReadString(Cursor, Key) || Key != TEXT("kind") || !Cursor.Take(TEXT(':')))
		{
			OutError = TEXT("bad_item:kind");
			return false;
		}

		FString Kind;
		if (!SpikeReadString(Cursor, Kind))
		{
			OutError = TEXT("bad_item:kind_value");
			return false;
		}

		if (!Cursor.Take(TEXT(',')) || !SpikeReadString(Cursor, Key) || Key != TEXT("n") || !Cursor.Take(TEXT(':')))
		{
			OutError = TEXT("bad_item:n");
			return false;
		}

		int32 Count = 0;
		if (!SpikeReadCount(Cursor, Count))
		{
			OutError = TEXT("bad_item:n_value");
			return false;
		}

		if (!Cursor.Take(TEXT('}')))
		{
			OutError = TEXT("bad_item:close");
			return false;
		}

		Out.Kinds.Add(Kind.ToLower());
		Out.Counts.Add(Count);

		if (Cursor.Take(TEXT(',')))
		{
			continue;
		}
		break;
	}

	if (!Cursor.Take(TEXT(']')))
	{
		OutError = TEXT("bad_who:close");
		return false;
	}

	// The cap is enforced here as well as in the grammar -- the parser must not
	// trust that the sampler was the only thing that produced this string.
	if (Out.Kinds.Num() < 1 || Out.Kinds.Num() > SpikeMaxSelectionKinds)
	{
		OutError = FString::Printf(TEXT("who_arity:%d"), Out.Kinds.Num());
		return false;
	}

	return true;
}

static bool SpikeReadWhen(FSpikeJsonCursor& Cursor, FSpikeParsed& Out, FString& OutError)
{
	Cursor.SkipWhitespace();

	if (Cursor.Peek() == TEXT('"'))
	{
		FString Word;
		if (!SpikeReadString(Cursor, Word) || !Word.Equals(TEXT("now"), ESearchCase::IgnoreCase))
		{
			OutError = TEXT("bad_when");
			return false;
		}
		return true;
	}

	if (!Cursor.Take(TEXT('{')))
	{
		OutError = TEXT("bad_when:shape");
		return false;
	}

	FString Key;
	if (!SpikeReadString(Cursor, Key) || Key != TEXT("kind") || !Cursor.Take(TEXT(':'))
		|| !SpikeReadString(Cursor, Out.TriggerKind))
	{
		OutError = TEXT("bad_when:kind");
		return false;
	}
	Out.TriggerKind = Out.TriggerKind.ToLower();

	if (!Cursor.Take(TEXT(',')) || !SpikeReadString(Cursor, Key) || Key != TEXT("at_least")
		|| !Cursor.Take(TEXT(':')) || !SpikeReadInt(Cursor, Out.TriggerAtLeast))
	{
		OutError = TEXT("bad_when:at_least");
		return false;
	}

	if (!Cursor.Take(TEXT('}')))
	{
		OutError = TEXT("bad_when:close");
		return false;
	}

	return true;
}

/**
 *  Key order is NOT enforced even though the grammar fixes it -- so that a
 *  grammar=0 control run still produces a scoreable number instead of a wall of
 *  parse failures that hide whether the grammar was doing any work.
 */
static bool ParseSpikeOutput(const FString& Json, FSpikeParsed& Out, FString& OutError)
{
	Out = FSpikeParsed();
	OutError.Reset();

	if (Json.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("empty_output");
		return false;
	}

	FSpikeJsonCursor Cursor;
	Cursor.Text = &Json;

	if (!Cursor.Take(TEXT('{')))
	{
		OutError = TEXT("malformed_json");
		return false;
	}

	bool bSawIntent = false;
	bool bSawWho = false;
	bool bSawWhere = false;
	bool bSawWhen = false;

	for (;;)
	{
		FString Key;
		if (!SpikeReadString(Cursor, Key) || !Cursor.Take(TEXT(':')))
		{
			OutError = TEXT("malformed_json:key");
			return false;
		}

		if (Key == TEXT("ask"))
		{
			if (!SpikeReadString(Cursor, Out.Ask))
			{
				OutError = TEXT("bad_ask");
				return false;
			}
			Out.bIsQuestion = true;
		}
		else if (Key == TEXT("intent"))
		{
			if (!SpikeReadString(Cursor, Out.Intent))
			{
				OutError = TEXT("bad_intent");
				return false;
			}
			Out.Intent = Out.Intent.ToLower();
			bSawIntent = true;
		}
		else if (Key == TEXT("who"))
		{
			if (!SpikeReadWho(Cursor, Out, OutError))
			{
				return false;
			}
			bSawWho = true;
		}
		else if (Key == TEXT("where"))
		{
			if (!SpikeReadString(Cursor, Out.Where))
			{
				OutError = TEXT("bad_where");
				return false;
			}
			Out.Where = Out.Where.ToLower();
			bSawWhere = true;
		}
		else if (Key == TEXT("when"))
		{
			if (!SpikeReadWhen(Cursor, Out, OutError))
			{
				return false;
			}
			bSawWhen = true;
		}
		else
		{
			OutError = FString::Printf(TEXT("unknown_key:%s"), *Key);
			return false;
		}

		if (Cursor.Take(TEXT(',')))
		{
			continue;
		}
		break;
	}

	if (!Cursor.Take(TEXT('}')))
	{
		OutError = TEXT("malformed_json:close");
		return false;
	}

	if (Out.bIsQuestion)
	{
		return true;
	}

	if (!bSawIntent || !bSawWho || !bSawWhere || !bSawWhen)
	{
		OutError = TEXT("missing_key");
		return false;
	}

	return true;
}

// ===========================================================================
// 4. THE SEALED CORPUS -- LOADED BY PATH, NEVER #included
// ===========================================================================
//
// ⚠️ CONVENTIONS section 11 + ruling 14. TASK-410 authored the RUNNER, never the
//    test set, and NEVER OPENED assistant_eval_holdout.csv. TASK-413 opens it
//    exactly once and the HOLDOUT number alone scores the >= 85 % bar. There is
//    deliberately NO default value for holdout= : dev= defaults to the dev file
//    so a quick loop is one keystroke, and the holdout must always be named
//    explicitly so it cannot be opened by muscle memory.
//
// ⚠️ INTENT LIVES IN `Notes`, NOT IN A COLUMN, AND THAT ENCODING IS PINNED. The
//    header has no ExpectIntent column; TASK-426 correctly refused to add a
//    10th one after the seal and prefixed every Notes cell instead. Extracted
//    with ^intent=([a-z]+); -- values are the 7 intents plus `none` (Refuse
//    rows) and `unasserted` (the verb itself is the ambiguity).
//
// ⚠️ AN EMPTY CELL IS "NOT ASSERTED", NEVER "ASSERTED EMPTY". It is scored as
//    SKIPPED. Treating empty as a pass would manufacture free score on exactly
//    the rows that assert least (`rally`, `fallback`). The per-row line prints
//    the assertion count so a reader can see how much each row actually tested.

static const TCHAR* const SpikeCorpusHeader =
	TEXT("Id,Sentence,ExpectOutcome,ExpectKinds,ExpectCounts,ExpectWhere,ExpectTriggerKind,ExpectTriggerAtLeast,Notes");

struct FCorpusRow
{
	FString Id;
	FString Sentence;
	FString ExpectOutcome;      // Execute | Clarify | Refuse
	FString ExpectWhere;
	FString ExpectTriggerKind;
	FString Notes;

	TArray<FString> ExpectKinds;
	TArray<int32> ExpectCounts;
	int32 ExpectTriggerAtLeast = 0;

	FString ExpectIntent;       // from ^intent=([a-z]+);
	bool bIntentAsserted = false;   // false for `unasserted`
	bool bKindsAsserted = false;
	bool bCountsAsserted = false;
	bool bWhereAsserted = false;
	bool bTriggerAsserted = false;

	int32 AssertedFieldCount() const
	{
		return (bIntentAsserted ? 1 : 0) + (bKindsAsserted ? 1 : 0)
			+ (bCountsAsserted ? 1 : 0) + (bWhereAsserted ? 1 : 0);
	}
};

/**
 *  RFC 4180 field splitter -- quoted fields, doubled quotes, embedded commas.
 *
 *  ⚠️ WRITTEN DEFENSIVELY ON PURPOSE. The dev file happens to contain no commas
 *     inside a cell, but the HOLDOUT COULD NOT BE INSPECTED (ruling 14), so the
 *     parser assumes nothing about it. A naive Split on ',' would have silently
 *     shifted every column of any quoted row -- the failure would have looked
 *     like a model error at the one moment nobody is allowed to re-open the file.
 */
static void SplitCsvLine(const FString& Line, TArray<FString>& OutFields)
{
	OutFields.Reset();

	FString Field;
	bool bInQuotes = false;

	for (int32 Index = 0; Index < Line.Len(); ++Index)
	{
		const TCHAR Char = Line[Index];

		if (bInQuotes)
		{
			if (Char == TEXT('"'))
			{
				if (Index + 1 < Line.Len() && Line[Index + 1] == TEXT('"'))
				{
					Field.AppendChar(TEXT('"'));
					++Index;
					continue;
				}
				bInQuotes = false;
				continue;
			}
			Field.AppendChar(Char);
			continue;
		}

		if (Char == TEXT('"'))
		{
			bInQuotes = true;
			continue;
		}
		if (Char == TEXT(','))
		{
			OutFields.Add(Field);
			Field.Reset();
			continue;
		}
		Field.AppendChar(Char);
	}

	OutFields.Add(Field);
}

/** Extracts the pinned `^intent=([a-z]+); ` prefix. Returns false when the prefix is absent. */
static bool ExtractIntentPrefix(const FString& Notes, FString& OutIntent)
{
	static const FString Prefix = TEXT("intent=");
	if (!Notes.StartsWith(Prefix, ESearchCase::CaseSensitive))
	{
		return false;
	}

	int32 Index = Prefix.Len();
	FString Value;
	while (Index < Notes.Len())
	{
		const TCHAR Char = Notes[Index];
		if (Char >= TEXT('a') && Char <= TEXT('z'))
		{
			Value.AppendChar(Char);
			++Index;
			continue;
		}
		break;
	}

	if (Value.IsEmpty() || Index >= Notes.Len() || Notes[Index] != TEXT(';'))
	{
		return false;
	}

	OutIntent = Value;
	return true;
}

static void SplitPipeList(const FString& In, TArray<FString>& Out)
{
	Out.Reset();
	if (In.TrimStartAndEnd().IsEmpty())
	{
		return;
	}
	In.ParseIntoArray(Out, TEXT("|"), false);
	for (FString& Entry : Out)
	{
		Entry = Entry.TrimStartAndEnd().ToLower();
	}
}

/**
 *  @return false only when the file could not be read at all. A structurally bad
 *          ROW is reported and skipped rather than aborting the run -- a run
 *          that dies on row 7 of the holdout, which may not be re-opened to
 *          diagnose, would be far worse than a run that scores 14 of 15 and says
 *          so loudly.
 */
static bool LoadCorpus(const FString& Path, TArray<FCorpusRow>& OutRows, FString& OutError)
{
	OutRows.Reset();

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *Path))
	{
		OutError = FString::Printf(TEXT("could not read '%s'"), *Path);
		return false;
	}

	if (Lines.Num() < 2)
	{
		OutError = FString::Printf(TEXT("'%s' has %d line(s) -- expected a header plus rows"), *Path, Lines.Num());
		return false;
	}

	const FString Header = Lines[0].TrimStartAndEnd();
	if (Header != SpikeCorpusHeader)
	{
		// Not fatal, but it is the single most likely cause of a wrong number, so
		// it is loud. The header is pinned character-for-character.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: corpus header MISMATCH in '%s'.\n  expected: %s\n  actual  : %s"),
			TagWarn, *Path, SpikeCorpusHeader, *Header);
	}

	int32 SkippedRows = 0;
	int32 MissingIntentPrefix = 0;

	for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
	{
		const FString& Line = Lines[LineIndex];
		if (Line.TrimStartAndEnd().IsEmpty())
		{
			continue;
		}

		TArray<FString> Fields;
		SplitCsvLine(Line, Fields);

		if (Fields.Num() < 9)
		{
			++SkippedRows;
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: '%s' line %d has %d field(s), expected 9 -- row SKIPPED (it is neither passed nor failed)."),
				TagWarn, *Path, LineIndex + 1, Fields.Num());
			continue;
		}

		FCorpusRow Row;
		Row.Id = Fields[0].TrimStartAndEnd();
		Row.Sentence = Fields[1].TrimStartAndEnd();
		Row.ExpectOutcome = Fields[2].TrimStartAndEnd();
		SplitPipeList(Fields[3], Row.ExpectKinds);

		TArray<FString> CountStrings;
		SplitPipeList(Fields[4], CountStrings);
		for (const FString& CountText : CountStrings)
		{
			Row.ExpectCounts.Add(FCString::Atoi(*CountText));
		}

		Row.ExpectWhere = Fields[5].TrimStartAndEnd().ToLower();
		Row.ExpectTriggerKind = Fields[6].TrimStartAndEnd().ToLower();

		const FString TriggerAtLeastText = Fields[7].TrimStartAndEnd();
		Row.ExpectTriggerAtLeast = FCString::Atoi(*TriggerAtLeastText);

		// Notes may itself have contained commas: rejoin the tail so the intent
		// prefix survives even if the row was written unquoted by hand.
		Row.Notes = Fields[8];
		for (int32 Extra = 9; Extra < Fields.Num(); ++Extra)
		{
			Row.Notes += TEXT(",");
			Row.Notes += Fields[Extra];
		}
		Row.Notes = Row.Notes.TrimStartAndEnd();

		Row.bKindsAsserted = Row.ExpectKinds.Num() > 0;
		Row.bCountsAsserted = Row.ExpectCounts.Num() > 0;
		Row.bWhereAsserted = !Row.ExpectWhere.IsEmpty();
		Row.bTriggerAsserted = !Row.ExpectTriggerKind.IsEmpty();

		if (Row.bKindsAsserted && Row.bCountsAsserted && Row.ExpectKinds.Num() != Row.ExpectCounts.Num())
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: %s asserts %d kind(s) but %d count(s) -- counts are index-aligned, so the count assertion is DROPPED for this row."),
				TagWarn, *Row.Id, Row.ExpectKinds.Num(), Row.ExpectCounts.Num());
			Row.bCountsAsserted = false;
		}

		FString IntentValue;
		if (ExtractIntentPrefix(Row.Notes, IntentValue))
		{
			Row.ExpectIntent = IntentValue;
			// `unasserted` = the verb itself is the ambiguity, so the intent
			// comparison is skipped for that row. `none` = a Refuse row and IS
			// asserted: it means "no command may be produced at all".
			Row.bIntentAsserted = (IntentValue != TEXT("unasserted"));
		}
		else
		{
			++MissingIntentPrefix;
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: %s has no '^intent=<value>; ' prefix in Notes -- intent is scored as NOT ASSERTED for this row (CONVENTIONS section 11)."),
				TagWarn, *Row.Id);
		}

		OutRows.Add(MoveTemp(Row));
	}

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s: corpus '%s' -- %d row(s) loaded, %d skipped, %d without an intent prefix, header %s."),
		TagRun, *FPaths::GetCleanFilename(Path), OutRows.Num(), SkippedRows, MissingIntentPrefix,
		(Header == SpikeCorpusHeader) ? TEXT("OK") : TEXT("MISMATCH"));

	return OutRows.Num() > 0;
}

// ===========================================================================
// 5. SCORING
// ===========================================================================
//
// ⚠️⚠️ THE ONE JUDGEMENT CALL IN THIS FILE, STATED IN FULL BECAUSE IT MOVES THE
//       NUMBER: WHAT DOES A `Clarify` ROW REQUIRE OF A SINGLE-TURN MODEL?
//
//    The corpus is internally inconsistent about this, and it is inconsistent
//    for a good reason -- `Clarify` is an OUTCOME OF THE PRODUCT, and the
//    product reaches it by two different routes:
//
//      route 1  the MODEL declines:   DEV-06 ("send the mage ...") -> {"ask":...}
//      route 2  the EXECUTOR objects: DEV-03 ("send 10 footmen" with 8 alive)
//                                     -> a FULL, CORRECT command, and the
//                                        shortfall is detected game-side. The
//                                        row's own note says so: "the parse is
//                                        correct and the EXECUTOR is what must
//                                        object".
//
//    So a `Clarify` row cannot demand one output form. The rule used here:
//
//      Execute  -> the output MUST be a command; every asserted field must match.
//      Refuse   -> the output MUST be a question (any ask code -- the corpus
//                  does not assert the taxonomy, so neither does the scorer).
//      Clarify  -> EITHER form is allowed. If a COMMAND was emitted, every
//                  asserted field must still match. If a QUESTION was emitted,
//                  the asserted fields are UNOBSERVABLE and are skipped.
//
//    ⚠️ THAT LAST CLAUSE IS LENIENT AND IT CAN BE GAMED, so the runner reports
//       a SECOND, STRICT number beside it in which a question only passes a
//       Clarify row that asserts NOTHING. It also prints how many Clarify rows
//       passed via a question. The inflation is bounded -- a model that answers
//       {"ask":...} to everything fails every Execute row, which is the majority
//       -- but "bounded" is not "absent", and both numbers go in the report so
//       nobody has to take my word for the bound.
//
//    The TRIGGER fields (deferred intent) are NOT part of bar #5's four fields.
//    They are compared and reported SEPARATELY so the deferred rows are visible
//    without quietly widening the bar.

struct FRowResult
{
	FString Id;
	FString Outcome;
	FString RawOutput;
	FString ParseError;

	bool bParsed = false;
	bool bIsQuestion = false;

	bool bOutcomeOk = false;
	bool bIntentOk = true;
	bool bKindsOk = true;
	bool bCountsOk = true;
	bool bWhereOk = true;
	bool bTriggerOk = true;

	bool bPassLenient = false;
	bool bPassStrict = false;
	bool bQuestionPassedAClarifyRow = false;

	int32 AssertionsCompared = 0;
	int32 AssertionsSkipped = 0;

	double WallMs = 0.0;
	double TtftMs = 0.0;
	int32 PrefillTokens = 0;
	int32 PrefixReused = 0;
	int32 OutputTokens = 0;
};

static bool KindSetMatches(const FCorpusRow& Row, const FSpikeParsed& Parsed, bool bCompareCounts)
{
	if (Row.ExpectKinds.Num() != Parsed.Kinds.Num())
	{
		return false;
	}

	// Order-insensitive over (kind, count) pairs. The kinds in one selection are
	// unique by invariant, and the order the model happens to list them in is not
	// a semantic difference -- punishing it would fail a correct answer.
	for (int32 ExpectedIndex = 0; ExpectedIndex < Row.ExpectKinds.Num(); ++ExpectedIndex)
	{
		const int32 FoundIndex = Parsed.Kinds.IndexOfByPredicate(
			[&Row, ExpectedIndex](const FString& Candidate)
			{
				return Candidate.Equals(Row.ExpectKinds[ExpectedIndex], ESearchCase::IgnoreCase);
			});

		if (FoundIndex == INDEX_NONE)
		{
			return false;
		}

		if (bCompareCounts)
		{
			if (!Row.ExpectCounts.IsValidIndex(ExpectedIndex) || !Parsed.Counts.IsValidIndex(FoundIndex))
			{
				return false;
			}
			if (Row.ExpectCounts[ExpectedIndex] != Parsed.Counts[FoundIndex])
			{
				return false;
			}
		}
	}

	return true;
}

static void ScoreRow(const FCorpusRow& Row, const FSpikeParsed& Parsed, bool bParsed, FRowResult& Result)
{
	Result.bParsed = bParsed;
	Result.bIsQuestion = bParsed && Parsed.bIsQuestion;

	if (!bParsed)
	{
		// An unparseable output fails every row shape. It is never "skipped".
		Result.bOutcomeOk = false;
		Result.bPassLenient = false;
		Result.bPassStrict = false;
		return;
	}

	const bool bWantsExecute = Row.ExpectOutcome.Equals(TEXT("Execute"), ESearchCase::IgnoreCase);
	const bool bWantsRefuse = Row.ExpectOutcome.Equals(TEXT("Refuse"), ESearchCase::IgnoreCase);
	const bool bWantsClarify = Row.ExpectOutcome.Equals(TEXT("Clarify"), ESearchCase::IgnoreCase);

	if (bWantsExecute)
	{
		Result.bOutcomeOk = !Parsed.bIsQuestion;
	}
	else if (bWantsRefuse)
	{
		Result.bOutcomeOk = Parsed.bIsQuestion;
	}
	else if (bWantsClarify)
	{
		Result.bOutcomeOk = true;
	}
	else
	{
		// An ExpectOutcome value outside the pinned three. Report, never guess.
		Result.bOutcomeOk = false;
		Result.ParseError = FString::Printf(TEXT("unknown_expect_outcome:%s"), *Row.ExpectOutcome);
	}

	// --- field comparison ---------------------------------------------------
	// Only over ASSERTED fields, and only when a COMMAND was produced. A question
	// carries none of them, so they are counted as SKIPPED, never as MATCHED.
	if (!Parsed.bIsQuestion)
	{
		if (Row.bIntentAsserted)
		{
			// intent=none on a command-shaped output is a fail by construction:
			// the row says no command may be produced at all.
			Result.bIntentOk = (Row.ExpectIntent != TEXT("none"))
				&& Parsed.Intent.Equals(Row.ExpectIntent, ESearchCase::IgnoreCase);
			++Result.AssertionsCompared;
		}
		else
		{
			++Result.AssertionsSkipped;
		}

		if (Row.bKindsAsserted)
		{
			// ⚠️ KINDS AND COUNTS ARE SCORED INDEPENDENTLY (qa/TASK-412.md WARN-4).
			// They used to share one bool, so a row where the model named the RIGHT
			// unit and the WRONG quantity printed `kinds=BAD counts=BAD` and sent
			// the remediation ladder after "it cannot name units" when the defect
			// was "it cannot count". The per-row diff is a TASK-413 deliverable, so
			// the column has to name the field that actually failed.
			//
			// PASS/FAIL IS UNCHANGED BY CONSTRUCTION: bFieldsOk is bKindsOk &&
			// bCountsOk, and KindSetMatches(counts=true) implies
			// KindSetMatches(counts=false), so the conjunction is the same value
			// the single call produced. Only the attribution moves.
			Result.bKindsOk = KindSetMatches(Row, Parsed, /*bCompareCounts*/ false);
			++Result.AssertionsCompared;
			if (Row.bCountsAsserted)
			{
				// A wrong KIND leaves no quantity to compare, so counts is BAD too --
				// that is a true statement about the answer, not a mis-attribution.
				Result.bCountsOk = KindSetMatches(Row, Parsed, /*bCompareCounts*/ true);
				++Result.AssertionsCompared;
			}
			else
			{
				++Result.AssertionsSkipped;
			}
		}
		else
		{
			Result.AssertionsSkipped += 2;
		}

		if (Row.bWhereAsserted)
		{
			Result.bWhereOk = Parsed.Where.Equals(Row.ExpectWhere, ESearchCase::IgnoreCase);
			++Result.AssertionsCompared;
		}
		else
		{
			++Result.AssertionsSkipped;
		}

		if (Row.bTriggerAsserted)
		{
			Result.bTriggerOk = Parsed.TriggerKind.Equals(Row.ExpectTriggerKind, ESearchCase::IgnoreCase)
				&& (Parsed.TriggerAtLeast == Row.ExpectTriggerAtLeast);
		}
	}
	else
	{
		Result.AssertionsSkipped += Row.AssertedFieldCount();
	}

	const bool bFieldsOk = Result.bIntentOk && Result.bKindsOk && Result.bCountsOk && Result.bWhereOk;

	Result.bPassLenient = Result.bOutcomeOk && bFieldsOk;

	// STRICT: a question may only pass a Clarify row that asserts nothing.
	if (bWantsClarify && Parsed.bIsQuestion)
	{
		Result.bQuestionPassedAClarifyRow = Result.bPassLenient;
		Result.bPassStrict = Result.bPassLenient && (Row.AssertedFieldCount() == 0);
	}
	else
	{
		Result.bPassStrict = Result.bPassLenient;
	}
}

// ===========================================================================
// 6. FRAME SAMPLING (GAME THREAD) AND THE HITCH HISTOGRAM
// ===========================================================================

enum class ESpikePhase : int32
{
	Idle = 0,
	Baseline,
	Load,
	Prefill,
	Decode,
	Count
};

static const TCHAR* PhaseName(ESpikePhase Phase)
{
	switch (Phase)
	{
		case ESpikePhase::Idle:     return TEXT("idle");
		case ESpikePhase::Baseline: return TEXT("baseline");
		case ESpikePhase::Load:     return TEXT("load");
		case ESpikePhase::Prefill:  return TEXT("prefill");
		case ESpikePhase::Decode:   return TEXT("decode");
		default:                    return TEXT("unknown");
	}
}

/** Bucket edges in milliseconds. 16.7 = 60 Hz, 33.3 = the bar #1 threshold. */
static const double SpikeHistogramEdgesMs[] = { 8.3, 11.1, 13.9, 16.7, 20.0, 25.0, 33.3, 50.0, 100.0 };
static constexpr int32 SpikeHistogramEdgeCount = static_cast<int32>(UE_ARRAY_COUNT(SpikeHistogramEdgesMs));
static constexpr int32 SpikeHistogramBucketCount = SpikeHistogramEdgeCount + 1;

/** The bar #1 threshold: "no frame > 33 ms attributable to inference on the partial tier". */
static constexpr double SpikeHitchThresholdMs = 33.3;

struct FFrameSample
{
	double DeltaMs = 0.0;
	int32 Phase = 0;
};

/**
 *  THE TWO PIECES OF CROSS-THREAD STATE, AND THEY ARE THE ONLY TWO.
 *  GCurrentPhase: written by the worker, read by the game-thread sampler.
 *  GBaselineFrameCount: written by the game-thread sampler, read by the worker
 *  while it waits for the control window. It exists SPECIFICALLY so the worker
 *  never touches GFrameSampler.Samples -- calling TArray::Num() on an array the
 *  game thread is appending to is a genuine data race, not a benign one.
 */
static FThreadSafeCounter GCurrentPhase(static_cast<int32>(ESpikePhase::Idle));
static FThreadSafeCounter GBaselineFrameCount(0);

/**
 *  Sampled on the GAME THREAD by an FTSTicker delegate, so what it records is
 *  the real frame period a player would feel. It deliberately does almost
 *  nothing per sample -- one FPlatformTime::Seconds() and one array append.
 *  Process RSS is read every SpikeRssSampleFrameStride frames, and DEVICE VRAM
 *  is never read here at all (see note (c) at the top of the file).
 */
struct FFrameSampler
{
	TArray<FFrameSample> Samples;
	FTSTicker::FDelegateHandle TickHandle;
	double LastTickSeconds = 0.0;
	int32 FrameCounter = 0;
	uint64 PeakRssBytes = 0;
	bool bActive = false;

	void Start()
	{
		check(IsInGameThread());
		if (bActive)
		{
			return;
		}

		Samples.Reset();
		Samples.Reserve(4096);
		LastTickSeconds = FPlatformTime::Seconds();
		FrameCounter = 0;
		PeakRssBytes = 0;
		GBaselineFrameCount.Set(0);
		bActive = true;

		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
			TEXT("SiegeLlamaSpikeFrameSampler"), 0.0f,
			[this](float /*UnusedDelta*/)
			{
				Tick();
				return true;
			});
	}

	void Stop()
	{
		check(IsInGameThread());
		if (!bActive)
		{
			return;
		}
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
		bActive = false;
	}

	void Tick();
};

static FFrameSampler GFrameSampler;

void FFrameSampler::Tick()
{
	const double Now = FPlatformTime::Seconds();
	const double DeltaMs = (Now - LastTickSeconds) * 1000.0;
	LastTickSeconds = Now;

	const int32 Phase = GCurrentPhase.GetValue();

	if (Samples.Num() < SpikeMaxFrameSamples)
	{
		FFrameSample Sample;
		Sample.DeltaMs = DeltaMs;
		Sample.Phase = Phase;
		Samples.Add(Sample);
	}

	if (Phase == static_cast<int32>(ESpikePhase::Baseline))
	{
		GBaselineFrameCount.Increment();
	}

	if ((++FrameCounter % SpikeRssSampleFrameStride) == 0)
	{
		const FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
		PeakRssBytes = FMath::Max(PeakRssBytes, static_cast<uint64>(Stats.UsedPhysical));
	}
}

struct FHistogram
{
	int32 Count = 0;
	double WorstMs = 0.0;
	double P99Ms = 0.0;
	double P95Ms = 0.0;
	double MedianMs = 0.0;
	double MeanMs = 0.0;
	int32 FramesOver33Ms = 0;
	int32 Buckets[SpikeHistogramBucketCount] = {};
};

static void BuildHistogram(const TArray<FFrameSample>& Samples, ESpikePhase Phase, FHistogram& Out)
{
	TArray<double> Values;
	Values.Reserve(Samples.Num());

	for (const FFrameSample& Sample : Samples)
	{
		if (Sample.Phase == static_cast<int32>(Phase))
		{
			Values.Add(Sample.DeltaMs);
		}
	}

	Out = FHistogram();
	Out.Count = Values.Num();
	if (Out.Count == 0)
	{
		return;
	}

	double Sum = 0.0;
	for (const double Value : Values)
	{
		Sum += Value;

		int32 Bucket = SpikeHistogramEdgeCount;
		for (int32 EdgeIndex = 0; EdgeIndex < SpikeHistogramEdgeCount; ++EdgeIndex)
		{
			if (Value < SpikeHistogramEdgesMs[EdgeIndex])
			{
				Bucket = EdgeIndex;
				break;
			}
		}
		++Out.Buckets[Bucket];

		if (Value > SpikeHitchThresholdMs)
		{
			++Out.FramesOver33Ms;
		}
	}

	Out.MeanMs = Sum / static_cast<double>(Out.Count);

	Values.Sort();
	Out.WorstMs = Values.Last();
	Out.MedianMs = Values[Out.Count / 2];
	Out.P95Ms = Values[FMath::Clamp(FMath::FloorToInt(0.95f * Out.Count), 0, Out.Count - 1)];
	Out.P99Ms = Values[FMath::Clamp(FMath::FloorToInt(0.99f * Out.Count), 0, Out.Count - 1)];
}

static void LogHistogram(ESpikePhase Phase, const FHistogram& Histogram)
{
	if (Histogram.Count == 0)
	{
		UE_LOG(LogSiegeLlama, Display, TEXT("%s phase=%s frames=0 (no samples)"), TagHitch, PhaseName(Phase));
		return;
	}

	// ⚠️ WORST AND p99 ARE THE BAR. The mean is printed LAST and labelled, because
	// the plan says "worst frame and p99, never the mean" -- a mean hides exactly
	// the one-frame stall that bar #1 exists to catch.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s phase=%-8s frames=%-5d WORST=%7.2fms p99=%7.2fms p95=%7.2fms median=%6.2fms over33ms=%d (mean=%.2fms -- NOT the bar)"),
		TagHitch, PhaseName(Phase), Histogram.Count,
		Histogram.WorstMs, Histogram.P99Ms, Histogram.P95Ms, Histogram.MedianMs,
		Histogram.FramesOver33Ms, Histogram.MeanMs);

	FString BucketLine;
	for (int32 BucketIndex = 0; BucketIndex < SpikeHistogramBucketCount; ++BucketIndex)
	{
		if (BucketIndex == 0)
		{
			BucketLine += FString::Printf(TEXT("<%.1f:%d "), SpikeHistogramEdgesMs[0], Histogram.Buckets[0]);
		}
		else if (BucketIndex == SpikeHistogramEdgeCount)
		{
			BucketLine += FString::Printf(TEXT(">=%.1f:%d"),
				SpikeHistogramEdgesMs[SpikeHistogramEdgeCount - 1], Histogram.Buckets[BucketIndex]);
		}
		else
		{
			BucketLine += FString::Printf(TEXT("%.1f-%.1f:%d "),
				SpikeHistogramEdgesMs[BucketIndex - 1], SpikeHistogramEdgesMs[BucketIndex], Histogram.Buckets[BucketIndex]);
		}
	}

	UE_LOG(LogSiegeLlama, Display, TEXT("%s phase=%-8s buckets(ms) %s"), TagHitch, PhaseName(Phase), *BucketLine);
}

// ===========================================================================
// 7. THE RUNNER -- MODEL, CONTEXT, SAMPLER, AND ONE BACKGROUND JOB AT A TIME
// ===========================================================================

enum class ESpikeJobKind : uint8
{
	Load,
	Bench,
	Eval
};

struct FSpikeOptions
{
	int32 GpuLayers = -1;           // -1 = full offload
	int32 ContextTokens = SpikeDefaultContextTokens;
	int32 UBatch = SpikeDefaultUBatch;
	int32 Threads = 0;              // 0 = the default below
	int32 Iterations = 5;
	int32 BaselineFrames = SpikeDefaultBaselineFrames;
	int32 MaxOutputTokens = SpikeDefaultMaxOutputTokens;

	/**
	 *  deadline= -- THE HARD ABORT CEILING, IN SECONDS, AND IT IS A DIAGNOSTIC
	 *  OVERRIDE RATHER THAN A RETUNE. The default is and stays
	 *  SpikeHardTimeoutSeconds = 10.0, which is CONVENTIONS section 10's shipped
	 *  HardTimeoutSeconds; nothing here moves that number.
	 *
	 *  ⚠️ WHY IT IS SETTABLE AT ALL (TASK-429, on Jonathan's "diagnose before
	 *  redefining the requirement" ruling). TASK-413's CPU tier aborted 2 of 5
	 *  generations with truncated JSON, and with a FIXED ceiling the logs cannot
	 *  tell "the CPU is slow and the deadline cut it" (a TUNABLE) apart from
	 *  "generation genuinely fails on CPU" (a WALL). Those two look identical in
	 *  the output and have opposite consequences for the CPU-fallback
	 *  requirement, so the ceiling has to be a variable before the question can
	 *  be measured at all.
	 *
	 *  ⚠️ AND IT IS ONLY AUTHORITATIVE ON THE CPU TIER. llama.h:382-385 documents
	 *  abort_callback as "currently works only with CPU execution", so on the
	 *  full/partial tiers this value can only fire BETWEEN graph submissions and
	 *  MaxOutputTokens is what actually bounds the run. Every line that prints
	 *  the deadline therefore prints WHICH REGIME it is in -- see
	 *  FormatDeadlineRegime. A reader who takes a GPU wall time as evidence about
	 *  this knob has misread the run, and that is exactly what the label prevents.
	 */
	double HardTimeoutSeconds = SpikeHardTimeoutSeconds;

	bool bUseGrammar = true;

	/**
	 *  chat=0|1 -- THE WIRE FORMAT. ⛔ DEFAULT 1, AND THE DEFAULT IS LOAD-BEARING:
	 *  every number on record (18 -> 19 -> 20, the 66.7 %) was taken templated.
	 *
	 *  ⚠️ DOCUMENTED AT TASK-477, NOT ADDED THERE -- THE FLAG ALREADY EXISTED, WAS
	 *  ALREADY PARSED IN THE SHARED ParseOptions (so it has always been live on
	 *  BOTH SpikeEval AND SpikePrompt), AND BuildPrompt ALREADY HONOURED IT. The
	 *  task spec asked for it to be added; re-adding it would have shipped a
	 *  duplicate flag, so it was VERIFIED against the mechanism instead and only
	 *  the missing documentation was written. This comment exists so the next
	 *  reader meets that history here rather than re-deriving it.
	 *
	 *  1 = the model's own chat template (Zone A as `system`, Zone B+C as `user`,
	 *      add_generation_prompt = true) -- see BuildPrompt.
	 *  0 = THE SHIPPED SHAPE: a raw Zone A + Zone B + Zone C concatenation with no
	 *      roles and no separator, which is byte-for-byte the assembly rule
	 *      USiegeAssistantComponent::ComposeTurnPrompt uses.
	 *
	 *  ⛔ AND THE TRAP THIS FLAG CANNOT SEE ON ITS OWN: BuildPrompt falls back to
	 *  raw concatenation when the loaded GGUF carries NO chat template, which is
	 *  SILENT. So chat= records an INTENTION; only the produced bytes record the
	 *  wire format. Siege.Llama.SpikePrompt therefore reports `wire=` read off the
	 *  bytes (SPIKE_PROMPT_OUT) instead of echoing this field back.
	 */
	bool bUseChatTemplate = true;
	FString TierLabel = TEXT("full");
	FString ModelPathOverride;
	FString ZoneAOverridePath;

	/**
	 *  gpu= -- WHICH DEVICE THIS TIER IS PINNED TO. Empty = "the first discrete
	 *  GPU, and say so". A ggml device INDEX (as printed on the SPIKE_MEM
	 *  stage=devices lines) or a device NAME. It is part of the reload key below,
	 *  because a tier that silently kept the previous device would report the
	 *  previous device's numbers.
	 */
	FString GpuDeviceSpec;

	/**
	 *  DERIVED, NEVER PARSED FROM ARGS. Set by the bench when grammar=0 so the
	 *  "fixed-length control" is genuinely fixed length (qa/TASK-412.md WARN-10).
	 *  The eval job never sets it: an accuracy run must stop at EOG.
	 */
	bool bIgnoreEogForFixedLength = false;

	FString DevCorpusPath;
	FString HoldoutCorpusPath;

	/**
	 *  repeats=<N> -- HOW MANY TIMES THE EVAL LOOPS EACH SPLIT **INSIDE ONE JOB**.
	 *
	 *  ⛔ THE DEFAULT IS 1 AND repeats=1 PRODUCES BYTE-IDENTICAL OUTPUT TO THE
	 *  COMMAND AS IT SHIPPED. Every SPIKE_EVAL_REPEAT line, every rep= field and
	 *  the whole distribution block are gated on N > 1. This is not politeness:
	 *  CONVENTIONS section 16 freezes SpikeEval's EXISTING output contract, and
	 *  "an edit that silently moves the instrument is worse than a deletion,
	 *  because a deletion is loud" -- every number on record (18 -> 19 -> 20, the
	 *  66.7 %) was taken with the old one and must stay comparable.
	 *
	 *  ⚠️ WHY IT EXISTS AT ALL (CONVENTIONS section 12h, THE REPEAT LAW): "one run
	 *  is not a measurement". Until now that law was satisfied by a HUMAN
	 *  re-typing the console command, which is exactly how the 19/25 -> 20/25
	 *  confusion happened -- and the harness correctly REFUSED the 3rd and 4th
	 *  attempt, because queue depth is 1 by design. So the only mechanical way to
	 *  take N readings is to loop INSIDE the single job, which is what this does.
	 *
	 *  ⛔ AND IT IS WHAT THE SEALED HOLDOUT STRUCTURALLY REQUIRES. Section 12h:
	 *  "the holdout is opened once; the repeat therefore happens INSIDE that
	 *  single opening." Without this flag section 10's gate clause #1 (">= 13/15
	 *  at the MINIMUM of 5 runs") cannot be taken at all without spending the seal
	 *  five times, which is spending it.
	 *
	 *  ⚠️ IT IS READ ONLY BY THE EVAL JOB. StartJob warns if a value > 1 reaches a
	 *  job kind that ignores it, so "I ran the bench five times" cannot be
	 *  believed silently.
	 */
	int32 EvalRepeats = 1;

	/**
	 *  out=<path> -- WRITE THE ASSEMBLED PROMPT'S EXACT BYTES TO A FILE.
	 *
	 *  ⛔ THIS EXISTS SO THE PYTHON TRAINER NEVER BECOMES A FOURTH LANE (trap T1).
	 *  There are already three lanes that disagree -- the spike, the shipped
	 *  composer, and what the documents say -- and a trainer that re-implements
	 *  Zone A bakes that skew into the weights, where it is invisible forever. The
	 *  consumer READS these bytes and asserts a length and a sha256 against them;
	 *  it never assembles a prompt of its own.
	 *
	 *  ⛔ EMPTY MEANS "NOT ASKED FOR", AND NOTHING IS WRITTEN. Read only by
	 *  Siege.Llama.SpikePrompt; StartJob warns if it reaches any other job kind.
	 */
	FString PromptOutPath;

	/**
	 *  ids=1 -- PRINT THE ENGINE'S TOKEN-ID SEQUENCE for the assembled prompt.
	 *
	 *  ⚠️ THIS IS THE **ENGINE HALF** OF M4 AND NOTHING MORE. The Python-side
	 *  comparison is a separate, later task and is deliberately not attempted here:
	 *  a tokenizer improvised inside this file to "check parity" would be the
	 *  fourth lane out= exists to prevent, wearing a different hat.
	 *
	 *  ⛔ THE FLAGS THE IDS WERE TAKEN UNDER ARE PRINTED BESIDE THEM. llama_tokenize
	 *  is called with add_special = true and parse_special = true (see
	 *  TokenizePrompt), and a Python count taken under different flags is a
	 *  different measurement -- comparing the two would manufacture a divergence,
	 *  or hide one.
	 */
	bool bDumpTokenIds = false;
};

/**
 *  ⚠️ THE DEADLINE, ITS PROVENANCE, AND -- THE LOAD-BEARING PART -- WHICH REGIME
 *  IT IS IN. Printed everywhere the deadline is mentioned, because the same
 *  number means two different things on two different tiers and the difference
 *  is documented in the vendored header rather than inferable from the logs:
 *
 *    llama.h:382-385, verbatim: "Abort callback / if it returns true, execution
 *    of llama_decode() will be aborted / currently works only with CPU
 *    execution."
 *
 *  So with gpulayers=0 (the cpu tier) the ceiling is a PROMISE -- the callback
 *  is polled inside llama_decode and cuts the run mid-graph. With any layer on
 *  the device it is ADVISORY: the callback can only be consulted between graph
 *  submissions, a submission already in flight runs to completion, and
 *  MaxOutputTokens is the real bound. CONVENTIONS section 8's footnote records
 *  this as "a promise on CPU, a request on GPU".
 *
 *  ⚠️ WITHOUT THIS LABEL the next reader compares a cpu wall time against a
 *  full-offload wall time taken at the same deadline= and concludes something
 *  false about the knob. That is the specific misreading this string exists to
 *  make impossible.
 */
static FString FormatDeadlineRegime(const FSpikeOptions& Options)
{
	const bool bOverridden = !FMath::IsNearlyEqual(Options.HardTimeoutSeconds, SpikeHardTimeoutSeconds, 1.e-6);
	const bool bCpuOnly = (Options.GpuLayers == 0);

	return FString::Printf(
		TEXT("deadline_s=%.1f(%s, %s)"),
		Options.HardTimeoutSeconds,
		bOverridden
			? TEXT("OVERRIDDEN by deadline= -- NOT the shipped 10.0s ceiling, do NOT compare this run's wall times against a default-deadline run without saying so")
			: TEXT("default = CONVENTIONS section 10 HardTimeoutSeconds"),
		bCpuOnly
			? TEXT("AUTHORITATIVE: gpulayers=0, so abort_callback aborts llama_decode mid-graph per llama.h:382-385")
			: TEXT("ADVISORY ONLY: layers are on the device and abort_callback is documented CPU-only, so this can fire only BETWEEN graph submissions -- tokens= is what actually bounds this run"));
}

struct FSpikeMemorySnapshot
{
	uint64 VramFreeBytes = 0;
	uint64 VramTotalBytes = 0;
	uint64 RssBytes = 0;

	/**
	 *  FALSE when no device was queried at all. Without it the bench printed
	 *  `vram_free_lowwater=0.0 MiB` on a machine with no GPU, which READS AS A
	 *  MEASUREMENT rather than as "not applicable" (qa/TASK-412.md NIT-3).
	 */
	bool bHasDevice = false;
};

/** All llama.cpp state. Touched ONLY from a spike worker thread, never the game thread. */
struct FSpikeRunner
{
	llama_model* Model = nullptr;
	llama_context* Context = nullptr;
	const llama_vocab* Vocab = nullptr;

	FSpikeOptions LoadedOptions;
	FString LoadedModelPath;
	int32 ModelLayerCount = 0;
	uint64 ModelSizeBytes = 0;

	/**
	 *  ⚠️ THE ONE DEVICE THIS TIER IS PINNED TO, AND THE ONLY DEVICE EVERY VRAM
	 *  FIGURE IN THIS RUN IS ABOUT. Set at load time from an explicit one-entry
	 *  `devices` list, so "which card is bar #4 talking about" is printed rather
	 *  than assumed (qa/TASK-412.md BLOCKER-2). nullptr = nothing was pinned and
	 *  the sampler is falling back to a guess, which every line then says.
	 */
	ggml_backend_dev_t OffloadDevice = nullptr;
	FString OffloadDeviceLabel = TEXT("<none>");
	bool bOffloadDeviceAssumed = true;

	/** The context's effective n_batch. THE PREFILL MUST BE SUBMITTED IN SLICES THIS BIG OR SMALLER. */
	int32 BatchSize = SpikeDefaultBatch;

	/**
	 *  The context's EFFECTIVE n_ctx, queried with llama_n_ctx after creation.
	 *  llama.h:551-556 says plainly that the requested value may differ from the
	 *  one actually used; every budget check below uses THIS, not the request.
	 */
	int32 ContextSize = SpikeDefaultContextTokens;

	/** The prompt tokens of the previous turn -- the basis of the KV-prefix reuse measurement. */
	TArray<llama_token> LastPromptTokens;

	/** Deadline for the abort_callback. 0 = no deadline armed. */
	double AbortDeadlineSeconds = 0.0;

	bool IsLoaded() const { return Model != nullptr && Context != nullptr; }
};

static FSpikeRunner GRunner;
static FThreadSafeBool GJobInFlight(false);
static FThreadSafeBool GJobFinished(false);
static FString GLastWorldName;
static bool bShutdownHookRegistered = false;

/**
 *  ⚠️ THE HARD-TIMEOUT MECHANISM, AND ITS DOCUMENTED LIMIT.
 *  llama.h states plainly that abort_callback "currently works only with CPU
 *  execution". So on the full/partial GPU tiers this deadline can only fire
 *  BETWEEN graph submissions, not inside one. The token budget is what actually
 *  bounds a GPU run. Recorded rather than discovered later by TASK-423.
 */
static bool SpikeAbortCallback(void* /*UserData*/)
{
	if (GRunner.AbortDeadlineSeconds <= 0.0)
	{
		return false;
	}
	return FPlatformTime::Seconds() > GRunner.AbortDeadlineSeconds;
}

// ---------------------------------------------------------------------------
// 7-0. DEVICE ENUMERATION AND PINNING (bar #4's subject)
// ---------------------------------------------------------------------------
//
// ⚠️ WHY ANY OF THIS EXISTS (qa/TASK-412.md BLOCKER-2). llama_model_params
//    documents `devices` as "NULL-terminated list of devices to use for
//    offloading (if NULL, all available devices are used)" (llama.h:307), and
//    llama_split_mode defaults to LAYER -- a split across every GPU. The machine
//    TASK-413 runs on enumerates THREE devices (an Intel Arc iGPU, an RTX 5070
//    and the CPU), so the default would let the weights straddle two cards while
//    the memory sampler reported the free-VRAM mark of ONE of them. Bar #4's
//    only question is "does this fit the modal 8 GB card", so that error is in
//    the direction that MANUFACTURES A PASS. A tier must mean one named device.

// ⚠️ `enum ggml_backend_dev_type` IS SPELLED OUT, AND IT HAS TO BE. ggml declares
// a FUNCTION of exactly that name (ggml-backend.h:182), which hides the enum type
// in C++ -- the unelaborated spelling names the function and does not compile.
// The vendored header writes it the same way for the same reason.
static const TCHAR* DeviceTypeName(enum ggml_backend_dev_type Type)
{
	switch (Type)
	{
		case GGML_BACKEND_DEVICE_TYPE_CPU:   return TEXT("CPU");
		case GGML_BACKEND_DEVICE_TYPE_GPU:   return TEXT("GPU");
		case GGML_BACKEND_DEVICE_TYPE_IGPU:  return TEXT("iGPU");
		case GGML_BACKEND_DEVICE_TYPE_ACCEL: return TEXT("ACCEL");
		case GGML_BACKEND_DEVICE_TYPE_META:  return TEXT("META");
		default:                             return TEXT("unknown");
	}
}

/** Everything printable about one ggml device, sampled at one instant. */
struct FSpikeDeviceInfo
{
	ggml_backend_dev_t Device = nullptr;
	int32 Index = INDEX_NONE;
	FString Name;
	FString Description;
	enum ggml_backend_dev_type Type = GGML_BACKEND_DEVICE_TYPE_CPU;
	uint64 FreeBytes = 0;
	uint64 TotalBytes = 0;
};

static void EnumerateDevices(TArray<FSpikeDeviceInfo>& Out)
{
	Out.Reset();

	const int32 DeviceCount = static_cast<int32>(ggml_backend_dev_count());
	for (int32 Index = 0; Index < DeviceCount; ++Index)
	{
		ggml_backend_dev_t Device = ggml_backend_dev_get(static_cast<size_t>(Index));
		if (Device == nullptr)
		{
			continue;
		}

		FSpikeDeviceInfo Info;
		Info.Device = Device;
		Info.Index = Index;

		const char* NameUtf8 = ggml_backend_dev_name(Device);
		const char* DescriptionUtf8 = ggml_backend_dev_description(Device);
		Info.Name = NameUtf8 != nullptr ? UTF8_TO_TCHAR(NameUtf8) : TEXT("<unnamed>");
		Info.Description = DescriptionUtf8 != nullptr ? UTF8_TO_TCHAR(DescriptionUtf8) : TEXT("<no description>");
		Info.Type = ggml_backend_dev_type(Device);

		size_t FreeBytes = 0;
		size_t TotalBytes = 0;
		ggml_backend_dev_memory(Device, &FreeBytes, &TotalBytes);
		Info.FreeBytes = static_cast<uint64>(FreeBytes);
		Info.TotalBytes = static_cast<uint64>(TotalBytes);

		Out.Add(MoveTemp(Info));
	}
}

static FString FormatMiB(uint64 Bytes)
{
	return FString::Printf(TEXT("%.1f MiB"), static_cast<double>(Bytes) / 1048576.0);
}

/** MiB, or a literal n/a -- never a 0 that reads like a measurement. */
static FString FormatVram(uint64 Bytes, bool bHasDevice)
{
	return bHasDevice ? FormatMiB(Bytes) : FString(TEXT("n/a"));
}

/** dev=<index> name=<name> type=<GPU|iGPU|CPU|...>, the exact string every VRAM line is tagged with. */
static FString DeviceLabel(const FSpikeDeviceInfo& Info)
{
	return FString::Printf(TEXT("dev=%d/%s/%s"), Info.Index, *Info.Name, DeviceTypeName(Info.Type));
}

/** Prints EVERY device, so bar #4's subject is on the page rather than in someone's head. */
static void LogDeviceInventory(const TCHAR* Stage, const TArray<FSpikeDeviceInfo>& Devices, ggml_backend_dev_t Selected)
{
	if (Devices.Num() == 0)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s stage=%s: ggml enumerates ZERO devices. Every VRAM figure in this run is n/a, not 0."),
			TagMem, Stage);
		return;
	}

	for (const FSpikeDeviceInfo& Info : Devices)
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s stage=%s %s free=%s total=%s%s desc=\"%s\""),
			TagMem, Stage, *DeviceLabel(Info),
			*FormatMiB(Info.FreeBytes), *FormatMiB(Info.TotalBytes),
			(Selected != nullptr && Info.Device == Selected) ? TEXT(" <== PINNED, AND THE SUBJECT OF EVERY vram_ FIGURE") : TEXT(""),
			*Info.Description);
	}
}

/**
 *  Resolves gpu= to exactly one device.
 *
 *  gpu=<index> is the index printed on the SPIKE_MEM stage=devices lines, and
 *  gpu=<name> matches ggml_backend_dev_name -- so whatever the log printed can
 *  be pasted straight back into the next command. With no gpu= the first
 *  DISCRETE GPU wins, then the first iGPU; either way the choice is logged as
 *  ASSUMED so nobody mistakes a default for a decision.
 *
 *  @return nullptr when nothing suitable exists, with the reason in OutReason.
 */
static ggml_backend_dev_t ResolveOffloadDevice(const TArray<FSpikeDeviceInfo>& Devices,
	const FString& Spec, bool& bOutAssumed, FString& OutReason)
{
	bOutAssumed = true;
	OutReason.Reset();

	if (!Spec.IsEmpty())
	{
		const bool bIsIndex = Spec.IsNumeric();
		for (const FSpikeDeviceInfo& Info : Devices)
		{
			const bool bMatches = bIsIndex
				? (Info.Index == FCString::Atoi(*Spec))
				: Info.Name.Equals(Spec, ESearchCase::IgnoreCase);

			if (!bMatches)
			{
				continue;
			}

			if (Info.Type == GGML_BACKEND_DEVICE_TYPE_CPU || Info.Type == GGML_BACKEND_DEVICE_TYPE_ACCEL)
			{
				OutReason = FString::Printf(
					TEXT("gpu='%s' names %s, which is not an offload target -- NOT pinning, and no vram_ figure in this run has a subject. Use tier=cpu for a CPU run."),
					*Spec, *DeviceLabel(Info));
				return nullptr;
			}

			bOutAssumed = false;
			return Info.Device;
		}

		OutReason = FString::Printf(
			TEXT("gpu='%s' matches no enumerated device -- falling back to the first discrete GPU. The pinned device is printed below; pass its index or name explicitly."),
			*Spec);
	}

	for (const FSpikeDeviceInfo& Info : Devices)
	{
		if (Info.Type == GGML_BACKEND_DEVICE_TYPE_GPU)
		{
			return Info.Device;
		}
	}

	for (const FSpikeDeviceInfo& Info : Devices)
	{
		if (Info.Type == GGML_BACKEND_DEVICE_TYPE_IGPU)
		{
			if (OutReason.IsEmpty())
			{
				OutReason = TEXT("no DISCRETE GPU is enumerated -- pinning the integrated GPU instead. Bar #4's 8 GB question is about a discrete card, so read this run as a different machine.");
			}
			return Info.Device;
		}
	}

	if (OutReason.IsEmpty())
	{
		OutReason = TEXT("no GPU device is enumerated at all -- nothing is pinned and every vram_ figure reads n/a. On a CPU-only run that is correct; on a GPU tier it means the backend did not register.");
	}
	return nullptr;
}

/**
 *  ⚠️ THE SUBJECT OF THIS SAMPLE IS GRunner.OffloadDevice -- THE PINNED ONE.
 *  It used to take "the first discrete GPU it finds", which is an assumption
 *  rather than a measurement, and on a two-GPU machine it can name a card the
 *  weights were never on. The fallback below is only reached before a load has
 *  pinned anything, and it marks itself as assumed.
 */
static FSpikeMemorySnapshot SampleMemory()
{
	FSpikeMemorySnapshot Snapshot;

	ggml_backend_dev_t Device = GRunner.OffloadDevice;
	if (Device == nullptr)
	{
		// Pre-load only: the pinned device is not chosen until EnsureModelLoaded.
		const int32 DeviceCount = static_cast<int32>(ggml_backend_dev_count());
		for (int32 Index = 0; Index < DeviceCount && Device == nullptr; ++Index)
		{
			ggml_backend_dev_t Candidate = ggml_backend_dev_get(static_cast<size_t>(Index));
			if (Candidate != nullptr && ggml_backend_dev_type(Candidate) == GGML_BACKEND_DEVICE_TYPE_GPU)
			{
				Device = Candidate;
			}
		}
	}

	if (Device != nullptr)
	{
		// Device-wide free bytes, not this process's usage -- see note (c).
		size_t FreeBytes = 0;
		size_t TotalBytes = 0;
		ggml_backend_dev_memory(Device, &FreeBytes, &TotalBytes);
		Snapshot.VramFreeBytes = static_cast<uint64>(FreeBytes);
		Snapshot.VramTotalBytes = static_cast<uint64>(TotalBytes);
		Snapshot.bHasDevice = true;
	}

	const FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
	Snapshot.RssBytes = static_cast<uint64>(Stats.UsedPhysical);
	return Snapshot;
}

/**
 *  WHERE EACH ZONE STARTS IN THE ASSEMBLED PROMPT, in CHARACTERS.
 *
 *  This is what turns bar #3 from a number into evidence: the KV reuse point is
 *  a token index, and without the zone boundaries beside it nobody can tell
 *  whether the reuse landed at the start of Zone B (the shipped worst case) or
 *  in Zone C's tail (a best case the shipped path never reaches). INDEX_NONE
 *  means "could not be located" -- reported as unknown, NEVER as zero.
 */
struct FSpikePromptLayout
{
	int32 ZoneBCharOffset = INDEX_NONE;
	int32 ZoneCCharOffset = INDEX_NONE;
};

/** Assembles the full prompt. Zone A is the system message when a chat template is available. */
static void BuildPrompt(const FSpikeOptions& Options, const FString& ZoneAText,
	const FSpikeWorldFixture& Fixture, const FString& Utterance, const FString& PendingLine,
	FString& OutPrompt, FSpikePromptLayout* OutLayout = nullptr)
{
	if (OutLayout != nullptr)
	{
		*OutLayout = FSpikePromptLayout();
	}

	FString ZoneB;
	AppendZoneB(ZoneB, Fixture);

	FString ZoneC;
	AppendZoneC(ZoneC, Fixture, SanitizeForPrompt(Utterance), SanitizeForPrompt(PendingLine));

	const FString UserBlock = ZoneB + ZoneC;

	const char* TemplateText = Options.bUseChatTemplate && GRunner.Model
		? llama_model_chat_template(GRunner.Model, nullptr)
		: nullptr;

	// The zone offsets are computed STRUCTURALLY, never by searching for a marker
	// string: "[FORCES]" also appears inside Zone A's schema block, so a marker
	// search would silently report a Zone C boundary inside the RULES.
	if (TemplateText == nullptr)
	{
		// Raw concatenation. Zone A stays the static prefix either way, so bar #3
		// is measurable in both modes -- only the absolute token counts move.
		OutPrompt = ZoneAText + UserBlock;
		if (OutLayout != nullptr)
		{
			OutLayout->ZoneBCharOffset = ZoneAText.Len();
			OutLayout->ZoneCCharOffset = ZoneAText.Len() + ZoneB.Len();
		}
		return;
	}

	const FTCHARToUTF8 SystemUtf8(*ZoneAText);
	const FTCHARToUTF8 UserUtf8(*UserBlock);

	llama_chat_message Messages[2];
	Messages[0].role = "system";
	Messages[0].content = SystemUtf8.Get();
	Messages[1].role = "user";
	Messages[1].content = UserUtf8.Get();

	TArray<char> Buffer;
	Buffer.SetNumZeroed(2 * (SystemUtf8.Length() + UserUtf8.Length()) + 1024);

	int32 Written = llama_chat_apply_template(TemplateText, Messages, 2, true, Buffer.GetData(), Buffer.Num());
	if (Written > Buffer.Num())
	{
		Buffer.SetNumZeroed(Written + 1);
		Written = llama_chat_apply_template(TemplateText, Messages, 2, true, Buffer.GetData(), Buffer.Num());
	}

	if (Written <= 0 || Written >= Buffer.Num())
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: llama_chat_apply_template returned %d for a %d-byte buffer -- falling back to raw concatenation. The token counts from this run are NOT comparable to a templated run."),
			TagWarn, Written, Buffer.Num());
		OutPrompt = ZoneAText + UserBlock;
		if (OutLayout != nullptr)
		{
			OutLayout->ZoneBCharOffset = ZoneAText.Len();
			OutLayout->ZoneCCharOffset = ZoneAText.Len() + ZoneB.Len();
		}
		return;
	}

	// The buffer was zero-filled and Written < Num(), so it is NUL-terminated
	// at [Written] -- llama_chat_apply_template does not promise a terminator.
	OutPrompt = UTF8_TO_TCHAR(Buffer.GetData());

	if (OutLayout != nullptr)
	{
		// The template WRAPS the user block, it does not rewrite it, so the block
		// appears verbatim exactly once and its offset is exact. If a template ever
		// escapes or re-encodes it, Find fails and both offsets stay INDEX_NONE --
		// which prints as `unknown` rather than as a confident wrong number.
		const int32 UserOffset = OutPrompt.Find(UserBlock, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		if (UserOffset != INDEX_NONE)
		{
			OutLayout->ZoneBCharOffset = UserOffset;
			OutLayout->ZoneCCharOffset = UserOffset + ZoneB.Len();
		}
	}
}

static bool TokenizePrompt(const FString& Prompt, TArray<llama_token>& OutTokens)
{
	OutTokens.Reset();

	const FTCHARToUTF8 Utf8(*Prompt);

	int32 Needed = -llama_tokenize(GRunner.Vocab, Utf8.Get(), Utf8.Length(), nullptr, 0, true, true);
	if (Needed <= 0)
	{
		return false;
	}

	OutTokens.SetNumUninitialized(Needed);
	const int32 Written = llama_tokenize(GRunner.Vocab, Utf8.Get(), Utf8.Length(),
		OutTokens.GetData(), OutTokens.Num(), true, true);
	if (Written < 0)
	{
		OutTokens.Reset();
		return false;
	}

	OutTokens.SetNum(Written);
	return true;
}

// ---------------------------------------------------------------------------
// WHERE THE KV REUSE POINT LANDED (bar #3's evidence)
// ---------------------------------------------------------------------------

/** Zone boundaries in TOKEN space, plus the classification of one reuse point. */
struct FSpikeDivergence
{
	int32 ZoneBStartTokens = INDEX_NONE;
	int32 ZoneCStartTokens = INDEX_NONE;
	int32 ReusedTokens = 0;
	const TCHAR* LandedIn = TEXT("unknown");
	bool bResolved = false;

	/** TRUE when the reuse point is at or past the start of Zone C -- the best case, not the shipped one. */
	bool bLandedInZoneCTail = false;
};

/**
 *  Token index of a character offset, obtained by tokenizing the PREFIX.
 *
 *  ⚠️ APPROXIMATE BY +/-1 AND SAID SO EVERYWHERE IT IS PRINTED. Tokenization is
 *  not compositional: a merge can straddle the boundary, so the prefix's token
 *  count is not guaranteed to be the index of that boundary in the full
 *  tokenization. It is exact enough to answer the only question being asked --
 *  "did the divergence land in Zone A, Zone B or Zone C" -- and that question is
 *  what separates a shipped-shaped bar #3 from a flattering one.
 */
static int32 TokenIndexOfCharOffset(const FString& Prompt, int32 CharOffset)
{
	if (CharOffset <= 0 || CharOffset > Prompt.Len())
	{
		return INDEX_NONE;
	}

	TArray<llama_token> Tokens;
	if (!TokenizePrompt(Prompt.Left(CharOffset), Tokens))
	{
		return INDEX_NONE;
	}
	return Tokens.Num();
}

/** Classifies a reuse point against the zone boundaries of the prompt it was measured on. */
static FSpikeDivergence ClassifyDivergence(const FString& Prompt, const FSpikePromptLayout& Layout, int32 ReusedTokens)
{
	FSpikeDivergence Divergence;
	Divergence.ReusedTokens = ReusedTokens;
	Divergence.ZoneBStartTokens = TokenIndexOfCharOffset(Prompt, Layout.ZoneBCharOffset);
	Divergence.ZoneCStartTokens = TokenIndexOfCharOffset(Prompt, Layout.ZoneCCharOffset);

	if (Divergence.ZoneBStartTokens == INDEX_NONE || Divergence.ZoneCStartTokens == INDEX_NONE)
	{
		return Divergence;
	}

	Divergence.bResolved = true;
	if (ReusedTokens < Divergence.ZoneBStartTokens)
	{
		Divergence.LandedIn = TEXT("ZONE_A");
	}
	else if (ReusedTokens < Divergence.ZoneCStartTokens)
	{
		Divergence.LandedIn = TEXT("ZONE_B");
	}
	else
	{
		Divergence.LandedIn = TEXT("ZONE_C");
		Divergence.bLandedInZoneCTail = true;
	}

	return Divergence;
}

/** `reused=N landed_in=ZONE_B zoneB_start~=N zoneC_start~=N` -- the same column set everywhere. */
static FString FormatDivergence(const FSpikeDivergence& Divergence)
{
	if (!Divergence.bResolved)
	{
		return FString::Printf(TEXT("reused=%d landed_in=unknown (zone boundaries could not be located in the assembled prompt)"),
			Divergence.ReusedTokens);
	}

	return FString::Printf(TEXT("reused=%d landed_in=%s zoneB_start~=%d zoneC_start~=%d"),
		Divergence.ReusedTokens, Divergence.LandedIn,
		Divergence.ZoneBStartTokens, Divergence.ZoneCStartTokens);
}

struct FGenerationResult
{
	FString Text;
	double TtftMs = 0.0;
	double PrefillMs = 0.0;
	double DecodeMs = 0.0;
	double WallMs = 0.0;
	int32 PromptTokens = 0;
	int32 PrefixReused = 0;
	int32 PrefillTokens = 0;
	int32 OutputTokens = 0;
	uint64 VramFreeLowWaterBytes = MAX_uint64;
	bool bAborted = false;
	bool bDecodeFailed = false;

	/** TRUE when any VRAM figure here has a device behind it. See FSpikeMemorySnapshot::bHasDevice. */
	bool bHasVramDevice = false;

	/**
	 *  The output-token index at which the model first emitted an end-of-generation
	 *  token, or INDEX_NONE. Under a grammar the loop stops there; under
	 *  bIgnoreEogForFixedLength it keeps decoding to the budget and THIS is what
	 *  says the trailing tokens are filler (qa/TASK-412.md WARN-10).
	 */
	int32 EogTokenIndex = INDEX_NONE;
	bool bEogIgnored = false;

	/**
	 *  ⚠️ WHY THE FAILURE IS CARRIED AND NOT MERELY LOGGED (TASK-429, the WARN-R2
	 *  fix -- CONVENTIONS section 12e). Every stop site below already printed its
	 *  own SPIKE_WARN, but the CALLER could not see WHY a turn stopped: it only
	 *  saw bDecodeFailed/bAborted, and RunPrefillBound did not even read those.
	 *  So a bound whose turn 1 aborted printed a DROP figure that is an artifact
	 *  of the cleared cache and then accused the PROMPT LAYOUT of it, at maximum
	 *  volume, twice. Anything that judges a bound now has the reason in hand and
	 *  can print it INSTEAD of a verdict.
	 *
	 *  Empty when the turn ran to a natural stop. Preformatted at the stop site,
	 *  which is the only place that holds the llama_decode return code AND the
	 *  offset/token it happened at.
	 */
	FString FailureDetail;

	/**
	 *  ⚠️ THE ONLY PREDICATE A LAYOUT VERDICT MAY BE GATED ON. "Completed" means
	 *  the turn reached a natural stop -- EOG, the token budget, or the context
	 *  ceiling -- with no abort and no llama_decode error. A turn that stopped
	 *  any other way produces prefill/reuse counters that describe the failure,
	 *  not the layout.
	 */
	bool CompletedNormally() const { return !bAborted && !bDecodeFailed; }
};

/**
 *  ONE single-turn call: one utterance + one snapshot -> one constrained JSON.
 *  THE MODEL NEVER SEES A PRIOR TURN (CONVENTIONS section 1). What is reused
 *  across calls is the KV CACHE of the byte-identical Zone A + Zone B prefix,
 *  which is a cache, not a conversation.
 *
 *  @param Fixture  THE BOARD THIS PROMPT WAS BUILT FROM (TASK-478). The GBNF's
 *                  `kind` alternatives are generated from it, so it must be the
 *                  SAME fixture the caller passed to BuildPrompt -- a mismatch
 *                  shows the model one board and constrains it to another.
 *
 *  ⛔ NO DEFAULT ARGUMENT, DELIBERATELY. A `= SpikeFixtureT0` default would turn a
 *     forgotten call site into a SILENTLY WRONG grammar; with no default it is a
 *     compile error. Every one of this function's callers already holds the fixture
 *     it built the prompt from, so the parameter costs nothing at any of them.
 */
static void RunGeneration(const FSpikeOptions& Options, const FString& Prompt,
	const FSpikeWorldFixture& Fixture, FGenerationResult& Out)
{
	const double StartSeconds = FPlatformTime::Seconds();

	TArray<llama_token> PromptTokens;
	if (!TokenizePrompt(Prompt, PromptTokens))
	{
		Out.bDecodeFailed = true;
		Out.FailureDetail = TEXT("TOKENIZE failed -- llama_decode was never called, so there is no return code and nothing about the layout can be read from this turn");
		return;
	}
	Out.PromptTokens = PromptTokens.Num();

	// ⚠️ THE EFFECTIVE n_ctx, NOT THE REQUESTED ONE (qa/TASK-412.md WARN-2).
	// llama.h:551-556: "the requested values via llama_context_params may differ
	// from the actual values used by the context". Checking the request would
	// wave through a prompt the context cannot hold, and every bar would then
	// read "inference failed" for a reason no log line names.
	if (PromptTokens.Num() + Options.MaxOutputTokens > GRunner.ContextSize)
	{
		// Caught here rather than as an opaque llama_decode return code: a prompt
		// that does not fit is a PROMPT-BUDGET finding (Zone A + Zone B + Zone C
		// against the context), and it must read as one.
		UE_LOG(LogSiegeLlama, Error,
			TEXT("%s: prompt is %d tokens and the output budget is %d, which exceeds the context's EFFECTIVE n_ctx=%d (requested ctx=%d). Raise ctx= or shrink Zone B/C -- this is a budget failure, not a model failure."),
			TagWarn, PromptTokens.Num(), Options.MaxOutputTokens, GRunner.ContextSize, Options.ContextTokens);
		Out.bDecodeFailed = true;
		Out.FailureDetail = FString::Printf(
			TEXT("PROMPT_BUDGET %d prompt + %d output tokens exceed the ACTUAL n_ctx=%d -- llama_decode was never called, so there is no return code"),
			PromptTokens.Num(), Options.MaxOutputTokens, GRunner.ContextSize);
		return;
	}

	// --- KV-PREFIX REUSE (BAR #3) ------------------------------------------
	// Common prefix with the previous turn's prompt, minus nothing: the divergent
	// tail is dropped from the cache and only that tail is re-prefilled. Zone A
	// and Zone B are byte-identical between turns by construction, so the reuse
	// point should land at the start of Zone C. If it does not, the LAYOUT is
	// wrong and must be fixed before anything else is built (plan, bar #3).
	int32 CommonPrefix = 0;
	while (CommonPrefix < PromptTokens.Num()
		&& CommonPrefix < GRunner.LastPromptTokens.Num()
		&& PromptTokens[CommonPrefix] == GRunner.LastPromptTokens[CommonPrefix])
	{
		++CommonPrefix;
	}

	// At least one token must be decoded, or there are no logits to sample from.
	CommonPrefix = FMath::Clamp(CommonPrefix, 0, PromptTokens.Num() - 1);

	llama_memory_t Memory = llama_get_memory(GRunner.Context);

	// ⚠️ THE RETURN VALUE IS CHECKED, AND THAT IS NOT DEFENSIVE PADDING.
	// llama_memory_seq_rm returns false when a PARTIAL sequence cannot be
	// removed (it is documented to; SWA and recurrent memories are the cases).
	// Ignoring it would leave the cache holding tokens this turn's positions are
	// about to overwrite, and the resulting garbage would show up as a BAD
	// ACCURACY NUMBER with no log line naming the cause -- while bar #3 happily
	// reported a large, entirely fictional prefix reuse. Falling back to a full
	// clear costs one slow turn and keeps both numbers true.
	if (CommonPrefix > 0 && !llama_memory_seq_rm(Memory, 0, CommonPrefix, -1))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: llama_memory_seq_rm refused a partial removal at %d -- clearing the whole cache and re-prefilling. This turn's KV-reuse figure is 0 by construction, not by measurement."),
			TagWarn, CommonPrefix);

		llama_memory_clear(Memory, true);
		CommonPrefix = 0;
	}
	else if (CommonPrefix == 0)
	{
		llama_memory_seq_rm(Memory, 0, 0, -1);
	}

	Out.PrefixReused = CommonPrefix;
	Out.PrefillTokens = PromptTokens.Num() - CommonPrefix;

	// ⚠️ THE DEADLINE COMES FROM THE OPTIONS, NOT FROM THE CONSTANT (TASK-429).
	// Options.HardTimeoutSeconds DEFAULTS to SpikeHardTimeoutSeconds and the
	// default is unchanged at 10.0 -- deadline= is a diagnostic override so the
	// CPU aborts can be classified as a tunable or a wall, and every line that
	// prints a wall time beside it says which regime and whether it was moved.
	GRunner.AbortDeadlineSeconds = StartSeconds + Options.HardTimeoutSeconds;

	// --- PREFILL ------------------------------------------------------------
	GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Prefill));
	const double PrefillStart = FPlatformTime::Seconds();

	// ⚠️ SUBMITTED IN n_batch SLICES, NOT AS ONE BATCH. llama_decode rejects a
	// batch larger than n_batch outright, and the full prompt here is ~1000
	// tokens against a 512-token n_batch -- a single-batch prefill would have
	// failed on the very first call of the very first tier. The slicing is also
	// what n_ubatch's contention mitigation rests on.
	{
		int32 Offset = CommonPrefix;
		bool bPrefillFailed = false;

		while (Offset < PromptTokens.Num())
		{
			const int32 ChunkTokens = FMath::Min(FMath::Max(1, GRunner.BatchSize), PromptTokens.Num() - Offset);

			llama_batch Batch = llama_batch_get_one(PromptTokens.GetData() + Offset, ChunkTokens);
			const int32 DecodeResult = llama_decode(GRunner.Context, Batch);
			if (DecodeResult != 0)
			{
				Out.bDecodeFailed = true;
				// llama.h:963-977 enumerates the codes: 2 = aborted, 1 = no KV
				// slot, -1 = invalid batch, < -1 = fatal. Only 2 is this harness's
				// deadline firing.
				Out.bAborted = (DecodeResult == 2);
				Out.FailureDetail = FString::Printf(
					TEXT("PREFILL llama_decode=%d at prompt offset %d (chunk %d, n_batch %d)%s"),
					DecodeResult, Offset, ChunkTokens, GRunner.BatchSize,
					(DecodeResult == 2)
						? TEXT(" -- code 2 is ABORTED, i.e. this run hit the deadline INSIDE the prefill")
						: TEXT(""));
				UE_LOG(LogSiegeLlama, Warning,
					TEXT("%s: prefill llama_decode returned %d at offset %d (chunk %d, n_batch %d)."),
					TagWarn, DecodeResult, Offset, ChunkTokens, GRunner.BatchSize);
				bPrefillFailed = true;
				break;
			}

			Offset += ChunkTokens;
		}

		if (bPrefillFailed)
		{
			GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Idle));
			GRunner.AbortDeadlineSeconds = 0.0;
			// The cache is now in an unknown state, so the next turn must not
			// claim a prefix it cannot prove.
			GRunner.LastPromptTokens.Reset();
			return;
		}
	}

	Out.PrefillMs = (FPlatformTime::Seconds() - PrefillStart) * 1000.0;

	const FSpikeMemorySnapshot AfterPrefill = SampleMemory();
	if (AfterPrefill.bHasDevice)
	{
		Out.bHasVramDevice = true;
		Out.VramFreeLowWaterBytes = FMath::Min(Out.VramFreeLowWaterBytes, AfterPrefill.VramFreeBytes);
	}

	// --- SAMPLER ------------------------------------------------------------
	// GREEDY, and greedy on purpose: an accuracy bar measured with a stochastic
	// sampler is not reproducible, and a go/no-go number that moves between runs
	// is not a number.
	llama_sampler* Chain = llama_sampler_chain_init(llama_sampler_chain_default_params());

	FString GrammarText;
	if (Options.bUseGrammar)
	{
		GrammarText = BuildSpikeGrammar(Fixture);
		const FTCHARToUTF8 GrammarUtf8(*GrammarText);
		llama_sampler* Grammar = llama_sampler_init_grammar(GRunner.Vocab, GrammarUtf8.Get(), "root");
		if (Grammar == nullptr)
		{
			UE_LOG(LogSiegeLlama, Error,
				TEXT("%s: llama_sampler_init_grammar returned NULL -- the GBNF FAILED TO PARSE. Every number from this run is unconstrained and must not be reported as a constrained result."),
				TagWarn);
		}
		else
		{
			llama_sampler_chain_add(Chain, Grammar);
		}
	}
	llama_sampler_chain_add(Chain, llama_sampler_init_greedy());

	// --- DECODE -------------------------------------------------------------
	GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Decode));
	const double DecodeStart = FPlatformTime::Seconds();

	// Bytes are accumulated RAW and converted once at the end: a single UTF-8
	// code point can straddle two tokens, and converting per token would turn it
	// into replacement characters. Irrelevant for ASCII JSON, wrong the moment
	// it is not.
	TArray<char> OutputBytes;
	OutputBytes.Reserve(512);

	int32 CurrentPosition = PromptTokens.Num();
	for (int32 TokenIndex = 0; TokenIndex < Options.MaxOutputTokens; ++TokenIndex)
	{
		const llama_token Token = llama_sampler_sample(Chain, GRunner.Context, -1);

		if (TokenIndex == 0)
		{
			Out.TtftMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		}

		// ⚠️ THE FIXED-LENGTH CONTROL IS ONLY FIXED-LENGTH IF EOG IS IGNORED
		// (qa/TASK-412.md WARN-10). The grammar=0 tokens=60 run exists to time a
		// GENUINE 60-token decode; stopping at EOG turned it into a second
		// extrapolation, which is the thing it was added to avoid. So under
		// bIgnoreEogForFixedLength the loop keeps decoding to the budget, the EOG
		// position is recorded, and the pieces AFTER it are dropped from the text
		// (they are timing filler, never an answer). The eval job never sets the
		// flag: an accuracy run must stop where the model stopped.
		const bool bIsEog = llama_vocab_is_eog(GRunner.Vocab, Token);
		if (bIsEog)
		{
			if (Out.EogTokenIndex == INDEX_NONE)
			{
				Out.EogTokenIndex = TokenIndex;
			}

			if (!Options.bIgnoreEogForFixedLength)
			{
				break;
			}
			Out.bEogIgnored = true;
		}

		if (!bIsEog && Out.EogTokenIndex == INDEX_NONE)
		{
			char PieceBuffer[256];
			const int32 PieceLength = llama_token_to_piece(GRunner.Vocab, Token, PieceBuffer,
				static_cast<int32>(sizeof(PieceBuffer)), 0, false);
			if (PieceLength > 0)
			{
				OutputBytes.Append(PieceBuffer, PieceLength);
			}
		}

		++Out.OutputTokens;

		// The EFFECTIVE n_ctx again, for the same reason as the budget check above.
		if (CurrentPosition + 1 >= GRunner.ContextSize)
		{
			UE_LOG(LogSiegeLlama, Warning, TEXT("%s: context full at %d tokens (n_ctx=%d) -- generation truncated."),
				TagWarn, CurrentPosition, GRunner.ContextSize);
			break;
		}

		llama_token NextToken = Token;
		llama_batch Batch = llama_batch_get_one(&NextToken, 1);
		const int32 DecodeResult = llama_decode(GRunner.Context, Batch);
		if (DecodeResult != 0)
		{
			Out.bDecodeFailed = true;
			Out.bAborted = (DecodeResult == 2);
			Out.FailureDetail = FString::Printf(
				TEXT("DECODE llama_decode=%d at output token %d of a %d-token budget%s"),
				DecodeResult, TokenIndex, Options.MaxOutputTokens,
				(DecodeResult == 2)
					? TEXT(" -- code 2 is ABORTED, i.e. this run hit the deadline mid-decode and the JSON is TRUNCATED, not wrong")
					: TEXT(""));
			UE_LOG(LogSiegeLlama, Warning, TEXT("%s: decode llama_decode returned %d at token %d."), TagWarn, DecodeResult, TokenIndex);
			break;
		}
		++CurrentPosition;

		if ((TokenIndex % SpikeVramSampleTokenStride) == 0)
		{
			const FSpikeMemorySnapshot DuringDecode = SampleMemory();
			if (DuringDecode.bHasDevice)
			{
				Out.bHasVramDevice = true;
				Out.VramFreeLowWaterBytes = FMath::Min(Out.VramFreeLowWaterBytes, DuringDecode.VramFreeBytes);
			}
		}

		if (FPlatformTime::Seconds() > GRunner.AbortDeadlineSeconds)
		{
			Out.bAborted = true;
			Out.FailureDetail = FString::Printf(
				TEXT("DEADLINE %.1fs elapsed after %d output token(s) of a %d-token budget -- llama_decode never returned an error; the LOOP stopped, so this is the ceiling cutting a slow run, not a generation failure. Re-run with deadline=<larger> to separate the two"),
				Options.HardTimeoutSeconds, Out.OutputTokens, Options.MaxOutputTokens);
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: hard timeout of %.1fs hit after %d output token(s). %s"),
				TagWarn, Options.HardTimeoutSeconds, Out.OutputTokens, *FormatDeadlineRegime(Options));
			break;
		}
	}

	llama_sampler_free(Chain);

	Out.DecodeMs = (FPlatformTime::Seconds() - DecodeStart) * 1000.0;
	Out.WallMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

	OutputBytes.Add('\0');
	Out.Text = UTF8_TO_TCHAR(OutputBytes.GetData());

	GRunner.LastPromptTokens = MoveTemp(PromptTokens);
	GRunner.AbortDeadlineSeconds = 0.0;
	GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Idle));
}

static void UnloadModel()
{
	if (GRunner.Context != nullptr)
	{
		llama_free(GRunner.Context);
		GRunner.Context = nullptr;
	}
	if (GRunner.Model != nullptr)
	{
		llama_model_free(GRunner.Model);
		GRunner.Model = nullptr;
	}
	GRunner.Vocab = nullptr;
	GRunner.LastPromptTokens.Reset();
	GRunner.ModelLayerCount = 0;
	GRunner.ModelSizeBytes = 0;

	// The pinned device belongs to the LOADED model. Leaving it set would let a
	// later sample report a device this process is no longer using.
	GRunner.OffloadDevice = nullptr;
	GRunner.OffloadDeviceLabel = TEXT("<none>");
	GRunner.bOffloadDeviceAssumed = true;
	GRunner.LoadedModelPath.Reset();
}

static int32 DefaultThreadCount()
{
	// Leave two cores for the game. A spike that pins every core would measure a
	// configuration nobody would ship and call the result "CPU-only latency".
	const int32 Cores = FPlatformMisc::NumberOfCores();
	return FMath::Max(1, Cores - 2);
}

/** @return true when a usable model + context are resident with the requested options. */
static bool EnsureModelLoaded(const FSpikeOptions& Options)
{
	// ⚠️ THE MODEL PATH AND THE DEVICE ARE PART OF THE RELOAD KEY (qa/TASK-412.md
	// BLOCKER-3). LoadedModelPath used to be stored and never read, so
	// `SpikeBench model=<B>` after `model=<A>` at the same tier kept A resident
	// and printed A's numbers under B's command line -- with no SPIKE_LOAD line
	// at all, because no load happened. TASK-413's ladder swaps models at the
	// same tier, so that is a silent wrong measurement on the exact path the
	// go/no-go runs on. The same argument applies to gpu=: a tier that quietly
	// kept the previous device would report the previous device's VRAM.
	// FString comparison is case-insensitive in UE, which is right for Windows
	// paths; a slash-style difference merely costs one honest reload.
	//
	// ⚠️ deadline= IS DELIBERATELY *NOT* PART OF THIS KEY (TASK-429). It is armed
	// per generation from Options.HardTimeoutSeconds and touches nothing the
	// context is built from, so including it would force a full 2.5 GB reload
	// between every cell of the deadline sweep -- changing nothing except how
	// long the diagnosis takes. Contrast Threads, which IS in the key: it goes
	// into llama_context_params and a stale context would answer with the
	// previous cell's thread count.
	if (GRunner.IsLoaded()
		&& GRunner.LoadedModelPath == Options.ModelPathOverride
		&& GRunner.LoadedOptions.GpuDeviceSpec == Options.GpuDeviceSpec
		&& GRunner.LoadedOptions.GpuLayers == Options.GpuLayers
		&& GRunner.LoadedOptions.ContextTokens == Options.ContextTokens
		&& GRunner.LoadedOptions.UBatch == Options.UBatch
		&& GRunner.LoadedOptions.Threads == Options.Threads)
	{
		return true;
	}

	UnloadModel();

	// Already resolved on the GAME THREAD in StartJob -- USiegeLlamaSettings is a
	// UObject CDO and this function runs on a worker.
	const FString& ModelPath = Options.ModelPathOverride;

	if (!FPaths::FileExists(ModelPath))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: no model at '%s'. Run Tools/fetch_llm_model.py, or pass model=<path>. This is a DEGRADED state, not a crash."),
			TagLoad, *ModelPath);
		return false;
	}

	GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Load));

	// --- PIN THE OFFLOAD DEVICE (bar #4's subject) ---------------------------
	TArray<FSpikeDeviceInfo> DevicesBefore;
	EnumerateDevices(DevicesBefore);

	FString DeviceReason;
	bool bDeviceAssumed = true;
	ggml_backend_dev_t PinnedDevice = ResolveOffloadDevice(DevicesBefore, Options.GpuDeviceSpec, bDeviceAssumed, DeviceReason);

	if (!DeviceReason.IsEmpty())
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: %s"), TagWarn, *DeviceReason);
	}

	GRunner.OffloadDevice = PinnedDevice;
	GRunner.bOffloadDeviceAssumed = bDeviceAssumed;
	GRunner.OffloadDeviceLabel = TEXT("<none>");
	for (const FSpikeDeviceInfo& Info : DevicesBefore)
	{
		if (Info.Device == PinnedDevice)
		{
			GRunner.OffloadDeviceLabel = DeviceLabel(Info);
			break;
		}
	}

	LogDeviceInventory(TEXT("devices_before_load"), DevicesBefore, PinnedDevice);

	const FSpikeMemorySnapshot BeforeLoad = SampleMemory();
	const double LoadStart = FPlatformTime::Seconds();

	llama_model_params ModelParams = llama_model_default_params();
	ModelParams.n_gpu_layers = Options.GpuLayers;

	// ⚠️ ONE DEVICE, NAMED, NOT "ALL AVAILABLE" (qa/TASK-412.md BLOCKER-2).
	// llama.h:307 documents devices as "if NULL, all available devices are used",
	// and split_mode defaults to LAYER -- so on this machine (iGPU + dGPU + CPU)
	// the default would split the weights across two cards while bar #4 quoted
	// the free-VRAM mark of one. main_gpu indexes THIS list, so with a one-entry
	// list it is unambiguously 0: no assumption about which position ggml's own
	// filtered ordering would have put the card in.
	//
	// The array must outlive the load call, which it does -- llama copies it
	// during llama_model_load_from_file and this function does not return first.
	ggml_backend_dev_t PinnedDeviceList[2] = { PinnedDevice, nullptr };
	if (PinnedDevice != nullptr)
	{
		ModelParams.devices = PinnedDeviceList;
		ModelParams.split_mode = LLAMA_SPLIT_MODE_NONE;
		ModelParams.main_gpu = 0;
	}
	else
	{
		// Nothing to pin. Say so rather than letting the default quietly mean
		// "every device", which is the state this whole block exists to end.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: NO DEVICE PINNED -- llama will use its own default (all available devices, layer-split). Every vram_ figure from this run is unattributed; pass gpu=<index> from the device list above."),
			TagWarn);
	}

	// ⚠️ THE `use_mmap = true` REQUIREMENT SURVIVES, BUT ITS FIELD DOES NOT.
	// CONVENTIONS section 7 and the plan both say `use_mmap = true`, and it is
	// load-bearing for the 8 GB claim (the weights stay page-cache-backed rather
	// than resident). In the VENDORED build (b10235) that bool is GONE -- it was
	// replaced by `load_mode`, and LLAMA_LOAD_MODE_MMAP is its exact successor.
	// Recorded here because TASK-423 will look for `use_mmap` and not find it.
	ModelParams.load_mode = LLAMA_LOAD_MODE_MMAP;

	GRunner.Model = llama_model_load_from_file(TCHAR_TO_UTF8(*ModelPath), ModelParams);
	if (GRunner.Model == nullptr)
	{
		GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Idle));
		UE_LOG(LogSiegeLlama, Error, TEXT("%s: llama_model_load_from_file FAILED for '%s'."), TagLoad, *ModelPath);
		return false;
	}

	GRunner.ModelLayerCount = llama_model_n_layer(GRunner.Model);
	GRunner.ModelSizeBytes = llama_model_size(GRunner.Model);
	GRunner.Vocab = llama_model_get_vocab(GRunner.Model);

	llama_context_params ContextParams = llama_context_default_params();
	ContextParams.n_ctx = static_cast<uint32>(Options.ContextTokens);
	ContextParams.n_batch = static_cast<uint32>(FMath::Max(SpikeDefaultBatch, Options.UBatch));
	ContextParams.n_ubatch = static_cast<uint32>(Options.UBatch);
	ContextParams.n_threads = Options.Threads > 0 ? Options.Threads : DefaultThreadCount();
	ContextParams.n_threads_batch = ContextParams.n_threads;
	ContextParams.abort_callback = &SpikeAbortCallback;
	ContextParams.abort_callback_data = nullptr;
	ContextParams.no_perf = false;

	GRunner.Context = llama_init_from_model(GRunner.Model, ContextParams);
	if (GRunner.Context == nullptr)
	{
		UnloadModel();
		GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Idle));
		UE_LOG(LogSiegeLlama, Error, TEXT("%s: llama_init_from_model FAILED."), TagLoad);
		return false;
	}

	const double LoadMs = (FPlatformTime::Seconds() - LoadStart) * 1000.0;
	const FSpikeMemorySnapshot AfterLoad = SampleMemory();

	GRunner.LoadedOptions = Options;
	GRunner.LoadedModelPath = ModelPath;

	// ⚠️ THE ACTUAL VALUES, QUERIED -- NOT THE REQUESTED ONES (qa/TASK-412.md
	// WARN-2). llama.h:551-556: "it is recommended to query the actual values
	// using these functions ... the requested values via llama_context_params may
	// differ from the actual values used by the context". If n_batch were clamped
	// below the request, the prefill would submit slices llama_decode rejects
	// outright and EVERY bar would read "inference failed"; if n_ctx were clamped,
	// the budget pre-check would wave through a prompt that does not fit.
	GRunner.BatchSize = static_cast<int32>(llama_n_batch(GRunner.Context));
	GRunner.ContextSize = static_cast<int32>(llama_n_ctx(GRunner.Context));
	const int32 ActualUBatch = static_cast<int32>(llama_n_ubatch(GRunner.Context));

	// ⚠️ THE FOURTH MEMBER OF THE SAME FAMILY, AND IT WAS MISSED BY THE PASS THAT
	// FIXED THE OTHER THREE (TASK-429; qa/TASK-412.md WARN-2 corrected n_ctx,
	// n_batch and n_ubatch and stopped there). n_threads is a REQUESTED value in
	// exactly the sense llama.h:551-556 warns about, and until now this file
	// printed `threads=` straight off llama_context_params -- i.e. it printed the
	// request and called it fact, which is the defect CONVENTIONS section 10 names.
	//
	// ⚠️ WHY IT IS NOT COSMETIC. The whole CPU-abort question is "is the cpu tier
	// slow because that is what the hardware does, or because it is not using the
	// hardware?" If the actual generation thread count comes back clamped -- to 1,
	// or to anything far below the physical core count -- that is a ROOT CAUSE and
	// a bug, not a wall, and it is settled by this one line instead of by a
	// benchmark matrix. llama_n_threads / llama_n_threads_batch (llama.h:985-989)
	// are the getters, and they are SEPARATE numbers: n_threads drives single-token
	// GENERATION (the decode loop, i.e. the part that runs out of time) while
	// n_threads_batch drives multi-token PREFILL. Both are printed, because a
	// clamp on either one is a different diagnosis.
	const int32 ActualThreads = static_cast<int32>(llama_n_threads(GRunner.Context));
	const int32 ActualThreadsBatch = static_cast<int32>(llama_n_threads_batch(GRunner.Context));
	const int32 RequestedThreads = static_cast<int32>(ContextParams.n_threads);
	const int32 RequestedThreadsBatch = static_cast<int32>(ContextParams.n_threads_batch);
	const int32 PhysicalCores = FPlatformMisc::NumberOfCores();

	GRunner.LastPromptTokens.Reset();
	GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Idle));

	if (GRunner.ContextSize != Options.ContextTokens
		|| GRunner.BatchSize != static_cast<int32>(ContextParams.n_batch)
		|| ActualUBatch != Options.UBatch)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: THE CONTEXT CLAMPED A REQUEST. requested ctx=%d batch=%d ubatch=%d -> actual n_ctx=%d n_batch=%d n_ubatch=%d. Every budget check and every prefill slice below uses the ACTUAL values; quote those in the report."),
			TagWarn, Options.ContextTokens, static_cast<int32>(ContextParams.n_batch), Options.UBatch,
			GRunner.ContextSize, GRunner.BatchSize, ActualUBatch);
	}

	// A SEPARATE line from the clamp warning above, deliberately: this one names
	// the physical core count, because "clamped" is only meaningful against it.
	if (ActualThreads != RequestedThreads || ActualThreadsBatch != RequestedThreadsBatch)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: THE CONTEXT CLAMPED THE THREAD REQUEST. requested n_threads=%d n_threads_batch=%d -> ACTUAL n_threads=%d n_threads_batch=%d, on a machine with %d physical core(s). Quote the ACTUAL figures. If the actual generation thread count is far below the core count, the cpu tier's latency is a CONFIGURATION ROOT CAUSE and not a hardware wall -- that is a bug to fix, not a requirement to renegotiate."),
			TagWarn, RequestedThreads, RequestedThreadsBatch, ActualThreads, ActualThreadsBatch, PhysicalCores);
	}

	// ⚠️ EVERY NEGOTIATED FIGURE ON THIS LINE IS NOW `actual(req N)`, AND THE ONE
	// THAT CANNOT BE IS LABELLED `(req)` RATHER THAN LEFT TO READ AS A
	// MEASUREMENT (TASK-429's sweep of the WARN-2 family):
	//   ctx / batch / ubatch / threads / threads_batch -- ACTUAL, queried.
	//   gpulayers -- REQUEST ONLY. The vendored header exposes n_gpu_layers as an
	//     INPUT field (llama.h:313) and ships no getter for how many layers were
	//     actually placed, so there is nothing to query and the honest thing is to
	//     say which one this is. Do NOT read `gpulayers=-1(req)` as "36 layers
	//     landed on the device"; -1 is the request "all of them".
	//   cores -- FPlatformMisc::NumberOfCores(), printed because "the thread count
	//     is clamped" is not a statement until there is something to clamp against,
	//     and because TASK-431 is asked to run a cell at the physical core count.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s tier=%s gpulayers=%d(req)/%d ctx=%d(req %d) batch=%d(req %d) ubatch=%d(req %d) threads=%d(req %d) threads_batch=%d(req %d) cores=%d load_ms=%.0f model=%s size=%s device=%s%s"),
		TagLoad, *Options.TierLabel, Options.GpuLayers, GRunner.ModelLayerCount,
		GRunner.ContextSize, Options.ContextTokens,
		GRunner.BatchSize, static_cast<int32>(ContextParams.n_batch),
		ActualUBatch, Options.UBatch,
		ActualThreads, RequestedThreads,
		ActualThreadsBatch, RequestedThreadsBatch,
		PhysicalCores, LoadMs,
		*FPaths::GetCleanFilename(ModelPath), *FormatMiB(GRunner.ModelSizeBytes),
		*GRunner.OffloadDeviceLabel,
		GRunner.OffloadDevice == nullptr
			? TEXT(" (NOT PINNED -- llama defaulted to all available devices)")
			: (GRunner.bOffloadDeviceAssumed ? TEXT(" (ASSUMED: first discrete GPU, no gpu= given)") : TEXT(" (pinned by gpu=)")));

	TArray<FSpikeDeviceInfo> DevicesAfter;
	EnumerateDevices(DevicesAfter);
	LogDeviceInventory(TEXT("devices_after_load"), DevicesAfter, GRunner.OffloadDevice);

	// The MODEL's own VRAM/RSS contribution is this DELTA, not the absolute
	// figure -- the absolute one includes everything the editor and the game
	// already had resident. ⚠️ BOTH SAMPLES ARE OF THE SAME PINNED DEVICE, and
	// vram_dev= names it: a delta taken across two different cards is not a
	// footprint, and bar #4's whole question is which card the weights are on.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s stage=model_load vram_dev=%s vram_free_before=%s vram_free_after=%s vram_delta=%s rss_before=%s rss_after=%s rss_delta=%s vram_total=%s"),
		TagMem, *GRunner.OffloadDeviceLabel,
		*FormatVram(BeforeLoad.VramFreeBytes, BeforeLoad.bHasDevice),
		*FormatVram(AfterLoad.VramFreeBytes, AfterLoad.bHasDevice),
		*FormatVram(BeforeLoad.VramFreeBytes > AfterLoad.VramFreeBytes ? BeforeLoad.VramFreeBytes - AfterLoad.VramFreeBytes : 0,
			BeforeLoad.bHasDevice && AfterLoad.bHasDevice),
		*FormatMiB(BeforeLoad.RssBytes), *FormatMiB(AfterLoad.RssBytes),
		*FormatMiB(AfterLoad.RssBytes > BeforeLoad.RssBytes ? AfterLoad.RssBytes - BeforeLoad.RssBytes : 0),
		*FormatVram(AfterLoad.VramTotalBytes, AfterLoad.bHasDevice));

	return true;
}

/** Reads the Zone-A override file, or builds the transcribed constant. */
static FString ResolveZoneAText(const FSpikeOptions& Options)
{
	if (!Options.ZoneAOverridePath.IsEmpty())
	{
		FString Override;
		if (FFileHelper::LoadFileToString(Override, *Options.ZoneAOverridePath))
		{
			UE_LOG(LogSiegeLlama, Display,
				TEXT("%s: Zone A loaded from '%s' (%d chars) -- the transcribed constant in SiegeLlamaSpike.cpp was NOT used."),
				TagRun, *Options.ZoneAOverridePath, Override.Len());
			return Override;
		}

		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: could not read prompt='%s' -- falling back to the transcribed Zone A constant."),
			TagWarn, *Options.ZoneAOverridePath);
	}

	FString ZoneA;
	AppendZoneA(ZoneA);
	return ZoneA;
}

// ---------------------------------------------------------------------------
// 7a. The bench job (bars #1, #2, #3, #4)
// ---------------------------------------------------------------------------

/**
 *  Utterances used for the latency and KV-reuse measurements. Deliberately NOT
 *  corpus sentences -- the bench must not become a second, unscored evaluation,
 *  and no corpus row may leak into a tuning surface.
 *
 *  ⚠️ INDEX 4 WAS REWRITTEN (qa/TASK-412.md WARN-1). It read
 *  "everyone pull back to our castle", which QA found to be BYTE-IDENTICAL to a
 *  sentence in the sealed holdout -- a natural derivation from the vocabulary's
 *  own aliases (fallback <- pull back, own_castle <- our castle) rather than a
 *  leak, but a bench run prints the model's raw answer to it, which is an
 *  incidental pre-exposure of a sealed row before the holdout is opened. The
 *  replacement keeps the row's JOB (army-wide intent + a home place, the only
 *  who="none" shape in this list) and is deliberately long and compound so it
 *  cannot plausibly collide with a short authored corpus sentence.
 *
 *  ⚠️ I CANNOT CHECK THE REPLACEMENT AGAINST THE HOLDOUT AND DID NOT TRY -- the
 *  sealed file is not mine to open (CONVENTIONS section 11). It IS checked
 *  against the dev split, which it does not match. If TASK-413 finds it collides
 *  after all, the bench is unscored so no number is invalidated; change the
 *  string and re-run.
 */
static const TCHAR* const SpikeBenchUtterances[] =
{
	TEXT("send 12 footmen and 2 archers with a sorcerer to the far ancient ground and wait until i have 4 knights"),
	TEXT("guard the nearest mine with 6 pikemen and 3 archers"),
	TEXT("ambush the middle with 5 cavalry and 2 wizards"),
	TEXT("send 20 footmen 3 clerics and a knight to their castle"),
	TEXT("whole army disengage from the middle and regroup at the home keep")
};

static constexpr int32 SpikeBenchUtteranceCount = static_cast<int32>(UE_ARRAY_COUNT(SpikeBenchUtterances));

/**
 *  Runs one cold turn 1 and one warm turn 2 and reports the pair.
 *
 *  @param BoundName        BEST_CASE or SHIPPED_WORST_CASE -- printed, not implied.
 *  @param Turn2Fixture     the world state turn 2 sees. The SAME fixture as turn 1
 *                          diverges in the `order:` line (best case); a DIFFERENT
 *                          one diverges at the start of Zone B (what ships).
 *  @param bExpectZoneBDivergence  true when this bound CLAIMS to be the shipped
 *                          shape, so a reuse point that reaches Zone C is a defect
 *                          in the fixtures rather than a good result.
 *  @param OutTurn2         turn 2's timings, so the caller can quote the warm TTFT
 *                          that belongs to each bound.
 *
 *  @return TRUE only when BOTH turns completed normally, i.e. only when this
 *          bound's numbers are bar #3 numbers at all. ⚠️ THE CALLER MUST NOT
 *          PRINT THIS BOUND'S FIGURES AS A RESULT WHEN IT IS FALSE.
 *
 *  ⛔ WARN-R2, AND WHY IT IS A FIX RATHER THAN ANOTHER WARNING (TASK-429;
 *     CONVENTIONS section 12e; manager ruling 10). This function used to ignore
 *     bDecodeFailed/bAborted entirely, so a turn 1 that aborted produced:
 *
 *       turn 1 stops early -> the prefill-failure path resets LastPromptTokens
 *       -> turn 2 finds a zero-length previous prompt -> CommonPrefix = 0
 *       -> turn 2 re-prefills the WHOLE prompt -> DROP computes to ~0 %
 *       -> DROP < 60 -> "THE PROMPT LAYOUT IS WRONG AND MUST BE FIXED BEFORE
 *          ANYTHING ELSE IS BUILT", at Warning volume, naming the wrong component.
 *
 *     It fired twice (TASK-413 PART 2 and PART 3's cpu tier), was checked twice,
 *     and was a decode abort BOTH times -- while the SAME run's
 *     SHIPPED_WORST_CASE bound sat healthy at 77.1 %. A diagnostic that
 *     confidently accuses the wrong component is worse than silence.
 *
 *     ⇒ THE LAYOUT VERDICT MAY ONLY BE EMITTED FOR A BOUND THAT COMPLETED BOTH
 *       TURNS. An incomplete bound reports the abort instead -- the llama_decode
 *       return code and the offset/token it stopped at -- and renders no
 *       judgement about the prompt at all.
 *
 *     ⚠️ AND THE OTHER DIRECTION IS PRESERVED ON PURPOSE: a bound that DID
 *       complete both turns and still reused little is exactly what that line is
 *       for, and it still shouts. The gate is completion, never the drop value.
 */
static bool RunPrefillBound(const FSpikeOptions& Options, const FString& ZoneAText,
	const TCHAR* BoundName, const TCHAR* Expectation,
	const FSpikeWorldFixture& Turn1Fixture, const FSpikeWorldFixture& Turn2Fixture,
	bool bExpectZoneBDivergence, FGenerationResult& OutTurn2)
{
	// Turn 1 is forced cold by clearing the whole cache, so the drop is measured
	// against a genuine full prefill and not against whatever happened to be
	// resident from the previous bound.
	llama_memory_clear(llama_get_memory(GRunner.Context), true);
	GRunner.LastPromptTokens.Reset();

	FString Prompt;
	FSpikePromptLayout Layout;
	BuildPrompt(Options, ZoneAText, Turn1Fixture, SpikeBenchUtterances[0], FString(), Prompt, &Layout);

	FGenerationResult Turn1;
	RunGeneration(Options, Prompt, Turn1Fixture, Turn1);

	BuildPrompt(Options, ZoneAText, Turn2Fixture, SpikeBenchUtterances[1], FString(), Prompt, &Layout);
	RunGeneration(Options, Prompt, Turn2Fixture, OutTurn2);

	const double DropPercent = (Turn1.PrefillTokens > 0)
		? 100.0 * (1.0 - static_cast<double>(OutTurn2.PrefillTokens) / static_cast<double>(Turn1.PrefillTokens))
		: 0.0;

	// WHERE the reuse landed, against the zone boundaries of turn 2's own prompt.
	const FSpikeDivergence Divergence = ClassifyDivergence(Prompt, Layout, OutTurn2.PrefixReused);

	// --- WARN-R2's GATE -----------------------------------------------------
	// The ONE predicate every judgement below hangs on. Read once, named once.
	const bool bBoundCompleted = Turn1.CompletedNormally() && OutTurn2.CompletedNormally();

	// ⚠️ THE DROP FIELD IS BUILT, NOT INLINED, BECAUSE IT SAYS THREE DIFFERENT
	// THINGS. The number is still PRINTED when the bound did not complete -- a
	// datum withheld is its own kind of lie, and a reader who understands the
	// failure can still use it -- but it is stripped of the Expectation string
	// and explicitly carries "not a bar #3 result", because a bare `DROP=0.0%`
	// beside a bar name is precisely what got quoted as a layout finding twice.
	//
	// The two incomplete cases are NOT the same and are not described as such:
	//   turn 1 failed  -> the drop is an ARTIFACT. Turn 1's failure cleared the
	//                     cache basis, so turn 2 re-prefilled everything and the
	//                     ~0 % has nothing whatever to do with the prompt layout.
	//   turn 1 fine,
	//   turn 2 failed  -> the drop is genuinely MEASURED (PrefillTokens and
	//                     PrefixReused are both fixed before the first decode),
	//                     but the bound still did not complete, so it renders no
	//                     verdict. Conservative on purpose, and the reason is
	//                     stated rather than hidden behind the same wording.
	//
	// NOTE: Expectation and FailureDetail are ARGUMENTS for %s, never format
	// strings, so a bare `%` inside either is correct and must NOT be doubled.
	FString DropField;
	if (bBoundCompleted)
	{
		DropField = FString::Printf(TEXT("DROP=%.1f%% (%s)"), DropPercent, Expectation);
	}
	else if (!Turn1.CompletedNormally())
	{
		DropField = FString::Printf(
			TEXT("DROP=%.1f%% [NOT A MEASUREMENT -- turn 1 did not complete (%s), so turn 2 re-prefilled from a cleared cache basis. This figure is an artifact of the abort and is NOT a bar #3 result. NO LAYOUT VERDICT IS RENDERED]"),
			DropPercent, *Turn1.FailureDetail);
	}
	else
	{
		DropField = FString::Printf(
			TEXT("DROP=%.1f%% [NO VERDICT -- the drop itself is measured, but turn 2 did not complete (%s), so this bound is NOT a bar #3 result]"),
			DropPercent, *OutTurn2.FailureDetail);
	}

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s tier=%s bound=%s status=%s fixtures=%s->%s turn1_prompt=%d turn1_prefill=%d | turn2_prompt=%d turn2_prefill=%d turn2_ttft_ms=%.1f | %s | %s"),
		TagPrefill, *Options.TierLabel, BoundName,
		bBoundCompleted ? TEXT("COMPLETED_BOTH_TURNS") : TEXT("INCOMPLETE"),
		Turn1Fixture.Label, Turn2Fixture.Label,
		Turn1.PromptTokens, Turn1.PrefillTokens,
		OutTurn2.PromptTokens, OutTurn2.PrefillTokens, OutTurn2.TtftMs,
		*FormatDivergence(Divergence), *DropField);

	// ⛔ THE SUPPRESSION. Everything past this point is a JUDGEMENT ABOUT THE
	// PROMPT, and a bound that did not run cannot support one. What it gets
	// instead is the abort, named -- which is the finding it actually has.
	if (!bBoundCompleted)
	{
		// Named locals rather than ternaries inside the argument list: an FString
		// temporary built in a UE_LOG argument is a lifetime question nobody
		// should have to answer while reading a diagnostic (the FixedLengthNote
		// precedent below).
		const FString Turn1Status = Turn1.CompletedNormally() ? FString(TEXT("completed")) : Turn1.FailureDetail;
		const FString Turn2Status = OutTurn2.CompletedNormally() ? FString(TEXT("completed")) : OutTurn2.FailureDetail;
		const FString DeadlineRegime = FormatDeadlineRegime(Options);

		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: bound=%s DID NOT COMPLETE BOTH TURNS, so ITS LAYOUT VERDICT IS SUPPRESSED -- this is NOT a bar #3 pass or fail, it is a generation that stopped. turn1: %s | turn2: %s | %s. THE FINDING HERE IS THE ABORT, NOT THE PROMPT LAYOUT: fix the stop (raise deadline=, or run a tier that finishes) and re-run before reading anything about bar #3 from this bound."),
			TagWarn, BoundName, *Turn1Status, *Turn2Status, *DeadlineRegime);
		return false;
	}

	// ⚠️ TWO GUARDS, AND THE SECOND ONE IS THE NEW ONE. The old code warned only
	// when the reuse was too LOW and was silent when it was implausibly HIGH --
	// which is the direction that flatters the result, and is exactly how a
	// single-fixture bench reported ~97 % against a shipped ~78 %.
	if (bExpectZoneBDivergence && Divergence.bResolved && Divergence.bLandedInZoneCTail)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: bound=%s reused %d tokens, which is at or past the start of Zone C (~%d) -- the two turns did not differ before the roster, so THIS IS NOT THE SHIPPED WORST CASE and DROP=%.1f%% overstates the reuse a live board would get. Check that the two fixtures actually differ."),
			TagWarn, BoundName, OutTurn2.PrefixReused, Divergence.ZoneCStartTokens, DropPercent);
	}

	// ⚠️ REACHED ONLY BY A BOUND THAT COMPLETED BOTH TURNS -- which is exactly
	// when this line means what it says. A real layout defect (both turns ran,
	// reuse was genuinely poor) still lands here at full volume; that direction
	// is not weakened by the gate above and must not be.
	if (DropPercent < 60.0)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: bound=%s KV-prefix reuse is only %.1f%%, and BOTH TURNS COMPLETED -- so this is a real reuse figure, not an abort artifact. Per the plan, if bar #3 fails THE PROMPT LAYOUT IS WRONG AND MUST BE FIXED BEFORE ANYTHING ELSE IS BUILT -- do not read the latency numbers as final until it passes."),
			TagWarn, BoundName, DropPercent);
	}

	return true;
}

static void RunBenchJob(const FSpikeOptions& InOptions)
{
	// ⚠️ A LOCAL COPY, because the bench derives one option the args cannot set:
	// with grammar=0 the decode must ignore EOG or the "fixed-length control" is
	// not fixed length (qa/TASK-412.md WARN-10). The eval job deliberately does
	// NOT get this treatment.
	FSpikeOptions Options = InOptions;
	Options.bIgnoreEogForFixedLength = !InOptions.bUseGrammar;

	if (!EnsureModelLoaded(Options))
	{
		return;
	}

	const FString ZoneAText = ResolveZoneAText(Options);

	// ⚠️ REWORDED AT TASK-478 BECAUSE THE OLD SENTENCE NAMED A DEAD MECHANISM
	// (CONVENTIONS section 22 -- a false claim in a runtime log string is believed
	// over the code). It used to read "the GBNF's kinds come from SpikeRoster; a
	// fixture that drifted from it would show the model a symbol the sampler
	// forbids." The GBNF's kinds now come from THE FIXTURE, so the danger inverted:
	// a fixture naming a kind SpikeRoster does not have would ADD that symbol to the
	// grammar and let the sampler emit a unit the game does not own.
	FString KindMismatch;
	if (!VerifyFixtureKindParity(SpikeFixtureT0, KindMismatch) || !VerifyFixtureKindParity(SpikeFixtureT1, KindMismatch))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: A BENCH FIXTURE IS NOT A VALID SUBSET OF SpikeRoster (%s). The grammar is generated from the fixture, so this run may admit a unit symbol the game does not have -- fix the fixture before quoting any number from it."),
			TagWarn, *KindMismatch);
	}

	// ⛔ SILENT ON t0 AND t1 BY CONSTRUCTION -- see ReportZoneAKindSeam. Both bench
	// fixtures carry all thirteen kinds (t1's count is bound to t0's by static_assert),
	// so neither call can emit a line and the bench's default output cannot move.
	// It first speaks on a Stage D subset fixture, which is the point.
	ReportZoneAKindSeam(ZoneAText, SpikeFixtureT0, TEXT("bench"), /*bReportWhenClosed*/ false);
	ReportZoneAKindSeam(ZoneAText, SpikeFixtureT1, TEXT("bench"), /*bReportWhenClosed*/ false);

	// --- BAR #3: TWO BOUNDS, NOT ONE NUMBER ---------------------------------
	// ⚠️ WHY THERE ARE TWO (qa/TASK-412.md BLOCKER-1). Turn 1 and turn 2 used to
	// share ONE fixture, so the only difference between the prompts was the
	// `order:` line -- the LAST line of Zone C. The reuse point therefore landed
	// in Zone C's tail and the drop read ~97-98 %, against a shipped figure that
	// CONVENTIONS section 8 records as 165/765 = 78 %. Worse, bar #2's "warm"
	// TTFT inherited it and was measured ~245 tokens of prefill short of what
	// ships, which on the CPU tier is hundreds of milliseconds off the number
	// Jonathan's tolerance is judged against.
	//
	// So: BEST_CASE is a paused board (nothing moved between turns, only the
	// order changed) and SHIPPED_WORST_CASE is a live board (the whole snapshot
	// moved, so the divergence is at the start of Zone B). The truth for any real
	// turn is between them, and BOTH are printed. ⚠️ THE BAR IS JUDGED ON
	// SHIPPED_WORST_CASE.
	FGenerationResult Turn2Best;
	const bool bBestCaseCompleted = RunPrefillBound(Options, ZoneAText, TEXT("BEST_CASE"),
		TEXT("bar #3 UPPER bound -- a PAUSED board: same fixture both turns, so only the order line differs. The shipped path cannot reach this; do NOT quote it as the result"),
		SpikeFixtureT0, SpikeFixtureT0, /*bExpectZoneBDivergence*/ false, Turn2Best);

	FGenerationResult Turn2Worst;
	const bool bShippedCaseCompleted = RunPrefillBound(Options, ZoneAText, TEXT("SHIPPED_WORST_CASE"),
		// NOTE: this string is an ARGUMENT for %s, never a format string, so a bare
		// `%` in it is correct and must NOT be doubled.
		TEXT("bar #3 THE NUMBER -- a LIVE board: every Zone B key and the roster moved between turns, so only Zone A is reused. Target ~70%, CONVENTIONS section 8 reference 165/765 = 78%"),
		SpikeFixtureT0, SpikeFixtureT1, /*bExpectZoneBDivergence*/ true, Turn2Worst);

	// ⚠️ THE VALIDITY TRAVELS WITH THE SUMMARY LINE (TASK-429). Suppressing the
	// verdict inside RunPrefillBound is not enough on its own: this line reprints
	// both bounds' figures in one place, and it is the line a report is most
	// likely to be copied from. A bound that did not complete is named here too,
	// or the suppression is undone one line later.
	FString BoundsValidity;
	if (!bBestCaseCompleted || !bShippedCaseCompleted)
	{
		BoundsValidity = FString::Printf(
			TEXT(" -- NOT A BAR #3 RESULT: %s%sdid not complete both turns (see the SPIKE_WARN line above for the llama_decode code and where it stopped). Fix the abort and re-run; do NOT quote these as bar #3 figures."),
			bBestCaseCompleted ? TEXT("") : TEXT("BEST_CASE "),
			bShippedCaseCompleted ? TEXT("") : TEXT("SHIPPED_WORST_CASE "));
	}

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s tier=%s BAR#3 BOUNDS best_case_warm_ttft_ms=%.1f shipped_worst_case_warm_ttft_ms=%.1f best_case_turn2_prefill=%d shipped_turn2_prefill=%d -- QUOTE THE SHIPPED FIGURE. The gap between them IS the cost of a board that moved.%s"),
		TagPrefill, *Options.TierLabel,
		Turn2Best.TtftMs, Turn2Worst.TtftMs, Turn2Best.PrefillTokens, Turn2Worst.PrefillTokens,
		*BoundsValidity);

	// --- BARS #1, #2, #4: the warm-prefix iterations -------------------------
	// ⚠️ THE FIXTURE ALTERNATES EVERY ITERATION, DELIBERATELY. Bar #2 is the bar
	// the feature lives or dies on, and a warm turn that only re-prefills the
	// order line is not the turn that ships. Alternating t0/t1 makes EVERY
	// iteration pay the full Zone B + Zone C re-prefill, so mean/worst TTFT here
	// are the SHIPPED-shaped figures. The best case is still on the record above.
	double WorstWallMs = 0.0;
	double TotalWallMs = 0.0;
	double TotalTtftMs = 0.0;
	int32 TotalOutputTokens = 0;
	int32 TotalPrefillTokens = 0;
	uint64 VramLowWater = MAX_uint64;
	bool bAnyVramDevice = false;

	// ⚠️ COUNTED, NOT LEFT TO THE READER TO TALLY FROM THE PER-ITERATION LINES
	// (TASK-429). "2 of 5 generations aborted" is the single most consequential
	// fact the cpu tier produced, and it was only ever recoverable by counting
	// ` ABORTED` suffixes by eye across five lines.
	int32 IncompleteIterations = 0;
	FString LastFailureDetail = TEXT("<none recorded>");

	FString Prompt;
	for (int32 Iteration = 0; Iteration < Options.Iterations; ++Iteration)
	{
		const TCHAR* Utterance = SpikeBenchUtterances[Iteration % SpikeBenchUtteranceCount];
		const FSpikeWorldFixture& Fixture = (Iteration % 2) == 0 ? SpikeFixtureT0 : SpikeFixtureT1;

		FSpikePromptLayout Layout;
		BuildPrompt(Options, ZoneAText, Fixture, Utterance, FString(), Prompt, &Layout);

		FGenerationResult Result;
		RunGeneration(Options, Prompt, Fixture, Result);

		WorstWallMs = FMath::Max(WorstWallMs, Result.WallMs);
		TotalWallMs += Result.WallMs;
		TotalTtftMs += Result.TtftMs;
		TotalOutputTokens += Result.OutputTokens;
		TotalPrefillTokens += Result.PrefillTokens;

		if (!Result.CompletedNormally())
		{
			++IncompleteIterations;
			if (!Result.FailureDetail.IsEmpty())
			{
				LastFailureDetail = Result.FailureDetail;
			}
		}
		if (Result.bHasVramDevice)
		{
			bAnyVramDevice = true;
			VramLowWater = FMath::Min(VramLowWater, Result.VramFreeLowWaterBytes);
		}

		const FSpikeDivergence Divergence = ClassifyDivergence(Prompt, Layout, Result.PrefixReused);

		// Built as a named local rather than inline: a *FString::Printf() inside a
		// ternary in the argument list is a temporary whose lifetime nobody should
		// have to reason about while reading a log line.
		const FString FixedLengthNote = Result.bEogIgnored
			? FString::Printf(TEXT(" FIXED_LENGTH_CONTROL(grammar=0: EOG IGNORED at out token %d -- the tokens after it are timing filler, NOT an answer)"), Result.EogTokenIndex)
			: FString();

		// ⚠️ THE SUFFIX NOW CARRIES THE REASON (TASK-429). ` ABORTED` alone told a
		// reader that something stopped but not WHAT stopped it, and the cpu tier's
		// "2 of 5 aborted with truncated JSON" is the finding this whole diagnosis
		// hangs on. The llama_decode code and the token index are what separate
		// "the ceiling cut a slow run" from "generation failed".
		const FString StopNote = Result.CompletedNormally()
			? FString()
			: FString::Printf(TEXT(" %s(%s)"),
				Result.bAborted ? TEXT("ABORTED") : TEXT("DECODE_FAILED"), *Result.FailureDetail);

		const double MsPerToken = Result.OutputTokens > 0 ? Result.DecodeMs / Result.OutputTokens : 0.0;

		// ⚠️ "60-token equivalent" is EXTRAPOLATED from the measured per-token
		// rate, because a grammar-constrained JSON command terminates when the
		// object closes and simply cannot be stretched to 60 tokens. The measured
		// figures are printed beside it so the extrapolation is auditable. With
		// grammar=0 the run IS 60 tokens (EOG is ignored), and eog_at= says where
		// the model would have stopped, so the control and the extrapolation can
		// be compared directly instead of one standing in for the other.
		const double SixtyTokenEquivalentMs = Result.TtftMs + 60.0 * MsPerToken;

		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s tier=%s iter=%d fixture=%s ttft_ms=%.1f prefill_ms=%.1f decode_ms=%.1f wall_ms=%.1f prefill_tok=%d %s out_tokens=%d ms_per_token=%.2f wall60_equiv_ms=%.0f%s%s"),
			TagLatency, *Options.TierLabel, Iteration, Fixture.Label,
			Result.TtftMs, Result.PrefillMs, Result.DecodeMs, Result.WallMs,
			Result.PrefillTokens, *FormatDivergence(Divergence),
			Result.OutputTokens, MsPerToken, SixtyTokenEquivalentMs, *FixedLengthNote, *StopNote);

		UE_LOG(LogSiegeLlama, Display, TEXT("%s   iter=%d output=%s"), TagLatency, Iteration, *Result.Text);
	}

	// ⚠️ THE SUMMARY CARRIES THE DEADLINE AND ITS REGIME (TASK-429). Every wall
	// time on this line is bounded by that ceiling, so a run at a non-default
	// deadline= is NOT comparable with a default one -- and the abort count says
	// how many of these iterations were cut rather than finished. Reporting a
	// mean wall over a set that includes truncated generations without saying so
	// is the measurement that lies.
	const int32 SafeIterations = FMath::Max(1, Options.Iterations);
	const FString DeadlineRegime = FormatDeadlineRegime(Options);
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s tier=%s SUMMARY iters=%d aborted_or_failed=%d/%d %s mean_ttft_ms=%.1f mean_wall_ms=%.1f WORST_wall_ms=%.1f mean_prefill_tok=%d total_out_tokens=%d (bar #2: <=2000ms partial, <=6000ms cpu -- measured with the fixture MOVING between turns, i.e. the shipped shape)"),
		TagLatency, *Options.TierLabel, Options.Iterations,
		IncompleteIterations, Options.Iterations, *DeadlineRegime,
		TotalTtftMs / SafeIterations, TotalWallMs / SafeIterations, WorstWallMs,
		TotalPrefillTokens / SafeIterations, TotalOutputTokens);

	if (IncompleteIterations > 0)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: %d of %d warm iteration(s) ABORTED OR FAILED at %s, so the mean/worst wall figures above are computed over a set that includes generations which were CUT, not finished -- and a cut generation emits TRUNCATED JSON, which is unusable rather than merely slow. Re-run with a larger deadline= to establish whether the ceiling is the binding constraint (a TUNABLE) or the generation genuinely fails (a WALL). Last recorded stop: %s"),
			TagWarn, IncompleteIterations, Options.Iterations, *DeadlineRegime, *LastFailureDetail);
	}

	const FSpikeMemorySnapshot Final = SampleMemory();
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s stage=bench_end tier=%s vram_dev=%s vram_free_lowwater=%s vram_free_now=%s vram_total=%s rss_now=%s"),
		TagMem, *Options.TierLabel, *GRunner.OffloadDeviceLabel,
		*FormatVram(VramLowWater, bAnyVramDevice && VramLowWater != MAX_uint64),
		*FormatVram(Final.VramFreeBytes, Final.bHasDevice),
		*FormatVram(Final.VramTotalBytes, Final.bHasDevice), *FormatMiB(Final.RssBytes));

	if (GRunner.OffloadDevice == nullptr || GRunner.bOffloadDeviceAssumed)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: bar #4's VRAM figures above are about %s, which was NOT named by the command line. Re-run with gpu=<index> from the SPIKE_MEM stage=devices_before_load list so the number has a stated subject."),
			TagWarn, *GRunner.OffloadDeviceLabel);
	}
}

// ---------------------------------------------------------------------------
// 7b. The eval job (bar #5)
// ---------------------------------------------------------------------------

/**
 *  ONE ROW'S OUTCOME IN ONE REPEAT -- the unit the stability report is built from.
 *
 *  ⚠️ THE SIGNATURE IS THE RAW GENERATED TEXT, COMPARED CASE-SENSITIVELY, AND THE
 *  CASE-SENSITIVITY IS DELIBERATE (CONVENTIONS section 13: FString comparison in
 *  this codebase has already produced a false "identical" once, because
 *  TestEqual on FString is case-INsensitive). A byte claim is made with a byte
 *  comparison. FString::Equals already defaults to CaseSensitive; it is passed
 *  explicitly so the next reader does not have to know that.
 *
 *  ⚠️ THE VERDICT IS COMPARED TOO EVEN THOUGH ScoreRow IS A PURE FUNCTION of
 *  (row, parsed output). Identical bytes MUST therefore yield an identical
 *  verdict, and if they ever do not, the scorer is carrying hidden state -- which
 *  would silently invalidate every accuracy number this harness has produced. The
 *  aggregate below reports that as an ERROR rather than assuming it cannot happen.
 */
struct FSplitRowObservation
{
	FString RawOutput;
	bool bPassLenient = false;
	bool bPassStrict = false;
	bool bParsed = false;

	bool SameBytesAs(const FSplitRowObservation& Other) const
	{
		return RawOutput.Equals(Other.RawOutput, ESearchCase::CaseSensitive);
	}

	bool SameVerdictAs(const FSplitRowObservation& Other) const
	{
		return bPassLenient == Other.bPassLenient
			&& bPassStrict == Other.bPassStrict
			&& bParsed == Other.bParsed;
	}
};

/** ONE REPEAT'S split-level totals, plus that repeat's per-row observations (parallel to the corpus). */
struct FSplitRunTotals
{
	int32 PassLenient = 0;
	int32 PassStrict = 0;
	int32 ParseFailures = 0;
	int32 QuestionPasses = 0;
	int32 DegenerateQuestionFloor = 0;
	TArray<FSplitRowObservation> Observations;
};

/**
 *  ⛔ THE AGGREGATION IS NAMED IN THE OUTPUT; IT IS NEVER LEFT FOR THE READER TO
 *  INFER. CONVENTIONS "THE FINE-TUNE RUNG" section 10 gate clause #1 scores the
 *  MINIMUM of N runs -- not the median, never the best -- while clause #6's dev
 *  regression rule scores the MEDIAN. Two clauses, two aggregations, one
 *  instrument: a printed number whose aggregation a reader has to guess is a
 *  DEFECT IN A GATE INSTRUMENT, not a formatting preference.
 *
 *  EVEN N: the median is the mean of the two middle values and can therefore be a
 *  half-integer. It is returned as a double and printed with one decimal for
 *  exactly that reason -- a silently floored 19.5 -> 19 would move a gate.
 */
static void SummariseIntSeries(const TArray<int32>& Values, int32& OutMin, double& OutMedian, int32& OutMax)
{
	OutMin = 0;
	OutMedian = 0.0;
	OutMax = 0;

	if (Values.Num() == 0)
	{
		return;
	}

	TArray<int32> Sorted = Values;
	Sorted.Sort();

	OutMin = Sorted[0];
	OutMax = Sorted[Sorted.Num() - 1];

	const int32 Middle = Sorted.Num() / 2;
	OutMedian = (Sorted.Num() % 2 == 1)
		? static_cast<double>(Sorted[Middle])
		: 0.5 * (static_cast<double>(Sorted[Middle - 1]) + static_cast<double>(Sorted[Middle]));
}

/** "[19,20,20,19,20]" -- the per-run series travels beside every aggregate, so the aggregation is checkable. */
static FString FormatIntSeries(const TArray<int32>& Values)
{
	FString Out = TEXT("[");
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		if (Index > 0)
		{
			Out += TEXT(",");
		}
		Out += FString::FromInt(Values[Index]);
	}
	Out += TEXT("]");
	return Out;
}

/**
 *  SCORES ONE PASS OVER ONE ALREADY-LOADED SPLIT.
 *
 *  ⛔ RepeatTag IS EMPTY ON THE DEFAULT PATH AND THAT IS WHAT KEEPS THIS EDIT
 *  ADDITIVE. It is spliced into the log lines as a bare %s immediately after
 *  `split=<name>`; an empty string changes not one byte of the line, so a
 *  repeats=1 run (i.e. every invocation that existed before this flag) prints
 *  character-for-character what it printed before. At repeats>1 it becomes
 *  " rep=2/5", which is what stops 125 identically-prefixed SPIKE_EVAL_ROW lines
 *  from being attributable only by counting.
 *
 *  ⚠️ THE CORPUS IS PASSED IN, NOT LOADED HERE (it used to be loaded here). N
 *  repeats have to be N replicates of the SAME rows: re-reading the CSV per
 *  repeat would let a mid-run edit turn a distribution into a comparison of two
 *  different corpora, and would reprint LoadCorpus's own warnings N times. The
 *  load and its "NOT SCORED" error moved UP to RunSplitRepeated verbatim, and it
 *  was the first statement in this function, so nothing prints out of order.
 */
static void RunOneSplit(const FSpikeOptions& Options, const FString& ZoneAText,
	const FString& SplitName, const TArray<FCorpusRow>& Rows,
	const FString& RepeatTag, FSplitRunTotals& OutTotals)
{
	OutTotals.Observations.Reset();
	OutTotals.Observations.Reserve(Rows.Num());

	int32 PassLenient = 0;
	int32 PassStrict = 0;
	int32 QuestionPasses = 0;
	int32 ParseFailures = 0;
	int32 TriggerAsserted = 0;
	int32 TriggerOk = 0;
	int32 TotalAssertionsCompared = 0;
	int32 SingleAssertionRows = 0;
	int32 ZeroAssertionRows = 0;
	int32 DegenerateQuestionFloor = 0;
	FString ThinRowIds;
	FString FreeRowIds;

	for (const FCorpusRow& Row : Rows)
	{
		// ⚠️ THE LENIENCY BOUND, COMPUTED RATHER THAN CLAIMED. A degenerate model
		// that answers {"ask":...} to EVERY sentence passes exactly the rows whose
		// ExpectOutcome is not Execute. Printing that floor beside the score is
		// what stops "the Clarify rule is lenient but bounded" from being a
		// promise the reader has to take on trust.
		if (!Row.ExpectOutcome.Equals(TEXT("Execute"), ESearchCase::IgnoreCase))
		{
			++DegenerateQuestionFloor;
		}

		// ⚠️ THE EVAL ALWAYS USES t0, AND THAT IS NOT AN ARBITRARY CHOICE. The
		// corpus rows were authored against t0's numbers -- DEV-03's "you asked for
		// 10 footmen and 8 exist" is only a shortfall while footman is 8, and
		// DEV-11's deferred trigger is only unsatisfied while knight is 3. Running
		// a split against t1 would silently rewrite the corpus's premises and score
		// the model on questions nobody wrote.
		FString Prompt;
		FSpikePromptLayout Layout;
		BuildPrompt(Options, ZoneAText, SpikeFixtureT0, Row.Sentence, FString(), Prompt, &Layout);

		// ⛔ THE SAME FIXTURE THAT BUILT THE PROMPT ONE LINE ABOVE, AND IT MUST STAY
		// THAT WAY: the GBNF's `kind` alternatives are now generated from it, so
		// passing a different fixture here would show the model one board and
		// constrain it to another -- a divergence with no log line.
		FGenerationResult Generation;
		RunGeneration(Options, Prompt, SpikeFixtureT0, Generation);

		// ⚠️ THE prefill=/reused= COLUMNS ARE NOT SHIPPED-SHAPED KV NUMBERS, and
		// now they say so rather than leaving the reader to assume. Every row in a
		// split shares one fixture, so consecutive prompts differ only in the
		// `order:` line and the reuse lands in ZONE C's tail every time. That is
		// the right thing for an ACCURACY run (it is faster and changes no answer),
		// but it is the BEST case for bar #3, not the shipped one -- the bench's
		// SHIPPED_WORST_CASE bound is the figure to quote.
		const FSpikeDivergence Divergence = ClassifyDivergence(Prompt, Layout, Generation.PrefixReused);

		FSpikeParsed Parsed;
		FString ParseError;
		const bool bParsed = ParseSpikeOutput(Generation.Text.TrimStartAndEnd(), Parsed, ParseError);

		FRowResult Result;
		Result.Id = Row.Id;
		Result.Outcome = Row.ExpectOutcome;
		Result.RawOutput = Generation.Text;
		Result.ParseError = ParseError;
		Result.WallMs = Generation.WallMs;
		Result.TtftMs = Generation.TtftMs;
		Result.PrefillTokens = Generation.PrefillTokens;
		Result.PrefixReused = Generation.PrefixReused;
		Result.OutputTokens = Generation.OutputTokens;

		ScoreRow(Row, Parsed, bParsed, Result);

		PassLenient += Result.bPassLenient ? 1 : 0;
		PassStrict += Result.bPassStrict ? 1 : 0;
		QuestionPasses += Result.bQuestionPassedAClarifyRow ? 1 : 0;
		ParseFailures += bParsed ? 0 : 1;
		TotalAssertionsCompared += Result.AssertionsCompared;

		// ⚠️ RECORDED ON EVERY RUN, INCLUDING repeats=1 (WHERE NOTHING EVER READS
		// IT). One traversal, one record: a stability report assembled by a second
		// pass over the corpus would be reporting about a different run than the
		// one that was scored, which is this project's exact recurring defect --
		// a confident green describing something else. RawOutput is the same
		// string the raw= column above prints, so the identity of a flip is
		// inspectable in the log rather than only summarised.
		FSplitRowObservation& Observation = OutTotals.Observations.AddDefaulted_GetRef();
		Observation.RawOutput = Generation.Text;
		Observation.bPassLenient = Result.bPassLenient;
		Observation.bPassStrict = Result.bPassStrict;
		Observation.bParsed = bParsed;

		if (Row.bTriggerAsserted)
		{
			++TriggerAsserted;
			TriggerOk += Result.bTriggerOk ? 1 : 0;
		}

		if (Row.AssertedFieldCount() <= 1)
		{
			++SingleAssertionRows;
			if (!ThinRowIds.IsEmpty())
			{
				ThinRowIds += TEXT(" ");
			}
			ThinRowIds += Row.Id;
		}

		if (Row.AssertedFieldCount() == 0)
		{
			++ZeroAssertionRows;
			if (!FreeRowIds.IsEmpty())
			{
				FreeRowIds += TEXT(" ");
			}
			FreeRowIds += Row.Id;
		}

		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s split=%s%s id=%-8s expect=%-8s got=%-8s lenient=%s strict=%s asserted=%d compared=%d skipped=%d intent=%s kinds=%s counts=%s where=%s fixture=%s prefill=%d %s wall_ms=%.0f"),
			TagEvalRow, *SplitName, *RepeatTag, *Row.Id, *Row.ExpectOutcome,
			bParsed ? (Parsed.bIsQuestion ? TEXT("question") : TEXT("command")) : TEXT("PARSEFAIL"),
			Result.bPassLenient ? TEXT("PASS") : TEXT("FAIL"),
			Result.bPassStrict ? TEXT("PASS") : TEXT("FAIL"),
			Row.AssertedFieldCount(), Result.AssertionsCompared, Result.AssertionsSkipped,
			Result.bIntentOk ? TEXT("ok") : TEXT("BAD"),
			Result.bKindsOk ? TEXT("ok") : TEXT("BAD"),
			Result.bCountsOk ? TEXT("ok") : TEXT("BAD"),
			Result.bWhereOk ? TEXT("ok") : TEXT("BAD"),
			SpikeFixtureT0.Label, Result.PrefillTokens, *FormatDivergence(Divergence), Result.WallMs);

		const FString ParseErrorSuffix = ParseError.IsEmpty()
			? FString()
			: FString::Printf(TEXT(" parse_error=%s"), *ParseError);

		// ⚠️ AN ABORTED ROW SCORES AS A WRONG ANSWER AND USED TO SAY NOTHING
		// (TASK-429). This is WARN-R2's defect in the accuracy lane: a generation
		// the ceiling CUT emits truncated JSON, fails to parse, and is then
		// indistinguishable in the log from a model that understood the sentence
		// and answered it wrongly -- so a ladder loop could be spent re-tuning a
		// prompt to fix what is actually a decode-rate problem. It is named here.
		const FString StopSuffix = Generation.CompletedNormally()
			? FString()
			: FString::Printf(TEXT(" ROW_DID_NOT_COMPLETE(%s -- this row scores as a WRONG ANSWER but is a STOPPED generation, not a comprehension failure; do not tune against it)"),
				*Generation.FailureDetail);

		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s   split=%s%s id=%s sentence=\"%s\" raw=%s%s%s"),
			TagEvalRow, *SplitName, *RepeatTag, *Row.Id, *Row.Sentence, *Generation.Text, *ParseErrorSuffix, *StopSuffix);
	}

	const double LenientPercent = 100.0 * PassLenient / FMath::Max(1, Rows.Num());
	const double StrictPercent = 100.0 * PassStrict / FMath::Max(1, Rows.Num());

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s%s rows=%d PRIMARY(lenient)=%d/%d = %.1f%% | STRICT=%d/%d = %.1f%% | parse_failures=%d | clarify_rows_passed_by_question=%d | assertions_compared=%d"),
		TagEvalScore, *SplitName, *RepeatTag, Rows.Num(),
		PassLenient, Rows.Num(), LenientPercent,
		PassStrict, Rows.Num(), StrictPercent,
		ParseFailures, QuestionPasses, TotalAssertionsCompared);

	if (TriggerAsserted > 0)
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s split=%s%s deferred-trigger agreement (NOT part of bar #5's four fields) = %d/%d"),
			TagEvalScore, *SplitName, *RepeatTag, TriggerOk, TriggerAsserted);
	}

	// The eval holds one fixture across every row, so its per-row reuse figures are
	// the BEST case by construction. Said once per split so no one lifts them into
	// the bar #3 slot of the report.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s%s NOTE: every row above ran against fixture %s, so the per-row prefill=/reused= columns are a CONSTANT-FIXTURE (best case) KV measurement and are NOT bar #3. Bar #3 is the bench's SHIPPED_WORST_CASE bound."),
		TagEvalScore, *SplitName, *RepeatTag, SpikeFixtureT0.Label);

	// ⚠️ THREE HONESTY LINES. They exist so nobody reads the headline percentage
	// as stronger than it is, and they are printed for BOTH splits.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s%s LENIENCY FLOOR: a degenerate model that answers {\"ask\":...} to EVERY sentence would score %d/%d = %.1f%% under the PRIMARY rule (it fails every Execute row). The bar is 85%% -- the floor is how much of the score is NOT evidence of translation."),
		TagEvalScore, *SplitName, *RepeatTag, DegenerateQuestionFloor, Rows.Num(),
		100.0 * DegenerateQuestionFloor / FMath::Max(1, Rows.Num()));

	if (SingleAssertionRows > 0)
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s split=%s%s %d row(s) assert at most ONE field and therefore test less than the score implies (CONVENTIONS section 11's overloaded-empty-cell limitation -- a row whose true answer is an EMPTY field cannot be positively asserted in v1): %s"),
			TagEvalScore, *SplitName, *RepeatTag, SingleAssertionRows, *ThinRowIds);
	}

	if (ZeroAssertionRows > 0)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s split=%s%s %d row(s) assert NOTHING AT ALL and pass on any parseable output. Subtract them before believing the headline: %s"),
			TagEvalScore, *SplitName, *RepeatTag, ZeroAssertionRows, *FreeRowIds);
	}

	// The totals this repeat contributes to the distribution. Assigned from the
	// same locals the lines above printed, so the aggregate cannot disagree with
	// the per-run report it summarises.
	OutTotals.PassLenient = PassLenient;
	OutTotals.PassStrict = PassStrict;
	OutTotals.ParseFailures = ParseFailures;
	OutTotals.QuestionPasses = QuestionPasses;
	OutTotals.DegenerateQuestionFloor = DegenerateQuestionFloor;
}

/**
 *  ⭐ THE REPEAT DRIVER -- CONVENTIONS section 12h (THE REPEAT LAW) MADE MECHANICAL.
 *
 *  Loops one split N times INSIDE THE SINGLE JOB it was already running in. No
 *  StartJob call is added, so queue depth stays 1 by design and the refusal
 *  section 12h recorded as a PASS is not weakened -- N readings now cost one
 *  queue slot, one model load and one session, which is also the only shape in
 *  which the sealed holdout can be repeated at all (section 12a: it opens once,
 *  so the repeat has to happen inside that opening).
 *
 *  ⛔ AT repeats=1 THIS FUNCTION PRINTS NOTHING OF ITS OWN. It loads the corpus
 *  (which RunOneSplit used to do), calls RunOneSplit once with an EMPTY repeat
 *  tag, and returns before the distribution block. That is the additive
 *  guarantee section 16 demands, and it is checkable by grep: a default run
 *  emits zero SPIKE_EVAL_REPEAT lines.
 */
static void RunSplitRepeated(const FSpikeOptions& Options, const FString& ZoneAText,
	const FString& SplitName, const FString& CorpusPath)
{
	// MOVED UP FROM RunOneSplit VERBATIM -- same tag, same text, same Error level.
	// It was that function's FIRST statement and nothing preceded it, so a failed
	// load prints exactly the line it printed before, in the same position.
	TArray<FCorpusRow> Rows;
	FString LoadError;
	if (!LoadCorpus(CorpusPath, Rows, LoadError))
	{
		UE_LOG(LogSiegeLlama, Error, TEXT("%s: split '%s' NOT SCORED -- %s"), TagEvalScore, *SplitName, *LoadError);
		return;
	}

	// Already validated in ParseOptions; clamped here only so a future caller that
	// builds FSpikeOptions directly cannot drive a negative loop count.
	const int32 Repeats = FMath::Clamp(Options.EvalRepeats, 1, SpikeMaxEvalRepeats);

	TArray<FSplitRunTotals> PerRepeat;
	PerRepeat.SetNum(Repeats);

	for (int32 RepeatIndex = 0; RepeatIndex < Repeats; ++RepeatIndex)
	{
		FString RepeatTag;
		if (Repeats > 1)
		{
			RepeatTag = FString::Printf(TEXT(" rep=%d/%d"), RepeatIndex + 1, Repeats);

			UE_LOG(LogSiegeLlama, Display,
				TEXT("%s ---- split=%s REPEAT %d of %d (rows=%d) ---- ONE job, ONE model load, ONE session: the loop is INSIDE the job, so the queue-depth-1 contract is untouched (CONVENTIONS section 12h recorded the harness REFUSING a concurrent job as a PASS, and nothing here weakens it)."),
				TagEvalRepeat, *SplitName, RepeatIndex + 1, Repeats, Rows.Num());
		}

		RunOneSplit(Options, ZoneAText, SplitName, Rows, RepeatTag, PerRepeat[RepeatIndex]);
	}

	// ⛔ THE DEFAULT PATH ENDS HERE. At repeats=1 this command has now printed
	// character-for-character what it printed before the flag existed.
	if (Repeats <= 1)
	{
		return;
	}

	// -----------------------------------------------------------------------
	// SPLIT-LEVEL DISTRIBUTION -- min / median / max, EACH LABELLED
	// -----------------------------------------------------------------------
	TArray<int32> LenientSeries;
	TArray<int32> StrictSeries;
	TArray<int32> ParseFailureSeries;
	LenientSeries.Reserve(Repeats);
	StrictSeries.Reserve(Repeats);
	ParseFailureSeries.Reserve(Repeats);

	for (const FSplitRunTotals& Totals : PerRepeat)
	{
		LenientSeries.Add(Totals.PassLenient);
		StrictSeries.Add(Totals.PassStrict);
		ParseFailureSeries.Add(Totals.ParseFailures);
	}

	int32 LenientMin = 0;
	int32 LenientMax = 0;
	double LenientMedian = 0.0;
	SummariseIntSeries(LenientSeries, LenientMin, LenientMedian, LenientMax);

	int32 StrictMin = 0;
	int32 StrictMax = 0;
	double StrictMedian = 0.0;
	SummariseIntSeries(StrictSeries, StrictMin, StrictMedian, StrictMax);

	int32 ParseFailMin = 0;
	int32 ParseFailMax = 0;
	double ParseFailMedian = 0.0;
	SummariseIntSeries(ParseFailureSeries, ParseFailMin, ParseFailMedian, ParseFailMax);

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s ======== split=%s REPEAT SUMMARY over %d runs (rows=%d) ========"),
		TagEvalRepeat, *SplitName, Repeats, Rows.Num());

	// ⛔ THE AGGREGATION IS NAMED IN THE LINE. Section 10 gate clause #1 scores the
	// MINIMUM of 5 runs -- never the median, never the best -- and clause #6's dev
	// regression rule scores the MEDIAN. The per-run series travels beside them so
	// the aggregation itself is checkable rather than trusted.
	//
	// ⚠️ AND THE LENIENCY FLOOR TRAVELS WITH THE NUMBER, EVERY TIME (section 12a).
	// An aggregate that dropped it would be the one number in this harness quoted
	// without the part of it that is not evidence of translation.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s LENIENT per_run=%s MIN=%d/%d MEDIAN=%.1f/%d MAX=%d/%d | leniency_floor=%d/%d -- READ THE MINIMUM for a gate: CONVENTIONS \"THE FINE-TUNE RUNG\" section 10 clause #1 scores the MINIMUM of the runs, clause #6's dev regression rule scores the MEDIAN, and NOTHING scores the maximum. MEDIAN of an even run count is the mean of the two middle values, hence one decimal."),
		TagEvalRepeat, *SplitName, *FormatIntSeries(LenientSeries),
		LenientMin, Rows.Num(), LenientMedian, Rows.Num(), LenientMax, Rows.Num(),
		PerRepeat[0].DegenerateQuestionFloor, Rows.Num());

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s STRICT  per_run=%s MIN=%d/%d MEDIAN=%.1f/%d MAX=%d/%d"),
		TagEvalRepeat, *SplitName, *FormatIntSeries(StrictSeries),
		StrictMin, Rows.Num(), StrictMedian, Rows.Num(), StrictMax, Rows.Num());

	// Gate clause #2 (STRICT == LENIENT in every run) is the anti-hedging clause,
	// so the runs where it does NOT hold are named rather than summarised.
	FString StrictLenientDisagreementRuns;
	for (int32 RepeatIndex = 0; RepeatIndex < Repeats; ++RepeatIndex)
	{
		if (PerRepeat[RepeatIndex].PassStrict != PerRepeat[RepeatIndex].PassLenient)
		{
			if (!StrictLenientDisagreementRuns.IsEmpty())
			{
				StrictLenientDisagreementRuns += TEXT(",");
			}
			StrictLenientDisagreementRuns += FString::FromInt(RepeatIndex + 1);
		}
	}

	const FString StrictLenientVerdict = StrictLenientDisagreementRuns.IsEmpty()
		? FString(TEXT("YES"))
		: FString::Printf(TEXT("NO -- disagreed in run(s) %s"), *StrictLenientDisagreementRuns);

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s strict_equals_lenient_in_every_run=%s (section 10 clause #2, the anti-hedging clause)"),
		TagEvalRepeat, *SplitName, *StrictLenientVerdict);

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s split=%s parse_failures per_run=%s MIN=%d MEDIAN=%.1f MAX=%d (section 10 clause #4 requires ZERO in EVERY run, so the figure that matters here is the MAXIMUM)"),
		TagEvalRepeat, *SplitName, *FormatIntSeries(ParseFailureSeries),
		ParseFailMin, ParseFailMedian, ParseFailMax);

	// ⛔ NUMBERS ONLY. This block does NOT print a gate verdict, and that is a
	// boundary rather than an omission: section 10's gate is CONJUNCTIVE over nine
	// clauses, five of which this command cannot see (the artifact's sha256, the
	// latency budget, the prompt-shape diff...). A harness that printed "GATE:
	// PASS" from the four clauses it can measure would be the project's recurring
	// defect -- a confident green describing something else.

	// -----------------------------------------------------------------------
	// PER-ROW STABILITY -- "THE IDENTITY OF THE UNSTABLE ROW IS THE DIAGNOSTIC,
	// NOT THE SCORE" (section 12h, verbatim)
	// -----------------------------------------------------------------------
	int32 UnstableRowCount = 0;
	FString UnstableRowIds;

	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		TArray<const FSplitRowObservation*> Series;
		Series.Reserve(Repeats);
		for (const FSplitRunTotals& Totals : PerRepeat)
		{
			if (Totals.Observations.IsValidIndex(RowIndex))
			{
				Series.Add(&Totals.Observations[RowIndex]);
			}
		}

		// Cannot happen while RunOneSplit visits every row unconditionally -- but a
		// stability claim made over a partial series would be a FALSE claim rather
		// than a missing one, so it is refused out loud instead of averaged over.
		if (Series.Num() != Repeats)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: split=%s id=%s contributed only %d of %d observations, so NO stability verdict is issued for it and it is NOT counted in UNSTABLE_ROWS below. Treat this split's stability report as incomplete."),
				TagWarn, *SplitName, *Rows[RowIndex].Id, Series.Num(), Repeats);
			continue;
		}

		int32 Flips = 0;
		int32 LenientPasses = 0;
		int32 StrictPasses = 0;
		bool bVerdictMovedWithoutBytes = false;

		TArray<FString> VariantTexts;   // distinct raw outputs, in first-seen order
		TArray<int32> VariantOfRun;     // run index -> variant index
		VariantOfRun.Reserve(Repeats);

		for (int32 RunIndex = 0; RunIndex < Series.Num(); ++RunIndex)
		{
			const FSplitRowObservation& Observation = *Series[RunIndex];

			LenientPasses += Observation.bPassLenient ? 1 : 0;
			StrictPasses += Observation.bPassStrict ? 1 : 0;

			int32 VariantIndex = INDEX_NONE;
			for (int32 Candidate = 0; Candidate < VariantTexts.Num(); ++Candidate)
			{
				// ⚠️ CaseSensitive PASSED EXPLICITLY (CONVENTIONS section 13). It is
				// the default, but this is a BYTE claim and a byte claim in this
				// codebase has already been made wrong once by an implicitly
				// case-insensitive comparison.
				if (VariantTexts[Candidate].Equals(Observation.RawOutput, ESearchCase::CaseSensitive))
				{
					VariantIndex = Candidate;
					break;
				}
			}

			if (VariantIndex == INDEX_NONE)
			{
				VariantIndex = VariantTexts.Add(Observation.RawOutput);
			}
			VariantOfRun.Add(VariantIndex);

			if (RunIndex > 0)
			{
				const FSplitRowObservation& Previous = *Series[RunIndex - 1];
				const bool bSameBytes = Observation.SameBytesAs(Previous);
				const bool bSameVerdict = Observation.SameVerdictAs(Previous);

				if (!bSameBytes || !bSameVerdict)
				{
					++Flips;
				}

				if (bSameBytes && !bSameVerdict)
				{
					bVerdictMovedWithoutBytes = true;
				}
			}
		}

		const bool bStable = (Flips == 0) && (VariantTexts.Num() == 1);

		if (bStable)
		{
			UE_LOG(LogSiegeLlama, Display,
				TEXT("%s split=%s id=%-8s stable=%d/%d (byte-identical output AND identical verdict in every run)"),
				TagEvalRepeat, *SplitName, *Rows[RowIndex].Id, Repeats, Repeats);
		}
		else
		{
			++UnstableRowCount;
			if (!UnstableRowIds.IsEmpty())
			{
				UnstableRowIds += TEXT(" ");
			}
			UnstableRowIds += Rows[RowIndex].Id;

			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s split=%s id=%-8s flips=%d variants=%d lenient_passes=%d/%d strict_passes=%d/%d MOVED -- flips counts run-to-run TRANSITIONS (0..N-1, so an A/B/A/B alternation reads higher than a single settle); variants counts DISTINCT outputs. THIS ROW'S IDENTITY IS THE READING, NOT THE SCORE (CONVENTIONS section 12h)."),
				TagEvalRepeat, *SplitName, *Rows[RowIndex].Id, Flips, VariantTexts.Num(),
				LenientPasses, Repeats, StrictPasses, Repeats);

			// The variants themselves, with the runs that produced them -- this is
			// what turns DEV-11's known flip between n:2 and n:all from an anecdote
			// somebody remembered into an instrument reading somebody can read.
			for (int32 VariantIndex = 0; VariantIndex < VariantTexts.Num(); ++VariantIndex)
			{
				FString RunsWithVariant;
				for (int32 RunIndex = 0; RunIndex < VariantOfRun.Num(); ++RunIndex)
				{
					if (VariantOfRun[RunIndex] == VariantIndex)
					{
						if (!RunsWithVariant.IsEmpty())
						{
							RunsWithVariant += TEXT(",");
						}
						RunsWithVariant += FString::FromInt(RunIndex + 1);
					}
				}

				UE_LOG(LogSiegeLlama, Warning,
					TEXT("%s   split=%s id=%s variant=%d/%d seen_in_runs=%s raw=%s"),
					TagEvalRepeat, *SplitName, *Rows[RowIndex].Id,
					VariantIndex + 1, VariantTexts.Num(), *RunsWithVariant, *VariantTexts[VariantIndex]);
			}
		}

		if (bVerdictMovedWithoutBytes)
		{
			UE_LOG(LogSiegeLlama, Error,
				TEXT("%s: split=%s id=%s produced BYTE-IDENTICAL output in two consecutive runs but a DIFFERENT pass/fail verdict. ScoreRow is a pure function of (corpus row, parsed output), so this is IMPOSSIBLE unless the scorer carries state -- and if it does, EVERY accuracy number this harness has ever produced is suspect. Do not report this run; diagnose the scorer first."),
				TagWarn, *SplitName, *Rows[RowIndex].Id);
		}
	}

	if (UnstableRowCount == 0)
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s split=%s repeats=%d UNSTABLE_ROWS=0/%d -- every row produced byte-identical output in all %d runs, which is why the per-run scores above are identical. A spread with no row named here would be a bug in this report."),
			TagEvalRepeat, *SplitName, Repeats, Rows.Num(), Repeats);
	}
	else
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s split=%s repeats=%d UNSTABLE_ROWS=%d/%d ids=%s -- THIS LIST IS THE INSTRUMENT READING. CONVENTIONS section 12h: a one-row delta is measured to sit INSIDE the instrument's own noise band, so no wave may claim a '+1 row' gain from a single run, and the spread travels with the number."),
			TagEvalRepeat, *SplitName, Repeats, UnstableRowCount, Rows.Num(), *UnstableRowIds);
	}

	// ⚠️ THE ONE CONFOUND IN THIS DESIGN, PRINTED RATHER THAN BURIED IN A HANDOFF.
	//
	// The KV cache is CHAINED across repeats and this change deliberately did not
	// touch that. RunGeneration reuses the token prefix it shares with the PREVIOUS
	// prompt, so the amount re-prefilled on a row depends on WHICH ROW RAN BEFORE
	// IT. Within a split that predecessor is fixed -- except for the first row,
	// whose predecessor in repeat 1 is whatever preceded the split and in repeats
	// 2..N is the split's own last row.
	//
	// ⛔ AND THE CONSEQUENCE IS NOT CONFINED TO THAT ONE ROW, WHICH IS WHY IT IS
	// STATED CAREFULLY RATHER THAN REASSURINGLY: row 2 reuses the cache row 1 left
	// behind, so a different re-prefill boundary on row 1 can carry down the split.
	// The claim that survives is the narrow one -- repeats 2..N are MUTUALLY
	// identical conditions; repeat 1 is the odd one out.
	//
	// Clearing the cache per repeat was rejected: it would make repeat 1 differ
	// from the run every number on record was taken with, which section 16 forbids
	// outright, and choosing the replicate design is M0's job (a corpus-free
	// determinism probe), not this flag's.
	UE_LOG(LogSiegeLlama, Warning,
		TEXT("%s split=%s CAVEAT -- THE KV CACHE IS CHAINED ACROSS REPEATS AND THAT WAS NOT CHANGED. Row 1 of repeat 1 follows whatever preceded this split (a fresh load, or the previous split); row 1 of repeats 2..%d follows the split's OWN last row, so its re-prefill boundary differs -- and because each row reuses the cache the previous row left, that difference can propagate DOWN the split rather than staying on row 1. => THE SOUND COMPARISON IS AMONG REPEATS 2..%d, WHICH ARE MUTUALLY IDENTICAL CONDITIONS. A row that differs ONLY between repeat 1 and the rest is NOT established as a model property; re-run with a higher N and read the 2..N agreement. The instrument's own noise floor is M0's job (a determinism probe with no corpus), not this flag's."),
		TagEvalRepeat, *SplitName, Repeats, Repeats);
}

static void RunEvalJob(const FSpikeOptions& Options)
{
	if (!EnsureModelLoaded(Options))
	{
		return;
	}

	const FString ZoneAText = ResolveZoneAText(Options);

	// ⚠️ ADDED AT TASK-478 AS A SECTION 22 SWEEP RESULT, NOT AS A NEW IDEA. The
	// parity check existed on the bench lane and on Siege.Llama.SpikePrompt and was
	// MISSING from the eval lane -- the one lane that produces the go/no-go number.
	// A citation is a lower bound on the extent of a defect, so the shape was swept
	// for rather than the two known sites trusted as the whole set.
	//
	// ⛔ BOTH CALLS ARE SILENT TODAY AND THAT IS PROVABLE, NOT HOPED FOR: t0's Roster
	// member IS SpikeRoster, so parity holds by identity and no kind can be absent.
	// ⚠️ Per CONVENTIONS section 32 this guard has therefore NEVER BEEN OBSERVED TO
	// FIRE and is not known to function -- it is an assertion that t0's frozen bytes
	// have not moved, and the only thing that could ever trip it is exactly that.
	FString EvalKindMismatch;
	if (!VerifyFixtureKindParity(SpikeFixtureT0, EvalKindMismatch))
	{
		UE_LOG(LogSiegeLlama, Error,
			TEXT("%s: THE EVAL FIXTURE t0 IS NOT A VALID SUBSET OF SpikeRoster (%s). t0's bytes are FROZEN (CONVENTIONS section 16) because the sealed corpus was authored against them, so this means the fixture was edited -- every row scored in this run is being asked a question nobody wrote. Reconcile before quoting any number."),
			TagWarn, *EvalKindMismatch);
	}

	ReportZoneAKindSeam(ZoneAText, SpikeFixtureT0, TEXT("eval"), /*bReportWhenClosed*/ false);

	if (!Options.DevCorpusPath.IsEmpty())
	{
		RunSplitRepeated(Options, ZoneAText, TEXT("dev"), Options.DevCorpusPath);
	}

	if (!Options.HoldoutCorpusPath.IsEmpty())
	{
		// ⚠️ THE HOLDOUT OPENS EXACTLY ONCE, AT TASK-413, AND ITS NUMBER ALONE
		// SCORES THE >= 85 % BAR. A dev score quoted as the bar is a QA FAIL --
		// the plan's remediation ladder begins "better few-shots", i.e. tuning
		// against dev, so a dev score is a fitted number by construction.
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s: OPENING THE HOLDOUT. Report the HOLDOUT number against the >= 85%% bar, never the dev number, and DO NOT TUNE ANYTHING AFTER THIS POINT -- a re-tune requires a FRESH holdout, which is a new task."),
			TagRun);

		// ⚠️ THE REPEAT HAPPENS INSIDE THIS SINGLE OPENING, WHICH IS THE ONLY SHAPE
		// section 12a AND section 12h BOTH ALLOW: "the holdout is opened once; the
		// repeat therefore happens INSIDE that single opening -- N generations
		// against the same file in the same session, reported as a distribution,
		// never as a second opening on a later day." repeats= is what makes that
		// mechanically possible; without it, taking N readings of the sealed file
		// means opening it N times, which is spending it N times.
		RunSplitRepeated(Options, ZoneAText, TEXT("HOLDOUT"), Options.HoldoutCorpusPath);
	}
	else
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s: no holdout= given, so ONLY the dev split was scored. The dev number IS NOT the go/no-go number."),
			TagRun);
	}
}

// ===========================================================================
// 8. SEH, JOB DISPATCH AND THE CONSOLE COMMANDS
// ===========================================================================

struct FSpikeJob
{
	ESpikeJobKind Kind = ESpikeJobKind::Load;
	FSpikeOptions Options;
};

static void SpikeJobBody(FSpikeJob& Job)
{
	switch (Job.Kind)
	{
		case ESpikeJobKind::Load:
			EnsureModelLoaded(Job.Options);
			break;
		case ESpikeJobKind::Bench:
			RunBenchJob(Job.Options);
			break;
		case ESpikeJobKind::Eval:
			RunEvalJob(Job.Options);
			break;
		default:
			break;
	}
}

/** A plain thunk with NO destructible locals -- MSVC forbids __try in a function that needs object unwinding. */
static void SpikeJobThunk(void* Context)
{
	SpikeJobBody(*static_cast<FSpikeJob*>(Context));
}

/**
 *  A GGML FAULT MUST NOT KILL THE GAME (CONVENTIONS section 8). This also covers
 *  the delay-load trap TASK-409 flagged: calling any llama_ or ggml_ entry point
 *  while the DLLs are unresolved raises a structured exception rather than
 *  returning an error.
 *
 *  ⚠️ THE WORDING ABOVE IS DELIBERATE. Writing the two prefixes as a glob pair
 *  puts a literal comment terminator inside a block comment and ends it early --
 *  the CONVENTIONS section 10 compile trap. It is not a style nit: the identical
 *  phrase in SiegeLlamaModule.h currently breaks that header (see the handoff's
 *  BLOCKER-1).
 */
static bool SpikeSehInvoke(void (*Thunk)(void*), void* Context, uint32& OutExceptionCode)
{
#if PLATFORM_WINDOWS && !PLATFORM_SEH_EXCEPTIONS_DISABLED
	__try
	{
		Thunk(Context);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		OutExceptionCode = static_cast<uint32>(GetExceptionCode());
		return false;
	}
	return true;
#else
	Thunk(Context);
	return true;
#endif
}

static void ShutdownSpike()
{
	// The model must be released before the module frees the delay-loaded DLLs.
	if (GJobInFlight)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: a spike job was still in flight at shutdown; the model is NOT being freed from the game thread (that would race the worker)."),
			TagWarn);
		return;
	}
	UnloadModel();
}

/** @return false when the job could not be started, with the reason already logged. */
static bool StartJob(ESpikeJobKind Kind, const FSpikeOptions& Options, UWorld* World)
{
	check(IsInGameThread());

	FSiegeLlamaModule* Module = FSiegeLlamaModule::GetPtr();
	if (Module == nullptr)
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: SiegeLlama module is not loaded."), TagRun);
		return false;
	}

	if (!Module->IsLlamaAvailable())
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: llama.cpp is unavailable (%s). This is a DEGRADED state, not a crash -- every keyboard command still works."),
			TagRun, *Module->GetLoadError());
		return false;
	}

	if (!Module->EnsureBackendsLoaded())
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: no ggml backends registered."), TagRun);
		return false;
	}

	if (GJobInFlight)
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: a spike job is already running. Queue depth is 1 by design."), TagRun);
		return false;
	}

	if (!bShutdownHookRegistered)
	{
		bShutdownHookRegistered = true;
		FCoreDelegates::OnEnginePreExit.AddStatic(&ShutdownSpike);
	}

	// ⚠️ THE MAP IS RECORDED IN THE REPORT ON PURPOSE. Bar #1 is about
	// CONTENTION: a number taken on an empty map is worse than no number,
	// because it looks like evidence. Printing the map makes an empty-map run
	// self-evident to whoever reads the log instead of a claim to be trusted.
	GLastWorldName = World ? World->GetMapName() : TEXT("<no world>");

	// ⚠️ THE DEADLINE IS ON THE FIRST LINE OF EVERY RUN (TASK-429), not only on
	// the runs that hit it. A wall time is only comparable against another wall
	// time taken under the same ceiling, and the ceiling is now settable.
	const FString DeadlineRegime = FormatDeadlineRegime(Options);

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s START kind=%d tier=%s %s map=%s netmode=%d utc=%s"),
		TagRun, static_cast<int32>(Kind), *Options.TierLabel, *DeadlineRegime, *GLastWorldName,
		World ? static_cast<int32>(World->GetNetMode()) : -1, *FDateTime::UtcNow().ToString());

	// ⛔ AND A NON-DEFAULT CEILING GETS ITS OWN WARNING, DELIBERATELY LOUDER THAN
	// A FIELD IN A LONG LINE. A reader comparing this run's numbers against an
	// earlier report has no way to know the ceiling moved unless the log SAYS SO,
	// and an unflagged non-default deadline is a measurement that lies. This is
	// also the line that stops the override from being mistaken for a retune: the
	// shipped HardTimeoutSeconds is unchanged at 10.0 and this argument does not
	// touch it.
	if (!FMath::IsNearlyEqual(Options.HardTimeoutSeconds, SpikeHardTimeoutSeconds, 1.e-6))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: NON-DEFAULT DEADLINE IN EFFECT FOR THIS RUN -- deadline=%.1fs, against the SHIPPED ceiling of %.1fs (CONVENTIONS section 10 HardTimeoutSeconds, which this argument does NOT change). Every wall-clock and abort figure below belongs to the %.1fs ceiling and MUST be quoted with it. %s"),
			TagWarn, Options.HardTimeoutSeconds, SpikeHardTimeoutSeconds, Options.HardTimeoutSeconds, *DeadlineRegime);
	}

	// ⛔ repeats= IS READ BY THE EVAL JOB AND BY NOTHING ELSE, AND A FLAG THAT IS
	// SILENTLY IGNORED IS WORSE THAN A FLAG THAT DOES NOT EXIST. Without this line
	// `Siege.Llama.SpikeBench repeats=5` runs ONCE, prints a completely normal
	// report, and the operator writes "5 runs" in a handoff. That is the same
	// class of defect the repeats= range check above refuses -- a single reading
	// believed to be N readings -- so it is caught at the one place that knows the
	// job kind. Unreachable unless repeats= is actually typed, so the default
	// output of every command is untouched.
	if (Options.EvalRepeats > 1 && Kind != ESpikeJobKind::Eval)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: repeats=%d was given but this is job kind=%d, and ONLY Siege.Llama.SpikeEval honours repeats=. THIS JOB RUNS EXACTLY ONCE -- do not report its numbers as a repeated measurement (CONVENTIONS section 12h)."),
			TagWarn, Options.EvalRepeats, static_cast<int32>(Kind));
	}

	// ⛔ THE SAME LAW ONE FLAG OVER, AND HERE IT IS UNCONDITIONAL RATHER THAN
	// KIND-DEPENDENT: out= AND ids= ARE READ BY Siege.Llama.SpikePrompt, WHICH DOES
	// NOT GO THROUGH StartJob AT ALL. So EVERY job kind that reaches this line
	// ignores them. Without this warning `Siege.Llama.SpikeEval out=C:/x.txt`
	// completes normally and writes nothing, and the operator is left to decide
	// whether the dump is EMPTY or ABSENT -- two very different facts that look
	// identical from a missing file. Unreachable unless one of them is typed, so
	// the default output of every command is untouched.
	if (!Options.PromptOutPath.IsEmpty() || Options.bDumpTokenIds)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: out= and/or ids= was given but this is job kind=%d, and ONLY Siege.Llama.SpikePrompt honours them. NO PROMPT FILE AND NO TOKEN-ID DUMP WILL BE PRODUCED BY THIS RUN -- nothing below is a rendered prompt artifact."),
			TagWarn, static_cast<int32>(Kind));
	}

	if (World == nullptr || !World->IsGameWorld())
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: this is NOT a running game world. Bar #1 requires PIE on L_Arena WITH UNITS ON THE FIELD -- an editor-viewport number is not a contention number."),
			TagWarn);
	}

	FSpikeJob* Job = new FSpikeJob();
	Job->Kind = Kind;
	Job->Options = Options;

	// ⚠️ RESOLVED HERE, ON THE GAME THREAD. USiegeLlamaSettings is a UObject CDO
	// and the override chain (-siegellm.model= > settings > <ProjectDir>/Models/)
	// must not be walked from a worker.
	if (Job->Options.ModelPathOverride.IsEmpty())
	{
		Job->Options.ModelPathOverride = USiegeLlamaSettings::ResolveModelPath();
	}

	GJobInFlight = true;
	GJobFinished = false;
	GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Baseline));
	GFrameSampler.Start();

	const int32 BaselineFrames = FMath::Max(0, Options.BaselineFrames);

	// ⚠️ TPri_BelowNormal, ON A BACKGROUND THREAD. See note (a) at the top: a
	// game-thread run cannot measure bar #1 at all. AsyncThread rather than an
	// FRunnable keeps this throwaway -- there is no worker CLASS to mistake for
	// TASK-423's production one.
	AsyncThread(
		[Job, BaselineFrames]()
		{
			// Let the sampler collect a quiet control window first, on the same
			// map, in the same session, with nothing running. Bounded so a
			// paused/unticking editor cannot hang the job forever.
			const double WaitDeadline = FPlatformTime::Seconds() + 30.0;
			while (GBaselineFrameCount.GetValue() < BaselineFrames
				&& !IsEngineExitRequested()
				&& FPlatformTime::Seconds() < WaitDeadline)
			{
				FPlatformProcess::Sleep(0.005f);
			}

			if (GBaselineFrameCount.GetValue() < BaselineFrames)
			{
				UE_LOG(LogSiegeLlama, Warning,
					TEXT("%s: only %d of %d baseline frames were captured before the 30s wait expired -- the control window is thin, so read the ATTRIBUTABLE deltas with suspicion."),
					TagWarn, GBaselineFrameCount.GetValue(), BaselineFrames);
			}

			uint32 ExceptionCode = 0;
			if (!SpikeSehInvoke(&SpikeJobThunk, Job, ExceptionCode))
			{
				UE_LOG(LogSiegeLlama, Error,
					TEXT("%s: THE SPIKE FAULTED (SEH code 0x%08X). The job was abandoned; the editor was not taken down with it. Treat every number from this run as void."),
					TagWarn, ExceptionCode);
			}

			GCurrentPhase.Set(static_cast<int32>(ESpikePhase::Idle));
			delete Job;
			GJobFinished = true;
		},
		0, TPri_BelowNormal);

	// The report is produced on the GAME THREAD, which is the only thread that
	// may touch the sampler.
	FTSTicker::GetCoreTicker().AddTicker(TEXT("SiegeLlamaSpikeCompletion"), 0.25f,
		[](float /*UnusedDelta*/)
		{
			if (!GJobFinished)
			{
				return true;
			}

			GFrameSampler.Stop();

			UE_LOG(LogSiegeLlama, Display,
				TEXT("%s ---- FRAME-TIME HITCH REPORT (map=%s, samples=%d) ----"),
				TagHitch, *GLastWorldName, GFrameSampler.Samples.Num());

			// ⚠️ BASELINE FIRST, ALWAYS. Every inference-phase number below is
			// only meaningful against it: L_Arena with a fielded army hitches on
			// its own, and an absolute worst-frame figure with no control is not
			// attributable to inference.
			FHistogram Baseline;
			BuildHistogram(GFrameSampler.Samples, ESpikePhase::Baseline, Baseline);
			LogHistogram(ESpikePhase::Baseline, Baseline);

			for (int32 PhaseIndex = static_cast<int32>(ESpikePhase::Load);
				PhaseIndex < static_cast<int32>(ESpikePhase::Count); ++PhaseIndex)
			{
				const ESpikePhase Phase = static_cast<ESpikePhase>(PhaseIndex);
				FHistogram Histogram;
				BuildHistogram(GFrameSampler.Samples, Phase, Histogram);
				LogHistogram(Phase, Histogram);

				if (Histogram.Count > 0)
				{
					UE_LOG(LogSiegeLlama, Display,
						TEXT("%s phase=%-8s ATTRIBUTABLE worst_delta=%+.2fms p99_delta=%+.2fms (vs baseline worst=%.2fms p99=%.2fms)"),
						TagHitch, PhaseName(Phase),
						Histogram.WorstMs - Baseline.WorstMs, Histogram.P99Ms - Baseline.P99Ms,
						Baseline.WorstMs, Baseline.P99Ms);
				}
			}

			UE_LOG(LogSiegeLlama, Display,
				TEXT("%s stage=peak peak_rss_sampled=%s"),
				TagMem, *FormatMiB(GFrameSampler.PeakRssBytes));

			UE_LOG(LogSiegeLlama, Display, TEXT("%s END utc=%s"), TagRun, *FDateTime::UtcNow().ToString());

			GJobInFlight = false;
			return false;
		});

	return true;
}

// ---------------------------------------------------------------------------
// 8a. Argument parsing
// ---------------------------------------------------------------------------

static bool GetArgValue(const TArray<FString>& Args, const TCHAR* Key, FString& OutValue)
{
	const FString Prefix = FString(Key) + TEXT("=");
	for (const FString& Arg : Args)
	{
		if (Arg.StartsWith(Prefix, ESearchCase::IgnoreCase))
		{
			OutValue = Arg.RightChop(Prefix.Len());
			OutValue = OutValue.TrimQuotes();
			return true;
		}
	}
	return false;
}

static void GetArgInt(const TArray<FString>& Args, const TCHAR* Key, int32& OutValue)
{
	FString Text;
	if (GetArgValue(Args, Key, Text) && !Text.IsEmpty())
	{
		OutValue = FCString::Atoi(*Text);
	}
}

/**
 *  ⚠️ RETURNS WHETHER THE ARG WAS PRESENT, unlike GetArgInt, and that matters:
 *  deadline= must be able to tell "not given" (keep the shipped 10.0) apart from
 *  "given" -- an unannounced non-default ceiling is a measurement that lies.
 *  FCString::Atod on a non-numeric string yields 0.0, which the caller rejects
 *  rather than silently arming a zero-second deadline that aborts everything.
 */
static bool GetArgDouble(const TArray<FString>& Args, const TCHAR* Key, double& OutValue)
{
	FString Text;
	if (GetArgValue(Args, Key, Text) && !Text.IsEmpty())
	{
		OutValue = FCString::Atod(*Text);
		return true;
	}
	return false;
}

static void GetArgBool(const TArray<FString>& Args, const TCHAR* Key, bool& OutValue)
{
	FString Text;
	if (GetArgValue(Args, Key, Text) && !Text.IsEmpty())
	{
		OutValue = (Text != TEXT("0")) && !Text.Equals(TEXT("false"), ESearchCase::IgnoreCase);
	}
}

/**
 *  tier= is a convenience over gpulayers=. `partial` needs the model's layer
 *  count, which is only knowable after a load -- so it resolves to "half of
 *  whatever the last load reported", and says so loudly when it has to guess.
 *  gpulayers= always wins, and Siege.Llama.SpikeLoad prints n_layer precisely so
 *  TASK-413 can pass the exact number and skip the guess entirely.
 */
static void ApplyTier(const TArray<FString>& Args, FSpikeOptions& Options)
{
	FString Tier;
	if (GetArgValue(Args, TEXT("tier"), Tier))
	{
		Options.TierLabel = Tier.ToLower();

		if (Options.TierLabel == TEXT("full"))
		{
			Options.GpuLayers = -1;
		}
		else if (Options.TierLabel == TEXT("cpu"))
		{
			Options.GpuLayers = 0;
		}
		else if (Options.TierLabel == TEXT("partial"))
		{
			if (GRunner.ModelLayerCount > 0)
			{
				Options.GpuLayers = GRunner.ModelLayerCount / 2;
			}
			else
			{
				// A first-run guess. Qwen3.5-4B is 36 layers, but guessing is not
				// measuring: the number that ends up in the report is the one
				// logged by SPIKE_LOAD, not this one.
				Options.GpuLayers = 18;
				UE_LOG(LogSiegeLlama, Warning,
					TEXT("%s: tier=partial before any load -- guessing gpulayers=%d. Run 'Siege.Llama.SpikeLoad tier=cpu' first to print the model's real n_layer, then pass gpulayers=<n_layer/2> explicitly."),
					TagWarn, Options.GpuLayers);
			}
		}
		else
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: unknown tier '%s' -- expected full | partial | cpu. Falling back to full offload."),
				TagWarn, *Options.TierLabel);
			Options.GpuLayers = -1;
		}
	}

	FString GpuLayersText;
	if (GetArgValue(Args, TEXT("gpulayers"), GpuLayersText) && !GpuLayersText.IsEmpty())
	{
		Options.GpuLayers = FCString::Atoi(*GpuLayersText);
		Options.TierLabel = FString::Printf(TEXT("gpulayers%d"), Options.GpuLayers);
	}
}

static FSpikeOptions ParseOptions(const TArray<FString>& Args)
{
	FSpikeOptions Options;

	ApplyTier(Args, Options);

	GetArgInt(Args, TEXT("ctx"), Options.ContextTokens);
	GetArgInt(Args, TEXT("ubatch"), Options.UBatch);
	GetArgInt(Args, TEXT("threads"), Options.Threads);
	GetArgInt(Args, TEXT("iters"), Options.Iterations);
	GetArgInt(Args, TEXT("baseline"), Options.BaselineFrames);
	GetArgInt(Args, TEXT("tokens"), Options.MaxOutputTokens);
	GetArgBool(Args, TEXT("grammar"), Options.bUseGrammar);
	GetArgBool(Args, TEXT("chat"), Options.bUseChatTemplate);
	GetArgValue(Args, TEXT("model"), Options.ModelPathOverride);
	GetArgValue(Args, TEXT("prompt"), Options.ZoneAOverridePath);

	// gpu=<index|name> -- WHICH CARD THE TIER MEANS. Without it the harness takes
	// the first discrete GPU and labels every VRAM figure ASSUMED, which is the
	// state qa/TASK-412.md BLOCKER-2 was raised against: on a machine that
	// enumerates an iGPU AND a dGPU, "the tier" was not a device.
	GetArgValue(Args, TEXT("gpu"), Options.GpuDeviceSpec);

	// ⛔ deadline=<seconds> -- A DIAGNOSTIC OVERRIDE, AND THE DEFAULT DOES NOT
	// MOVE. Options.HardTimeoutSeconds is initialised to SpikeHardTimeoutSeconds
	// (10.0 = CONVENTIONS section 10's shipped HardTimeoutSeconds); this only
	// replaces it when the argument is actually present, so a command line
	// without deadline= measures exactly what shipped.
	//
	// ⚠️ A REJECTED VALUE FALLS BACK LOUDLY RATHER THAN BEING CLAMPED QUIETLY.
	// `deadline=abc` parses to 0.0 through FCString::Atod, and a silently clamped
	// 0.0 would abort every generation instantly -- which would read as a
	// catastrophic model failure caused by a typo. The bound is generous (the
	// point is to find where generation stops succeeding, so an experiment must
	// be allowed to run long) but not unbounded.
	double RequestedDeadline = SpikeHardTimeoutSeconds;
	if (GetArgDouble(Args, TEXT("deadline"), RequestedDeadline))
	{
		if (RequestedDeadline < 0.1 || RequestedDeadline > 600.0)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: deadline=%.3f is outside the accepted range 0.1..600.0 seconds and was IGNORED -- this run uses the default %.1fs. Nothing was clamped: a clamped deadline would have produced timings under a ceiling the command line did not ask for."),
				TagWarn, RequestedDeadline, SpikeHardTimeoutSeconds);
		}
		else
		{
			Options.HardTimeoutSeconds = RequestedDeadline;
		}
	}

	// ⛔ repeats=<N> -- THE SAME "REJECT LOUDLY, NEVER CLAMP QUIETLY" TREATMENT AS
	// deadline=, AND FOR A SHARPER REASON. GetArgInt would turn `repeats=abc` into
	// 0 through FCString::Atoi and a Max(1, ...) would then quietly run ONE pass.
	// The operator would read the log, see a normal-looking single score, and
	// report it as a repeat-satisfying measurement -- which is CONVENTIONS section
	// 12h's exact failure, mechanised. So presence is detected separately from
	// value (GetArgValue, not GetArgInt), and an unusable value is REFUSED with a
	// line that says the run is a SINGLE pass and may not be quoted as a repeat.
	FString RepeatsText;
	if (GetArgValue(Args, TEXT("repeats"), RepeatsText) && !RepeatsText.IsEmpty())
	{
		const int32 RequestedRepeats = FCString::Atoi(*RepeatsText);
		if (RequestedRepeats < 1 || RequestedRepeats > SpikeMaxEvalRepeats)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: repeats='%s' parsed to %d, which is outside the accepted range 1..%d, and was IGNORED -- THIS RUN IS A SINGLE PASS. Nothing was clamped: a quietly clamped repeats= would print one score that reads exactly like a repeated measurement, and CONVENTIONS section 12h (THE REPEAT LAW) forbids quoting a single run as a measurement. Re-run with repeats=<1..%d>."),
				TagWarn, *RepeatsText, RequestedRepeats, SpikeMaxEvalRepeats, SpikeMaxEvalRepeats);
		}
		else
		{
			Options.EvalRepeats = RequestedRepeats;
		}
	}

	// ⛔ out=<path> AND ids=1 -- PARSED HERE, IN THE SHARED PARSER, FOR THE SAME
	// REASON chat= IS: one parser means one spelling of every flag, and a flag that
	// is spelled twice is a flag that will eventually be spelled two ways. They are
	// READ by Siege.Llama.SpikePrompt alone; StartJob warns when either reaches a
	// job kind that cannot honour it.
	//
	// ⚠️ AN EMPTY out= IS REFUSED OUT LOUD RATHER THAN TREATED AS ABSENT. `out=`
	// with nothing after it is a typo whose entire effect is that no file appears,
	// while the command completes normally -- so the operator goes looking for a
	// dump that was never created and concludes something false about the dumper.
	// Same "reject loudly, never proceed quietly" treatment as deadline= and
	// repeats= above.
	FString PromptOutText;
	if (GetArgValue(Args, TEXT("out"), PromptOutText))
	{
		if (PromptOutText.IsEmpty())
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: out= was given with an EMPTY value and was IGNORED -- NO FILE WILL BE WRITTEN by this run. Re-run with out=<path> (a relative path resolves against the project directory)."),
				TagWarn);
		}
		else
		{
			Options.PromptOutPath = PromptOutText;
		}
	}

	GetArgBool(Args, TEXT("ids"), Options.bDumpTokenIds);

	Options.ContextTokens = FMath::Clamp(Options.ContextTokens, 512, 32768);
	Options.UBatch = FMath::Clamp(Options.UBatch, 1, 2048);
	Options.Iterations = FMath::Clamp(Options.Iterations, 1, 200);
	Options.MaxOutputTokens = FMath::Clamp(Options.MaxOutputTokens, 1, 512);
	Options.BaselineFrames = FMath::Clamp(Options.BaselineFrames, 0, 10000);

	return Options;
}

// ---------------------------------------------------------------------------
// 8b. The commands
// ---------------------------------------------------------------------------

static void CmdSpikeLoad(const TArray<FString>& Args, UWorld* World)
{
	FSpikeOptions Options = ParseOptions(Args);
	Options.BaselineFrames = FMath::Min(Options.BaselineFrames, 60);
	StartJob(ESpikeJobKind::Load, Options, World);
}

static void CmdSpikeUnload(const TArray<FString>& /*Args*/, UWorld* /*World*/)
{
	if (GJobInFlight)
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: a job is in flight -- refusing to free the model underneath it."), TagRun);
		return;
	}

	// Safe on the game thread only because no worker is running (checked above).
	FSiegeLlamaModule* Module = FSiegeLlamaModule::GetPtr();
	if (Module != nullptr && Module->IsLlamaAvailable())
	{
		UnloadModel();
		UE_LOG(LogSiegeLlama, Display, TEXT("%s: model and context released."), TagRun);
	}
}

static void CmdSpikeBench(const TArray<FString>& Args, UWorld* World)
{
	StartJob(ESpikeJobKind::Bench, ParseOptions(Args), World);
}

static void CmdSpikeEval(const TArray<FString>& Args, UWorld* World)
{
	FSpikeOptions Options = ParseOptions(Args);

	if (!GetArgValue(Args, TEXT("dev"), Options.DevCorpusPath))
	{
		Options.DevCorpusPath = FPaths::ProjectDir() / TEXT("Docs/Data/assistant_eval_dev.csv");
	}

	// ⚠️ NO DEFAULT FOR holdout=, DELIBERATELY. The holdout opens exactly once,
	// at TASK-413, and it must be an explicit act every single time.
	GetArgValue(Args, TEXT("holdout"), Options.HoldoutCorpusPath);

	StartJob(ESpikeJobKind::Eval, Options, World);
}

/**
 *  ⚠️ THE TOKEN-ID DUMP IS CHUNKED, AND THE CHUNKING IS THE POINT. A templated
 *  Zone A alone is ~1362 tokens, so a single line would be ~10 KB -- long enough
 *  that log sinks truncate it, and a TRUNCATED id sequence compared against a
 *  Python one manufactures a divergence at exactly the index where the log gave
 *  up. Indexed chunks make the comparison resumable and the truncation visible.
 */
static constexpr int32 SpikePromptIdsPerLine = 32;

/** Comma-separated token ids over [Start, Start + Count) -- one formatter, used by both the summary line and the chunks. */
static FString FormatTokenIdSpan(const TArray<llama_token>& Tokens, int32 Start, int32 Count)
{
	FString Out;

	const int32 First = FMath::Max(0, Start);
	const int32 End = FMath::Min(First + FMath::Max(0, Count), Tokens.Num());

	for (int32 Index = First; Index < End; ++Index)
	{
		if (!Out.IsEmpty())
		{
			Out += TEXT(",");
		}
		Out += FString::FromInt(static_cast<int32>(Tokens[Index]));
	}

	return Out;
}

static void CmdSpikePrompt(const TArray<FString>& Args, UWorld* /*World*/)
{
	const FSpikeOptions Options = ParseOptions(Args);

	FString Utterance = TEXT("send 10 footmen with a sorcerer to the nearest ancient ground");
	GetArgValue(Args, TEXT("order"), Utterance);

	// fixture=t0|t1. t0 is the corpus's board and the default; t1 is the same
	// board a few seconds later and exists so the bench's SHIPPED_WORST_CASE bound
	// can be inspected byte-for-byte rather than taken on trust.
	FString FixtureName;
	GetArgValue(Args, TEXT("fixture"), FixtureName);
	const bool bUseT1 = FixtureName.Equals(TEXT("t1"), ESearchCase::IgnoreCase);
	const FSpikeWorldFixture& Fixture = bUseT1 ? SpikeFixtureT1 : SpikeFixtureT0;

	FString ZoneA;
	AppendZoneA(ZoneA);

	FString ZoneB;
	AppendZoneB(ZoneB, Fixture);

	FString ZoneC;
	AppendZoneC(ZoneC, Fixture, SanitizeForPrompt(Utterance), FString());

	const int32 ZoneBCChars = ZoneB.Len() + ZoneC.Len();

	UE_LOG(LogSiegeLlama, Display, TEXT("%s ---- ZONE A (%d chars) ----"), TagTokens, ZoneA.Len());
	UE_LOG(LogSiegeLlama, Display, TEXT("\n%s"), *ZoneA);
	UE_LOG(LogSiegeLlama, Display, TEXT("%s ---- ZONE B (fixture=%s, %d chars) ----"), TagTokens, Fixture.Label, ZoneB.Len());
	UE_LOG(LogSiegeLlama, Display, TEXT("\n%s"), *ZoneB);
	UE_LOG(LogSiegeLlama, Display, TEXT("%s ---- ZONE C (fixture=%s, %d chars) ----"), TagTokens, Fixture.Label, ZoneC.Len());
	UE_LOG(LogSiegeLlama, Display, TEXT("\n%s"), *ZoneC);

	// The cap that actually governs the <= 400-token budget is on B + C only;
	// Zone A is deliberately unbounded (CONVENTIONS section 8, corrected from
	// measurement -- a smaller Zone A makes bar #3 WORSE, not better).
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s fixture=%s zoneA_chars=%d zoneB_chars=%d zoneC_chars=%d zoneBC_chars=%d MaxSnapshotChars=%d headroom=%d"),
		TagTokens, Fixture.Label, ZoneA.Len(), ZoneB.Len(), ZoneC.Len(), ZoneBCChars,
		SpikeMaxSnapshotChars, SpikeMaxSnapshotChars - ZoneBCChars);

	// The frozen t0 figures QA verified by hand. A mismatch means someone edited
	// the fixture the sealed corpus was authored against.
	if (!bUseT1 && (ZoneB.Len() != 68 || ZoneC.Len() != 887) && Utterance.Equals(TEXT("send 10 footmen with a sorcerer to the nearest ancient ground")))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: fixture t0 now measures Zone B = %d chars and Zone C = %d chars for the default order line; QA verified 68 and 887. The corpus's premises were authored against the OLD numbers -- reconcile before quoting bar #5."),
			TagWarn, ZoneB.Len(), ZoneC.Len());
	}

	// ⚠️ THE STRING WAS SWEPT AT TASK-478 AND NOTHING ELSE IN THIS FUNCTION WAS
	// TOUCHED (CONVENTIONS section 22 -- a runtime log string is the PRIORITY
	// surface for a stale claim, because it is the artifact an investigator trusts
	// most). It read "fixture %s disagrees with THE GRAMMAR'S KIND LIST", which is
	// now self-contradictory: the grammar's kind list IS this fixture's roster. What
	// the check actually verifies is the fixture against SpikeRoster, the game's
	// unit VOCABULARY -- which is the authority that survived.
	// ⛔ The branch is unreachable for t0 and t1 (both are valid subsets by
	// construction), so this rewording cannot move one byte of this command's
	// output on any command line that exists today.
	FString KindMismatch;
	if (!VerifyFixtureKindParity(Fixture, KindMismatch))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: fixture %s is NOT A VALID SUBSET of SpikeRoster (%s). The grammar is generated from this fixture, so the prompt above may be showing -- and the sampler may be able to emit -- a unit symbol the game does not have."),
			TagWarn, Fixture.Label, *KindMismatch);
	}

	FSiegeLlamaModule* Module = FSiegeLlamaModule::GetPtr();
	if (Module == nullptr || !Module->IsLlamaAvailable() || !GRunner.IsLoaded() || GJobInFlight)
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s: TOKEN counts need a resident model and no job in flight -- run 'Siege.Llama.SpikeLoad' first. Character counts above are exact regardless."),
			TagTokens);

		// ⛔ AND IF A DUMP WAS ASKED FOR, REFUSE IT AT ERROR LEVEL RATHER THAN
		// RETURNING QUIETLY. The dump is NOT merely unavailable here -- writing it
		// would be actively WRONG. With no resident model BuildPrompt cannot reach
		// llama_model_chat_template, so it falls back to raw concatenation SILENTLY,
		// and out= would produce a file that is the RAW wire format while the
		// command line said chat=1. A consumer asserting a length and a sha256
		// against that file would be asserting against the wrong lane and would
		// PASS -- traps T1 and T8 in a single move, which is this project's exact
		// recurring failure: a confident green describing something else.
		//
		// ⚠️ Deliberately NOT special-cased for chat=0, which genuinely needs no
		// model: that would create a SECOND site that writes the artifact, and
		// "exactly one composer" is the rule this whole batch exists to defend.
		if (!Options.PromptOutPath.IsEmpty() || Options.bDumpTokenIds)
		{
			UE_LOG(LogSiegeLlama, Error,
				TEXT("%s: REFUSED -- out= and/or ids= needs a RESIDENT MODEL and no job in flight, and NOTHING WAS WRITTEN. This is a refusal, not a failure to find the file. Without a loaded model BuildPrompt cannot consult the chat template and would silently emit the RAW concatenation, so out= would write a file whose wire format does not match the chat= this command line asked for. Run 'Siege.Llama.SpikeLoad tier=full gpu=0' first, then re-issue this command."),
				TagPromptOut);
		}

		return;
	}

	// Zone-wise counts are INDICATIVE ONLY: tokenization is not compositional,
	// so the parts do not have to sum to the whole. The assembled total is the
	// figure to quote when correcting MaxSnapshotChars from measurement.
	TArray<llama_token> Tokens;
	int32 ZoneATokens = 0;
	int32 ZoneBTokens = 0;
	int32 ZoneCTokens = 0;

	if (TokenizePrompt(ZoneA, Tokens)) { ZoneATokens = Tokens.Num(); }
	if (TokenizePrompt(ZoneB, Tokens)) { ZoneBTokens = Tokens.Num(); }
	if (TokenizePrompt(ZoneC, Tokens)) { ZoneCTokens = Tokens.Num(); }

	FString FullPrompt;
	FSpikePromptLayout Layout;
	BuildPrompt(Options, ZoneA, Fixture, Utterance, FString(), FullPrompt, &Layout);
	int32 FullTokens = 0;
	if (TokenizePrompt(FullPrompt, Tokens)) { FullTokens = Tokens.Num(); }

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s fixture=%s zoneA_tok~=%d zoneB_tok~=%d zoneC_tok~=%d zoneBC_tok~=%d (<=400 cap) assembled_total_tok=%d chars_per_token=%.2f"),
		TagTokens, Fixture.Label, ZoneATokens, ZoneBTokens, ZoneCTokens, ZoneBTokens + ZoneCTokens, FullTokens,
		FullTokens > 0 ? static_cast<double>(FullPrompt.Len()) / FullTokens : 0.0);

	// The ZONE BOUNDARIES IN TOKEN SPACE, in the assembled prompt -- the same
	// numbers bar #3's `landed_in=` classification is derived from, printed here
	// so TASK-413 can check the classifier itself rather than trusting it. A KV
	// reuse point below zoneB_start~ means only Zone A was reused (the shipped
	// worst case); at or above zoneC_start~ means the snapshot did not move.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s fixture=%s assembled_prompt_chars=%d zoneB_start_char=%d zoneC_start_char=%d zoneB_start_tok~=%d zoneC_start_tok~=%d (~ = a prefix tokenization, +/-1 because tokenization is not compositional)"),
		TagTokens, Fixture.Label, FullPrompt.Len(), Layout.ZoneBCharOffset, Layout.ZoneCCharOffset,
		TokenIndexOfCharOffset(FullPrompt, Layout.ZoneBCharOffset),
		TokenIndexOfCharOffset(FullPrompt, Layout.ZoneCCharOffset));

	// =======================================================================
	// out= / ids= -- APPENDED AFTER EVERY PRE-EXISTING LINE, ON PURPOSE
	// =======================================================================
	//
	// ⛔ NOTHING ABOVE THIS POINT WAS TOUCHED. The whole block is unreachable
	// unless out= or ids= is typed, so a command line passing neither produces
	// character-identical output to this command as it stood before the flags
	// existed (CONVENTIONS section 16 freeze #2). Placing it at the END rather
	// than woven in also means no pre-existing line CHANGED POSITION in the
	// stream, which is the half of "identical output" that a format-string
	// comparison alone would not catch.
	if (!Options.PromptOutPath.IsEmpty() || Options.bDumpTokenIds)
	{
		// ⛔ WHICH WIRE FORMAT THESE BYTES ACTUALLY ARE, READ OFF THE BYTES RATHER
		// THAN ECHOED BACK FROM THE FLAG -- and that distinction is the whole
		// validity of STOP 1. chat= records an INTENTION. BuildPrompt falls back to
		// raw concatenation whenever llama_model_chat_template returns null (the
		// GGUF carries no template) and that fallback is SILENT; it falls back
		// again, with a warning, if llama_chat_apply_template fails. Either way a
		// `chat=1` run can produce raw bytes, and an A/B that was secretly raw-vs-raw
		// would measure a difference of zero and be read as "D1 does not exist".
		//
		// ⚠️ THE DISCRIMINATOR IS A POSITIVE STRUCTURAL FACT, NOT A SEARCH FOR AN
		// ABSENCE (CONVENTIONS section 14): in BOTH raw branches BuildPrompt emits
		// ZoneAText + ZoneB + ZoneC, so Zone A sits at offset 0. Any chat template
		// opens with a role marker BEFORE the system content, so it cannot. The
		// prompt therefore starts with Zone A if and only if these bytes are raw.
		const bool bChatRequested = Options.bUseChatTemplate;
		const bool bModelHasTemplate = (GRunner.Model != nullptr)
			&& (llama_model_chat_template(GRunner.Model, nullptr) != nullptr);
		const bool bStartsWithZoneA = FullPrompt.StartsWith(ZoneA, ESearchCase::CaseSensitive);
		const TCHAR* const WireLabel = bStartsWithZoneA ? TEXT("raw") : TEXT("chat");

		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s fixture=%s wire=%s chat_requested=%d model_has_template=%d prompt_starts_with_zoneA=%d assembled_chars=%d assembled_tok=%d zoneA_chars=%d (wire= is MEASURED from the produced bytes, never copied from chat=)"),
			TagPromptOut, Fixture.Label, WireLabel,
			bChatRequested ? 1 : 0, bModelHasTemplate ? 1 : 0, bStartsWithZoneA ? 1 : 0,
			FullPrompt.Len(), FullTokens, ZoneA.Len());

		if (bChatRequested && bStartsWithZoneA)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: chat=1 WAS REQUESTED BUT THESE BYTES ARE THE RAW CONCATENATION. BuildPrompt fell back, for one of exactly two reasons: the loaded GGUF exposes no chat template (model_has_template=%d above), or llama_chat_apply_template failed and logged its own line. DO NOT quote this run as the templated half of a wire-format A/B -- it would compare raw against raw and report a difference of zero, which reads as 'the two shapes are the same' and is the one conclusion this measurement must never reach by accident."),
				TagWarn, bModelHasTemplate ? 1 : 0);
		}

		if (!Options.PromptOutPath.IsEmpty())
		{
			// A relative path resolves against the PROJECT directory, matching how
			// dev= builds its default, and the ABSOLUTE path is then printed -- so a
			// typo shows up as a file in a surprising place rather than as a silence.
			//
			// ⚠️ THE SECOND CALL IS NOT REDUNDANT, AND THE TWO-ARGUMENT OVERLOAD ALONE
			// WOULD HAVE BEEN WRONG HERE. FPaths::ProjectDir() is ITSELF RELATIVE in an
			// editor build (`../../../<Project>/` from Engine/Binaries/Win64), and
			// ConvertRelativePathToFull(Base, In) only JOINS and COLLAPSES -- it does
			// not force absoluteness when the base is relative. The result would still
			// have opened the right file, because IFileManager anchors relative paths
			// at BaseDir, and would have PRINTED a `../../../` path that is useless as
			// the "here is exactly what I wrote" evidence this line exists to be. The
			// one-argument overload anchors at FPlatformProcess::BaseDir() and does
			// normalize + collapse internally, which is why NormalizeFilename is not
			// called separately.
			FString AbsPath = Options.PromptOutPath;
			if (FPaths::IsRelative(AbsPath))
			{
				AbsPath = FPaths::ProjectDir() / AbsPath;
			}
			AbsPath = FPaths::ConvertRelativePathToFull(AbsPath);

			// The destination tree is created because the intended home for these
			// dumps does not exist yet, and "the write failed" is a poor way to
			// learn that a directory is missing.
			const FString OutDir = FPaths::GetPath(AbsPath);
			if (!OutDir.IsEmpty())
			{
				IFileManager::Get().MakeDirectory(*OutDir, true);
			}

			// ⛔ UTF-8 WITHOUT A BOM, AND THIS IS NOT A STYLE CHOICE -- IT IS THE
			// ONLY ENCODING THAT MAKES THE FILE THE SAME OBJECT THE ENGINE MEASURED.
			// TokenizePrompt hands llama_tokenize the FTCHARToUTF8 of this exact
			// FString, so these bytes are, byte for byte, what the tokenizer saw.
			// FFileHelper's DEFAULT encoding is AutoDetect, which writes UTF-16 with
			// a BOM the moment the prompt contains one non-ASCII character -- a file
			// that still looks correct in an editor, still has a plausible length,
			// and hashes to something no consumer can reproduce. Hence the explicit
			// byte array rather than SaveStringToFile.
			const FTCHARToUTF8 Utf8(*FullPrompt);
			TArray64<uint8> Bytes;
			Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());

			if (FFileHelper::SaveArrayToFile(Bytes, *AbsPath))
			{
				// ⚠️ THE sha256 IS DELIBERATELY NOT COMPUTED HERE. The consumer
				// asserts it over the file it actually reads, which is the only
				// place the assertion means anything; a hash printed by the writer
				// certifies the writer. The LENGTH is printed because it is the
				// cheap disagreement detector, and it is the UTF-8 BYTE count --
				// not the FString character count, which differs on any non-ASCII
				// content and is printed separately so the two never get confused.
				UE_LOG(LogSiegeLlama, Display,
					TEXT("%s WROTE fixture=%s wire=%s utf8_bytes=%d chars=%d encoding=UTF-8-no-BOM path=%s (assert LENGTH and sha256 over THIS FILE; the trainer CONSUMES these bytes and must never assemble a prompt of its own)"),
					TagPromptOut, Fixture.Label, WireLabel, Utf8.Length(), FullPrompt.Len(), *AbsPath);
			}
			else
			{
				UE_LOG(LogSiegeLlama, Error,
					TEXT("%s: FAILED to write the prompt bytes to '%s' -- NOTHING was written and any file already at that path is UNCHANGED and therefore STALE. Check the path is writable and is not open elsewhere."),
					TagPromptOut, *AbsPath);
			}
		}

		if (Options.bDumpTokenIds)
		{
			// ⚠️ Tokens STILL HOLDS THE FULL PROMPT'S IDS -- TokenizePrompt(FullPrompt,
			// Tokens) filled it above and nothing between here and there touches it
			// (TokenIndexOfCharOffset uses its own local array). Reused DELIBERATELY
			// rather than re-tokenized: the ids printed below are then PROVABLY the
			// same sequence that produced the assembled_total_tok figure on the
			// SPIKE_TOKENS line, instead of a second reading that merely ought to
			// agree with it. FullTokens > 0 is exactly the condition under which
			// that call succeeded.
			if (FullTokens <= 0)
			{
				UE_LOG(LogSiegeLlama, Error,
					TEXT("%s: ids=1 was requested but llama_tokenize FAILED on the assembled prompt, so NO ids were produced. The character counts above are still exact; nothing here is a zero-length token sequence."),
					TagPromptIds);
			}
			else
			{
				UE_LOG(LogSiegeLlama, Display,
					TEXT("%s fixture=%s wire=%s count=%d add_special=1 parse_special=1 source=llama_tokenize(vocab,UTF-8(assembled prompt)) first=[%s] last=[%s] -- THE ENGINE HALF OF THE TOKEN-ID PARITY CHECK. A Python count taken under different add_special/parse_special flags is a DIFFERENT measurement and comparing them manufactures a divergence."),
					TagPromptIds, Fixture.Label, WireLabel, Tokens.Num(),
					*FormatTokenIdSpan(Tokens, 0, 8),
					*FormatTokenIdSpan(Tokens, FMath::Max(0, Tokens.Num() - 8), 8));

				for (int32 SpanStart = 0; SpanStart < Tokens.Num(); SpanStart += SpikePromptIdsPerLine)
				{
					const int32 SpanCount = FMath::Min(SpikePromptIdsPerLine, Tokens.Num() - SpanStart);
					UE_LOG(LogSiegeLlama, Display,
						TEXT("%s i=%d..%d ids=%s"),
						TagPromptIds, SpanStart, SpanStart + SpanCount - 1,
						*FormatTokenIdSpan(Tokens, SpanStart, SpanCount));
				}
			}
		}
	}
}

/**
 *  ⭐ THE ONE PLACE THE PER-FIXTURE CAPABILITY CAN BE SEEN WITH NO MODEL LOADED,
 *     AND THAT IS WHY THE ALWAYS-ON READOUT LIVES HERE RATHER THAN ON SpikeEval.
 *
 *  ⚖️ THE TASK-477 DISPOSITION, COPIED DELIBERATELY: when the useful line would move
 *     the instrument, it goes on the OTHER command and the help text says so. A
 *     "seam CLOSED" line is genuinely valuable -- CONVENTIONS section 14 wants the
 *     positive proved, and a check that only ever prints on failure is
 *     indistinguishable from a check that never ran. But printing it on
 *     Siege.Llama.SpikeEval's default path would add a line to the instrument every
 *     number on record was taken with (section 16 freeze #2, unconditional).
 *     ⇒ The measurement lanes get the WARNING only; THIS command gets the verdict
 *     both ways, because Siege.Llama.SpikeGrammar is not in section 16's frozen set
 *     and no measurement has ever been taken from it.
 *
 *  ⛔ THE PRE-EXISTING HEADER LINE AND THE DUMP ARE UNTOUCHED AND STILL FIRST, and
 *     the new block is appended AFTER them (TASK-477's placement rule: nothing that
 *     already existed changes POSITION in the stream). The single exception is a
 *     LEADING banner when a NON-DEFAULT fixture was named -- because at that point
 *     the dump below is NOT the canonical grammar, and a reader who copied it into
 *     a diff against the landed generator would file a false mismatch.
 *
 *  ⚠️ THIS COMMAND READS THE TRANSCRIBED ZONE A CONSTANT. It has no prompt= override
 *     (it never called ParseOptions), which is stated in the readout rather than left
 *     to be assumed -- see the handoff's finding on prompt= and Siege.Llama.SpikePrompt.
 */
static void CmdSpikeGrammar(const TArray<FString>& Args, UWorld* /*World*/)
{
	FString FixtureName;
	GetArgValue(Args, TEXT("fixture"), FixtureName);

	bool bFixtureRecognised = true;
	const FSpikeWorldFixture& Fixture = ResolveSpikeFixture(FixtureName, bFixtureRecognised);

	if (!bFixtureRecognised)
	{
		// Loudly, and BEFORE the dump: a typo that silently hands back t0 is how an
		// operator ends up quoting a grammar for a board they never selected.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: unknown fixture '%s' -- expected %s. FALLING BACK TO t0, so the grammar below is t0's and NOT the one you asked for."),
			TagWarn, *FixtureName, *ListSpikeFixtureNames());
	}
	else if (!FixtureName.IsEmpty() && &Fixture != &SpikeFixtureT0)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: ---- THE DUMP BELOW IS FIXTURE '%s', NOT THE DEFAULT t0 ---- The `kind` alternatives are generated from THIS fixture's roster (%d of SpikeRoster's %d kinds), so do NOT diff it against the landed USiegeAssistantGrammar::Build output taken for a full board and report a mismatch."),
			TagWarn, Fixture.Label, Fixture.RosterNum, SpikeRosterNum);
	}

	const FString Grammar = BuildSpikeGrammar(Fixture);

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s ---- GBNF (%d chars) ---- diff this against USiegeAssistantGrammar::Build(fixture kinds, the 7 pinned places). THE LANDED GRAMMAR IS THE AUTHORITY (CONVENTIONS section 9c); if they differ, THIS FILE IS WRONG."),
		TagRun, Grammar.Len());

	TArray<FString> Lines;
	Grammar.ParseIntoArrayLines(Lines, false);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogSiegeLlama, Display, TEXT("  %s"), *Line);
	}

	// =======================================================================
	// TASK-478's READOUT -- APPENDED AFTER EVERY PRE-EXISTING LINE
	// =======================================================================
	FString KindMismatch;
	const bool bParityOk = VerifyFixtureKindParity(Fixture, KindMismatch);
	const FString ParityText = bParityOk ? FString(TEXT("OK")) : (FString(TEXT("FAILED -- ")) + KindMismatch);

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s GRAMMAR_FIXTURE fixture=%s kinds=%d of SpikeRoster's %d (%s) subset_parity=%s -- `kind` is generated from THIS fixture. `count` and `at-least` are NOT: they stay %d..%d, never the live totals (CONVENTIONS section 1: constrain identity hard, leave quantity soft; a grammar capped at the live count silently emits the wrong number and makes the clarification undetectable)."),
		TagRun, Fixture.Label, Fixture.RosterNum, SpikeRosterNum,
		Fixture.RosterNum == SpikeRosterNum
			? TEXT("FULL ROSTER -- character-identical to the grammar this command emitted before per-fixture derivation existed")
			: TEXT("STRICT SUBSET -- a sparse board, the class divergence D3 is about"),
		*ParityText,
		SpikeGrammarCountMin, SpikeGrammarCountMax);

	// The transcribed constant, named as such: this command has no prompt= override,
	// so the seam reported here is the one the SHIPPED-mirror Zone A produces.
	FString ZoneA;
	AppendZoneA(ZoneA);
	ReportZoneAKindSeam(ZoneA, Fixture, TEXT("grammar(transcribed Zone A -- this command has NO prompt= override)"),
		/*bReportWhenClosed*/ true);
}

} // namespace SiegeLlamaSpike

// ---------------------------------------------------------------------------
// Registration. FAutoConsoleCommandWithWorldAndArgs in a NEW FILE -- never a
// UFUNCTION(exec) on a shipped class (CONVENTIONS section 5). The world is used
// only to record which map the numbers were taken on.
// ---------------------------------------------------------------------------

static FAutoConsoleCommandWithWorldAndArgs GSiegeLlamaSpikeLoadCommand(
	TEXT("Siege.Llama.SpikeLoad"),
	TEXT("Loads the GGUF for a tier and prints n_layer, load time, EVERY ggml device, which one the weights were pinned to, and the VRAM/RSS deltas for THAT device. "
		 "Also prints the context's ACTUAL n_ctx / n_batch / n_ubatch / n_threads / n_threads_batch beside the REQUESTED value of each, plus the machine's physical core count -- "
		 "llama.h:551-556 warns the actual may differ from the request, and a thread count clamped below the cores is a root cause rather than a hardware limit. "
		 "Args: tier=full|partial|cpu gpulayers=N gpu=<index|name> ctx=2048 ubatch=64 threads=N model=<path>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeLlamaSpike::CmdSpikeLoad));

static FAutoConsoleCommandWithWorldAndArgs GSiegeLlamaSpikeUnloadCommand(
	TEXT("Siege.Llama.SpikeUnload"),
	TEXT("Frees the model and context. Refuses while a job is in flight."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeLlamaSpike::CmdSpikeUnload));

static FAutoConsoleCommandWithWorldAndArgs GSiegeLlamaSpikeBenchCommand(
	TEXT("Siege.Llama.SpikeBench"),
	TEXT("Bars #1-#4: frame-time hitch histogram (worst + p99, never the mean), TTFT and wall clock, "
		 "turn-1 vs turn-2 prefill tokens, peak VRAM and RSS. RUN IT IN PIE ON L_Arena WITH UNITS ON THE FIELD. "
		 "Bar #3 prints TWO bounds -- BEST_CASE (a paused board) and SHIPPED_WORST_CASE (the board moved between turns) -- plus WHERE the reuse landed; "
		 "QUOTE THE SHIPPED FIGURE. The warm iterations alternate the two fixtures, so bar #2 is the shipped shape too. "
		 "Args: tier=full|partial|cpu gpulayers=N gpu=<index|name> iters=5 baseline=180 ubatch=64 threads=N tokens=96 grammar=0|1 chat=0|1 prompt=<zoneA path> "
		 "deadline=<seconds> (DIAGNOSTIC OVERRIDE of the 10.0s hard abort ceiling; the DEFAULT IS AND STAYS 10.0 = the shipped HardTimeoutSeconds. "
		 "It exists to tell a deadline-cut run apart from a genuine generation failure, which look identical otherwise. "
		 "AUTHORITATIVE only on tier=cpu -- abort_callback is documented CPU-only, so on GPU tiers it fires only between graph submissions and tokens= is the real bound. Every line that prints it says which)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeLlamaSpike::CmdSpikeBench));

static FAutoConsoleCommandWithWorldAndArgs GSiegeLlamaSpikeEvalCommand(
	TEXT("Siege.Llama.SpikeEval"),
	TEXT("Bar #5: exact-match accuracy on the SEALED corpus, scored per split and reported separately. "
		 "dev= defaults to Docs/Data/assistant_eval_dev.csv; holdout= HAS NO DEFAULT and opens the sealed file -- "
		 "the HOLDOUT number alone scores the 85-percent bar. Every row runs against fixture t0, the board the corpus was authored "
		 "against. Args: dev=<path> holdout=<path> tier=... gpu=<index|name> prompt=<zoneA path> deadline=<seconds> "
		 "(same diagnostic override as SpikeBench, default 10.0. A row whose generation is CUT by the ceiling scores as a WRONG ANSWER, so the row line names it) "
		 "repeats=<N> (1..25, DEFAULT 1). Loops each split N times INSIDE THIS ONE JOB -- one queue slot, one model load, one session. "
		 "CONVENTIONS section 12h: ONE RUN IS NOT A MEASUREMENT, and this is the mechanical way to satisfy that instead of re-typing the command -- "
		 "which the harness correctly REFUSES, because queue depth is 1 by design. It is also the only way to repeat the SEALED holdout, whose repeat must happen INSIDE its single opening. "
		 "At N>1 it additionally prints SPIKE_EVAL_REPEAT lines: split-level min/median/max (section 10 clause #1 scores the MINIMUM, clause #6 the MEDIAN, nothing scores the maximum), "
		 "per-row stable=N/N or flips=k, and THE IDENTITY of every row that moved together with its distinct outputs. "
		 "repeats=1 emits ZERO SPIKE_EVAL_REPEAT lines and is byte-identical to this command as it was before the flag existed. "
		 "chat=0|1 (DEFAULT 1) selects the wire format for EVERY row: 1 = the model's chat template (Zone A as system, Zone B+C as user, add_generation_prompt=true), "
		 "0 = THE SHIPPED SHAPE, a raw Zone A + Zone B + Zone C concatenation with no roles. This flag was ALWAYS parsed here; only this sentence is new. "
		 "It is what makes the wire-format A/B two commands instead of a code change: 'chat=1 repeats=5' then 'chat=0 repeats=5'. "
		 "WARNING: this command prints NO wire= indicator, because adding a line to its default output would move the instrument every number on record was taken with. "
		 "BuildPrompt falls back to raw concatenation SILENTLY when the loaded GGUF exposes no chat template, so a chat=1 eval can be secretly raw. "
		 "TAKE THE WIRE-FORMAT READING WITH Siege.Llama.SpikePrompt FIRST -- its SPIKE_PROMPT_OUT line reports wire= measured off the produced bytes -- and only then trust an A/B taken here"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeLlamaSpike::CmdSpikeEval));

static FAutoConsoleCommandWithWorldAndArgs GSiegeLlamaSpikePromptCommand(
	TEXT("Siege.Llama.SpikePrompt"),
	TEXT("Dumps the exact Zone A / Zone B / Zone C bytes plus character and token counts, so MaxSnapshotChars "
		 "is corrected from measurement rather than guessed twice. Args: order=\"...\" fixture=t0|t1 "
		 "(t0 is the corpus's board and the default; t1 is the same board seconds later, which is what the bench's "
		 "SHIPPED_WORST_CASE bound diverges into. Token counts need a loaded model) "
		 "out=<path> writes the ASSEMBLED prompt's exact bytes as UTF-8 WITHOUT a BOM. THE TRAINER CONSUMES THESE BYTES AND MUST NEVER ASSEMBLE A PROMPT OF ITS OWN -- "
		 "three lanes already disagree and a re-implementation would bake that skew into the weights, where it is invisible forever. "
		 "A relative path resolves against the project directory, the destination directory is created, and the absolute path is printed. "
		 "Assert LENGTH and sha256 over the written file; the command deliberately does not hash it, because a hash printed by the writer certifies the writer. "
		 "ids=1 prints the engine's llama_tokenize sequence for the SAME assembled prompt, in indexed 32-id chunks, beside the add_special/parse_special flags it was taken under -- "
		 "the ENGINE half of the token-id parity check; the Python comparison is a separate task and is deliberately not attempted here. "
		 "chat=0|1 (DEFAULT 1) selects the wire format: 1 = the model's chat template (Zone A as system, Zone B+C as user, add_generation_prompt=true), "
		 "0 = THE SHIPPED SHAPE, a raw Zone A + Zone B + Zone C concatenation with no roles and no separator. "
		 "THE FLAG IS NOT ECHOED BACK: the SPIKE_PROMPT_OUT line reports wire= MEASURED from the produced bytes, so a chat=1 that silently fell back to raw "
		 "(the GGUF carries no template, or llama_chat_apply_template failed) is VISIBLE rather than assumed. "
		 "out= and ids= both need a resident model and REFUSE at Error level without one -- a dump taken with no model would silently be the raw shape whatever chat= said"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeLlamaSpike::CmdSpikePrompt));

static FAutoConsoleCommandWithWorldAndArgs GSiegeLlamaSpikeGrammarCommand(
	TEXT("Siege.Llama.SpikeGrammar"),
	TEXT("Dumps the spike's GBNF so QA can DIFF it against USiegeAssistantGrammar::Build instead of proof-reading it. "
		 "Args: fixture=t0|t1 (DEFAULT t0). THE `kind` ALTERNATIVES ARE GENERATED FROM THE NAMED FIXTURE'S ROSTER, not from a fixed thirteen-symbol list -- "
		 "which is what the SHIPPED USiegeAssistantGrammar::Build has always done, so this is the spike moving TOWARD the landed authority (CONVENTIONS section 9c). "
		 "t0 and t1 both carry all thirteen kinds, so the default dump is CHARACTER-IDENTICAL to what this command emitted before per-fixture derivation existed. "
		 "An unknown fixture name is REFUSED OUT LOUD and falls back to t0, and a NON-default fixture prints a banner ABOVE the dump -- a dump nobody labelled is a false mismatch waiting to be filed. "
		 "`count` and `at-least` are NOT fixture-derived and MUST NEVER BECOME SO: they stay 1..30, never the live totals (CONVENTIONS section 1 -- a grammar capped at the live count silently emits 8 for a player who asked for 10 and makes the clarification undetectable). "
		 "It also prints ZONE_A_KIND_SEAM: which unit symbols the transcribed Zone A shows the model that THIS fixture's grammar forbids, split into NAMED (anywhere in Zone A) and DEMONSTRATED (inside a few-shot answer). "
		 "That is divergence D3 made visible, and it is printed BOTH WAYS here -- 'CLOSED' is a result, not a silence. "
		 "Siege.Llama.SpikeEval and Siege.Llama.SpikeBench deliberately print it ONLY when the seam is OPEN: a new line on their default path would move the instrument every number on record was taken with. "
		 "NEEDS NO MODEL -- this is the whole per-fixture capability, inspectable offline"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SiegeLlamaSpike::CmdSpikeGrammar));
