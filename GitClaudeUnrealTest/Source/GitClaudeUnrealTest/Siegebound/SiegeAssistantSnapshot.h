// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Siegebound/SiegeAssistantCommand.h"   // TASK-441: FSiegeAssistantCommand + ESiegeAssistantIntent for the non-orderable-kind guard. Pure-data header by design (its own doc comment sanctions this) - no new weight.
#include "Siegebound/SiegeMapMark.h"           // TASK-746 (MARK-§5): FSiegeMapMark. Header-only pure data on the TeamId.h precedent, included here BY LAW because AppendMarkPlaces takes it by reference - no new weight.
#include "Siegebound/TeamId.h"
#include "SiegeAssistantSnapshot.generated.h"

class USiegeAssistantVocabulary;
class UWorld;

/**
 *  One aggregated roster row (TASK-416; CONVENTIONS "In-match LLM command
 *  assistant (v1, text-only) - 2026-08-02" §9 pinned registry).
 *
 *  ⚠️ AGGREGATED BY (CardID, GroupId) - NEVER PER UNIT. Fourteen footmen are at
 *  most a handful of rows, never fourteen objects. A 4B model does not need
 *  per-unit HP to route "send 10 footmen", and per-unit rows would blow the
 *  ≤400-token snapshot cap on the first real mid-match capture (CONVENTIONS §8).
 *
 *  Kind is the CANONICAL LOWER-CASE unit symbol ("footman"), i.e. the card row
 *  name lower-cased - the same symbol the grammar's `kind` alternation is
 *  generated from, so a small model physically cannot name a unit that does not
 *  exist.
 *
 *  GroupId is the unit's live ASiegePlayerController group order id, or
 *  INDEX_NONE for an ungrouped unit. It is carried for the WAVE-1 EXECUTOR, and
 *  is deliberately NOT printed into the prompt: the model has no way to address
 *  a group (FSiegeAssistantCommand carries no group field) and a raw id in the
 *  prompt is exactly the kind of number a small model hallucinates arithmetic on.
 */
USTRUCT()
struct FSiegeAssistantRosterEntry
{
	GENERATED_BODY()

	/** Canonical lower-case unit symbol - the card row name lower-cased ("footman"). */
	UPROPERTY()
	FName Kind = NAME_None;

	/** Live count of this Kind sitting in this GroupId. Never zero for a row that exists. */
	UPROPERTY()
	int32 Count = 0;

	/** Live group-order id, or INDEX_NONE when ungrouped. Game-side only - never serialized into the prompt. */
	UPROPERTY()
	int32 GroupId = INDEX_NONE;

	/**
	 *  How many of this row's units may take a SELECTION-BEARING ZONE ORDER
	 *  (send / guard / ambush) - ASummonedUnit::IsGroupCommandEligible(), the
	 *  SHIPPING predicate, counted from the same per-unit call Capture() already
	 *  makes for KindOrderable. 0 <= Orderable <= Count.
	 *
	 *  ⚠️ DECLARED ADDITIVE EXTENSION TO THE CONVENTIONS §9 PIN, NOT A DRIFT -
	 *  SAY SO RATHER THAN LET A REVIEWER FIND IT (TASK-441). The §9 registry pins
	 *  this struct as `{ FName Kind; int32 Count; int32 GroupId; }`, and the
	 *  SETTINGS+CONFIRM §8 registry pins
	 *
	 *      bool ValidateCommandAgainstSnapshot(const FSiegeAssistantCommand&,
	 *                                          const TArray<FSiegeAssistantRosterEntry>& Roster,
	 *                                          ESiegeAssistantRejectReason&, FName&);
	 *
	 *  whose contract requires it to tell `KindNotOrderable` (present, but none
	 *  of them may take this order) apart from `KindUnknown` (not on the board at
	 *  all) FROM THE ROSTER ARRAY ALONE. Those two pins are not simultaneously
	 *  satisfiable while the row carries no orderability: the three pinned fields
	 *  answer "is it here" and cannot answer "may it be ordered". This field is
	 *  the MINIMUM addition that makes the pinned signature implementable, and it
	 *  is strictly additive - nothing is renamed, nothing is re-typed, no existing
	 *  reader changes.
	 *
	 *  ⚠️ AND IT COSTS THE PROMPT NOTHING, WHICH IS THE PART THAT MATTERS AT THIS
	 *  BATCH. `Roster` is used by Capture() for aggregation and by GetRoster() for
	 *  the executor; NO ZONE PRINTS IT (the roster block prints from the parallel
	 *  UnitKinds / KindTotals / KindOrderable / KindFollowable arrays - see
	 *  AppendRosterBlock). So zoneB_chars = 68 and zoneC_chars = 887 are
	 *  BYTE-UNCHANGED by this field, and the t0 tripwire cannot move.
	 *
	 *  📌 THAT CLAIM IS ABOUT *THIS FIELD* AND IT STILL HOLDS - but the t0 tripwire
	 *  itself was retired for the SHIPPED lane on 2026-08-04 by TASK-517, which
	 *  raised MaxRosterKinds 8 → 13 on Jonathan's ruling. The shipped builder's
	 *  zoneC_chars on a t0-shaped board moves 670 → 887, i.e. INTO agreement with
	 *  the spike fixture that always measured 887. Anything re-measured after that
	 *  change is not comparable to a figure measured before it. ⛔ Do not read this
	 *  paragraph as licence to "align" the spike lane: Plugins/SiegeLlama/** was
	 *  deliberately not touched.
	 *
	 *  ⚠️ THERE IS DELIBERATELY NO `Followable` COMPANION HERE. The pinned
	 *  ESiegeAssistantRejectReason has exactly three values and none of them means
	 *  "cannot follow", so a followable column would be unused state today. When a
	 *  KindNotFollowable reason is ever pinned, it arrives WITH its column; see the
	 *  Follow scoping note on ValidateCommandAgainstSnapshot below.
	 */
	UPROPERTY()
	int32 Orderable = 0;
};

/**
 *  THE IN-MATCH ASSISTANT'S GAME-STATE SNAPSHOT + THE THREE-ZONE PROMPT
 *  SERIALIZER (TASK-416 / LLM-A3; CONVENTIONS "In-match LLM command assistant
 *  (v1, text-only) - 2026-08-02" §3, §4, §8, §9 - signatures PINNED by §9,
 *  character-for-character).
 *
 *  M8 DECLARATION DUTY (CONVENTIONS §8, stated verbatim as required):
 *  "adds no replicated property, no new replicated class, no new relevancy tier."
 *  This is a client-local, read-only survey object; nothing here crosses the wire.
 *
 *  ⚠️ PINNED LINK, DESIGNED AND NOT A DEFECT (board manager ruling 8): this
 *  file's .cpp includes TASK-417's SiegeAssistantCommand.h (for the
 *  LogSiegeAssistant category) and SiegeAssistantVocabulary.h, and calls
 *  TASK-418's AAncientGround::FindNearestAncientGround. It CANNOT COMPILE ALONE
 *  and is not expected to - the batch compiles as ONE UBT module at TASK-420,
 *  exactly like TASK-395/396 did. Do NOT "fix" it by declaring a second log
 *  category or by hand-rolling a local nearest-ground search.
 *
 *  ── WHAT THIS IS, AND THE ONE SENTENCE THAT DECIDES WHETHER IT WORKS ──
 *
 *  THE SNAPSHOT IS NOT A WORLD DUMP. It is the set of things the player can
 *  legally NAME (CONVENTIONS §8): unit KINDS with counts, a named-place
 *  vocabulary, and quantized match facts. There are no individually-addressable
 *  units in v1 - nobody says "Footman #7" - and there are no coordinates
 *  anywhere in any zone (§3): every resolved FVector stays GAME-SIDE behind
 *  ResolvePlace(). That single rule deletes the hallucinated-number failure
 *  class outright, because the model has no number in front of it that means a
 *  position.
 *
 *  ⚠️ THE WORD "FIXED" CAME OUT OF THAT SENTENCE ON 2026-09-01 AND THE REASON IS
 *  RECORDED RATHER THAN LEFT AS A DIFF (TASK-746, MARK-§0). The place vocabulary
 *  is now the seven FIXED symbols PLUS the local player's own map marks -
 *  `circle_1` … - which he draws at runtime on the war map and which vanish when
 *  he deletes them. ⭐ THAT IS A NEW REFERENT CLASS IN THE COMMAND GRAMMAR, ⛔ not
 *  a UI feature: it is what makes "move all units to hold 1" and "ambush 2"
 *  executable sentences.
 *
 *  ⭐⭐ AND IT COSTS THE PROMPT ⛔ NOTHING IN ZONE A, WHICH IS THE WHOLE DESIGN.
 *  Zone A already defines `where` as "a place symbol from places in [FORCES]" -
 *  BY REFERENCE to Zone C's per-match `places:` line - and it already carries the
 *  comment "ZONE A PRINTS THE FULL FIXED VOCABULARY, THE GRAMMAR ENFORCES WHAT
 *  EXISTS THIS MATCH". A mark therefore needs ⛔ no new Zone-A line, ⛔ no new
 *  intent (`hold` already aliases `guard`, `ambush` already ships) and ⛔ no
 *  grammar code. ⛔ THE 5658-BYTE ZONE-A FREEZE IS UNTOUCHED AND MUST STAY SO.
 *  The cost that is NOT zero is Zone C's, and it is written beside
 *  SnapshotTrimBudgetChars where the budget it spends actually lives.
 *
 *  ⛔ THE AIRLOCK IS UNCHANGED AND IS NOT WEAKENED BY MARKS. A mark's CENTRE and
 *  RADIUS never enter any zone; only its SYMBOL does, and the centre is resolved
 *  GAME-SIDE by ResolvePlace exactly as `nearest_mine` already is.
 *
 *  ⚠️ THE SELECTION IS MULTI-KIND (manager ruling 15, CONVENTIONS §9 as
 *  corrected 2026-08-02). "Send 10 footmen WITH A SORCERER to the nearest
 *  ancient ground" is the feature's flagship sentence, so one command carries
 *  up to SiegeAssistantMaxSelectionKinds (3) index-aligned {kind, count} pairs.
 *  What that costs THIS file is that per-kind availability must be answerable
 *  for EVERY kind in one turn - which is why the roster block prints a live
 *  count per kind rather than a single "selected kind" figure. If footman and
 *  sorcerer availability were not both on screen, a two-kind order would open a
 *  clarification turn for a question the snapshot already knew the answer to,
 *  and every avoided turn is a whole inference call saved.
 *
 *  ── THE THREE ZONES, AND WHY THE LAYOUT IS LOAD-BEARING ──
 *
 *      ZONE A  1139 tok  STATIC  system prompt, schema, place vocabulary,
 *                                synonym table, 3 few-shots
 *      ZONE B    32 tok  SLOW    castle HP bands, mid owner, gold band
 *      ZONE C   320 tok  FAST    roster, pending-intent line, the utterance
 *
 *  ⚠️ THOSE ARE MEASURED TOKEN COUNTS, NOT ESTIMATES - TASK-413's spike run,
 *  2026-08-03, `Siege.Llama.SpikePrompt` against the shipping tokenizer with the
 *  model resident (Zone A 4314 chars, Zone B 68, Zone C 887; assembled prompt
 *  5349 chars / 1504 tok of a 2048 context). Everything below was written
 *  against the earlier estimates and its ARGUMENT survived the measurement
 *  intact; only the numbers moved.
 *
 *  ⚠️ ZONE A IS 1139 tok AS BUILT, NOT THE "~350" THE ORIGINAL PLAN ESTIMATED
 *  AND NOT THE "~600" THIS FILE ESTIMATED SECOND, AND IT MUST NOT BE TRIMMED
 *  TOWARD EITHER. The earlier 2158-char figure was measured with
 *  `synonyms: none`; with DA_AssistantVocabulary attached the synonym table
 *  doubles it, and the real figure is 4314 chars. That is DELIBERATE and it
 *  makes the spike BETTER, not worse. Bar #3 is the KV-reuse drop, whose ratio
 *  is (B+C)/(A+B+C): the reference derivation gave 165/765 = 78% and TASK-413
 *  MEASURED 77.1% on the shipped-worst-case bound, within 0.9 pp - so the bar
 *  PASSES and the reference figure is corroborated rather than assumed. Cutting
 *  Zone A to the nominal 350 gives 165/515 = 68% and the bar MARGINALLY FAILS.
 *  Zone A is also prefilled once and cached, so its size costs turn-1 TTFT only
 *  and barely touches bar #2, which measures a warm prefix. Trimming here would
 *  optimise the wrong number and could turn a GO into a GO-WITH-RESCOPE.
 *  (CONVENTIONS §8; TASK-419 WARN-5.)
 *
 *  The zones exist to keep the llama.cpp KV cache prefix warm - that is what
 *  turns a ~500-token prefill into ~150 from turn two onward. Every rule below
 *  protects that prefix, and breaking one silently destroys the reuse (a QA
 *  FAIL under CONVENTIONS §8, and one that shows up as a latency regression
 *  rather than as a wrong answer, which is why it is written down here):
 *
 *  - FIXED KEY ORDER, AND EVERY KEY IS ALWAYS EMITTED. An empty value prints
 *    `none`. Omitting a key shifts every downstream token and invalidates the
 *    cached prefix from that point on.
 *  - EVERYTHING IS QUANTIZED - HP in 10% steps, gold floored to 10s. This also
 *    stops a unit taking 1 damage from invalidating Zone B.
 *  - NO TIMESTAMPS, NO COORDINATES, NO ACTOR NAMES, NO POINTERS. Ever.
 *  - THE ROSTER IS SORTED BY FIXED DT_Cards ROW ORDER - never by count, never
 *    by distance, because both of those reorder as the match runs.
 *  - ZONE A IS BYTE-IDENTICAL FOR THE LIFE OF THE PROCESS. It reads NO member
 *    state at all (that is why it is the only builder taking an argument that
 *    is not live state), and changes only when DA_AssistantVocabulary changes.
 *    QA CRITERION: calling BuildZoneA twice in one process returns byte-
 *    identical strings. CALLER CONTRACT: pass the SAME vocabulary object every
 *    turn - passing null on one turn and an asset on the next changes Zone A
 *    and throws the prefix away.
 *
 *  ── COST ──
 *
 *  Capture() is called ONCE PER TYPED SENTENCE, NEVER ON A TICK (~0.2 ms over
 *  ~400 actors). The player types a handful of sentences per match.
 *
 *  ⛔ DO NOT ADD A UNIT REGISTRY, AN ACTOR CACHE, A DIRTY FLAG OR A
 *  SUBSCRIPTION LIST. That rejection is LAW (CONVENTIONS §4): it is new
 *  lifetime state to get wrong - spawn/death/destroy/level-travel/Play-Again
 *  bookkeeping, every one of which is a shipped bug class in this repo - to
 *  save time we never pay per frame. A future task proposing one is rejected on
 *  sight; cite the clause.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeAssistantSnapshot : public UObject
{
	GENERATED_BODY()

public:

	/**
	 *  THE SNAPSHOT'S CHARACTER **TRIM** BUDGET — AND IT IS THE **THIRD** ROLE, NOT
	 *  EITHER OF THE FIRST TWO. READ THE ROLE TABLE BEFORE TOUCHING THE NUMBER.
	 *
	 *  ⛔ THIS REPLACES `MaxSnapshotChars`, WHICH TASK-455 **RETIRED** RATHER THAN
	 *  RE-POINTED (CONVENTIONS "In-match LLM command assistant" §8, resolution 1:
	 *  *"after that commit a grep for MaxSnapshotChars returns nothing"*).
	 *  ⚠️ THE RETIREMENT IS THE FIX, NOT BOOKKEEPING, AND THE REASON IS SUBTLE:
	 *  `MaxSnapshotChars` was the AUTHORITY on the ≤400-token budget, and a char
	 *  cap standing in for a token budget is safe only while it assumes the
	 *  **SMALLEST** plausible chars/token ratio. The PRE-FILTER role that
	 *  succeeded it **INVERTS** that — a pre-filter must never reject what the
	 *  authority would accept, so it must assume the **LARGEST**. Carrying one
	 *  name across that boundary carries a safety label that now points the wrong
	 *  way, which is verbatim the defect this lane already shipped once (the 3.6
	 *  ratio labelled "conservative" while every measured reading sat below it).
	 *
	 *  ── THE THREE ROLES, AND WHICH LANE OWNS EACH ──
	 *
	 *    AUTHORITY    USiegeLlamaSubsystem::MaxSnapshotTokens = 400   TOKENS, PLUGIN
	 *                 Counted by llama_tokenize on the worker, so there is no ratio
	 *                 left to be wrong about — that is the whole point of moving it
	 *                 to tokens (§8 resolution 2). Over budget ⇒ the request is
	 *                 REJECTED, never truncated, and logged at Warning.
	 *
	 *    PRE-FILTER   USiegeLlamaSubsystem::SnapshotPreFilterMaxChars = 3000  CHARS, PLUGIN
	 *                 Game thread, before a single token is counted, which is what
	 *                 makes it cheap. Duties: CHEAP · FINITE · NEVER BINDING IN
	 *                 NORMAL PLAY. Assumes the LARGEST plausible ratio by
	 *                 construction (3000 = 400 × 7.5, ~2× the highest ratio ever
	 *                 measured here — 3.79, Zone A).
	 *
	 *    TRIM BUDGET  this constant = 1085                            CHARS, **HERE**
	 *                 What BuildZoneC sizes its elastic roster block against.
	 *                 Assumes the SMALLEST plausible ratio — the SAME direction the
	 *                 retired authority had. ⇒ THAT IS WHY THE VALUE DID NOT MOVE
	 *                 EVEN THOUGH THE NAME DID: only the *pre-filter* role inverts,
	 *                 and this is not that role.
	 *
	 *  ⚠️ WHY A THIRD CONSTANT EXISTS AT ALL — STATED SO IT IS NOT READ AS THE OLD
	 *  ONE IN DISGUISE. **The two plugin bounds both REJECT the turn; neither of
	 *  them trims anything.** If this budget were pointed at the pre-filter's 3000
	 *  the roster would stop collapsing, a wide board would sail past this file, and
	 *  the tokenizer would REFUSE THE WHOLE TURN — the player gets "the assistant is
	 *  unavailable" instead of an answer computed against a slightly narrower
	 *  roster. **A trimmer and a bound are different jobs, and only a trimmer can
	 *  degrade gracefully.** ⛔ Pointing this at 3000 is a QA FAIL, not a
	 *  simplification.
	 *
	 *  ── THE VALUE, UNCHANGED, ON ITS ORIGINAL MEASURED OPERANDS ──
	 *
	 *      1085 = 400 tokens × 2.71 chars/token
	 *
	 *  2.71 is the MEASURED chars/token of ZONE B + ZONE C — the region this budget
	 *  governs — from the shipping tokenizer with the model resident
	 *  (`Siege.Llama.SpikePrompt`, fixture t0: 955 chars / 352 tokens). BOTH
	 *  OPERANDS ARE MEASUREMENTS. ⛔ IT IS DELIBERATELY **NOT** RE-DERIVED HERE:
	 *  moving it would move Zone C's bytes, and CONVENTIONS §12c freezes them for
	 *  this wave (*"ZONE B AND ZONE C BYTES DO NOT MOVE IN THIS WAVE"* — the `t0`
	 *  tripwire, on which bar #3's reproduction depends).
	 *
	 *  ⚠️ AND IT IS A TRIMMER, NOT A GUARANTEE — SAY IT PLAINLY RATHER THAN LET THE
	 *  WORD "BUDGET" IMPLY OTHERWISE. At Zone B's own stricter measured ratio (2.13)
	 *  a 1085-char snapshot can still be ~509 tokens, over the 400 the authority
	 *  enforces. This budget makes that outcome UNLIKELY; **the tokenizer is what
	 *  makes it IMPOSSIBLE.** Calling a proxy a guarantee is the defect §8 spent a
	 *  whole section retiring, and 850 (= 400 × 2.13) is the number to move to if
	 *  Zone C ever becomes as symbol-dense as Zone B — read the ratio, never re-guess it.
	 *
	 *  ⚠️ THE BUDGET COVERS ZONE B + ZONE C — the live snapshot — NOT Zone A. Zone A
	 *  is the static preamble, bounded separately by §8's ZONE-A SIZE BOUND
	 *  assertion, which the plugin runs the moment USiegeAssistantComponent
	 *  registers the prefix. Zone A measures 4,314 chars / 1,139 tok as built; see
	 *  the zone table on the class above for why growing it PASSES the KV-reuse bar
	 *  and trimming it toward 350 marginally FAILS it. ⛔ Do not read this constant
	 *  as licence to shrink Zone A (TASK-419 WARN-5).
	 *
	 *  ⚠️ IT IS COUPLED TO MaxRosterKinds, AND SINCE TASK-517 THE COUPLING IS TIGHT
	 *  RATHER THAN THEORETICAL. Zone C is budgeted at
	 *  SnapshotTrimBudgetChars - ZoneBCharReserve = 1085 - 192 = 893 chars, less the
	 *  head and the tail. The old text here said "at MaxRosterKinds = 8 a realistic
	 *  board clears it with ~220 to spare; raise it toward 13 and it BITES
	 *  IMMEDIATELY (887 against 893)" - MaxRosterKinds IS 13 NOW (Jonathan's
	 *  ruling, see its comment), so that is no longer a warning about a future
	 *  edit, it is the shipped operating point:
	 *
	 *      head 108 + roster 621 + tail 158 = 887 of 893  ⇒  ~6 chars spare
	 *
	 *  counted on the t0-shaped 13-kind board with the default 61-char `order:`
	 *  line.
	 *
	 *  ⚠️⚠️ AND SINCE TASK-746 THE HEAD IS NO LONGER FIXED AT 108 - IT GROWS WITH
	 *  THE PLAYER'S MAP MARKS, WHICH IS THE OTHER HALF OF THAT ~6-CHAR HEADROOM'S
	 *  STORY AND IS RECORDED BESIDE IT RATHER THAN IN A HANDOFF (MARK-§2):
	 *
	 *      head(M marks) = 108 + 10*M          (`, circle_N` is exactly 10 chars)
	 *      roster budget = 893 - head - tail = 627 - 10*M   (at the 158-char tail)
	 *      13-kind roster block = 621
	 *   ⇒  621 > 627 - 10*M  whenever  M >= 1
	 *
	 *  ⛔ THE FIRST MARK ALREADY COLLAPSES THE ROSTER TAIL ON A FULL BOARD. That is
	 *  a DECLARED COST, ⛔ not a defect and ⛔ not a reason to trim marks: the
	 *  collapse is the SHIPPED elastic trimmer doing its job, `other_kinds:` NAMES
	 *  every kind it hid (so a collapse costs COUNTS, never EXISTENCE), and
	 *  BuildZoneC logs at Warning every time it degrades. ⛔ DO NOT buy the room by
	 *  raising this constant; the one honest lever is still ZoneBCharReserve, which
	 *  TASK-528 owns and which may only move from a PRINTED zoneB_chars reading.
	 *
	 *  ⚠️ ~7 MORE CHARACTERS OF TYPED TEXT RE-COLLAPSE THE TAIL, and both
	 *  player-text lines below can each spend up to MaxUtteranceBytes, so a deep
	 *  collapse is NORMAL rather than exceptional. That is correct behaviour
	 *  against a real budget rather than a defect - and it is exactly why
	 *  BuildZoneC's collapse logs every time it degrades, and why `other_kinds:`
	 *  prints the collapsed kinds' NAMES (a collapse costs the model those kinds'
	 *  COUNTS, never their EXISTENCE). ⛔ DO NOT RAISE THIS BUDGET TO BUY THAT
	 *  ROOM. The honest lever is ZoneBCharReserve; see its comment for why
	 *  TASK-455 could not move that one either.
	 *
	 *  ⚠️ BOTH PLAYER-TEXT LINES SPEND THIS BUDGET AND NEITHER IS TRIMMED BY IT —
	 *  the roster absorbs all of it (CONVENTIONS §8: never truncate the utterance).
	 *  `pending:` and `order:` are each bounded by MaxUtteranceBytes (240) and both
	 *  sit in BuildZoneC's Tail, so they are subtracted BEFORE the roster is given a
	 *  budget. Worst case with both lines at their cap: 1085 - 192 reserve - ~137
	 *  head - ~566 tail leaves the roster ~190 chars, so the collapse is DEEP — and
	 *  it is LOGGED, at Warning, with its cause named. That is `qa/TASK-416.md`
	 *  WARN-1 confirmed, bounded, and observable rather than denied.
	 */
	static constexpr int32 SnapshotTrimBudgetChars = 1085;

	/**
	 *  Zone B's guaranteed slice of SnapshotTrimBudgetChars. Zone B is four short
	 *  fixed keys and never approaches this; the reserve exists so BuildZoneC can
	 *  size its roster budget WITHOUT calling BuildZoneB (which would double the
	 *  work and couple the two builders). BuildZoneB logs once if it ever exceeds it.
	 *
	 *  ⛔ 192 IS AN OVER-CHARGE, IT IS THE ACKNOWLEDGED HONEST LEVER, AND TASK-455
	 *  **DELIBERATELY LEFT IT ALONE.** Read this before "finishing the job": every
	 *  char over-charged here is a char stolen from the roster, and the recorded
	 *  Zone B readings are 68 (t0) / 71 (t1) — roughly 124 chars of over-charge.
	 *  The spec that owns this constant says, in terms, **"set it from the SHIPPED
	 *  builder's PRINTED worst case"** and **"re-measure it, do not eyeball it."**
	 *
	 *  ⚠️ THE PRINTED FIGURE DOES NOT EXIST YET, AND THAT IS THE WHOLE REASON THIS
	 *  IS STILL 192. Both candidate instruments fail on the word *printed*:
	 *    - **68 / 71 were printed by `Siege.Llama.SpikePrompt`, which measures the
	 *      SPIKE's `AppendZoneB` — not this file's `BuildZoneB`.** A measurement of
	 *      the other lane is not a measurement of this one; CONVENTIONS §12g carries
	 *      a standing WARN making exactly that point about Zone A.
	 *    - **`USiegeAssistantComponent::ReportFirstCapture` prints `zoneB_chars`
	 *      from THIS builder — and it has NEVER EXECUTED.** The batch is uncompiled
	 *      and TASK-447 is the single compile gate.
	 *  ⛔ A DERIVED WORST CASE IS NOT A MEASUREMENT. Substituting one here is the
	 *  precise defect CONVENTIONS §12g exists to prevent, and this constant's own
	 *  history is the argument: the number it would replace was itself eyeballed.
	 *
	 *  ⇒ THE READING IS ONE LINE AWAY AND IS NAMED HERE SO IT IS NOT LOST. After
	 *  TASK-447 compiles, open the console once and read `zoneB_chars` off the
	 *  `FIRST LIVE CAPTURE` line, then size this from the WIDEST value each of the
	 *  four fixed keys can take (both castles at `100%`, `mid: neutral`, a
	 *  four-digit `gold:` band) — one live board is a sample, not a worst case.
	 *  ✅ Lowering it can only WIDEN the roster, so it is safe in the one direction
	 *  it will ever be moved.
	 */
	static constexpr int32 ZoneBCharReserve = 192;

	/**
	 *  Roster kinds printed in full before the tail collapses into one
	 *  `other_kinds:` line. Over budget ⇒ aggregate harder, never spill.
	 *
	 *  ⚖️ 13 IS JONATHAN'S RULING (2026-08-04, TASK-517), TAKEN VIA AskUserQuestion
	 *  WITH THE COST IN FRONT OF HIM, AND IT IS NOT A TUNER'S NUMBER. It was 8,
	 *  with the note that "eight covers a realistic mid-match board" - and that
	 *  note was WRONG in the one way that mattered. DT_Cards has THIRTEEN
	 *  commandable kinds and the roster prints in fixed DT_Cards ROW ORDER, in
	 *  which `Sorcerer` is the LAST row - so at 8 the Sorcerer was ALWAYS the
	 *  first kind collapsed, and the collapse line printed COUNTS ONLY. The token
	 *  `sorcerer` never reached the model at all, Zone A's "if the unit named is
	 *  not a kind in [FORCES], answer unsupported" rule then refused it, and the
	 *  player's report was *"whenever I say all units, it doesn't seem to include
	 *  sorcerers even when they were spawned."* (CONVENTIONS AS-§20.2.)
	 *
	 *  ⛔ 13, NOT 12, NOT "all kinds, dynamically". It stays a static constexpr:
	 *  the grammar, the budget arithmetic and two log lines all read it, and a
	 *  runtime-sized cap would make the roster's width depend on the board, which
	 *  is the one thing the fixed-order rule exists to prevent.
	 *
	 *  ⚠️ THE COST, ACCEPTED BY ITS OWNER, WRITTEN HERE RATHER THAN IN A FOOTNOTE:
	 *  A 13-KIND BOARD MEASURES 887 CHARS OF ZONE C AGAINST A 893-CHAR BUDGET
	 *  (SnapshotTrimBudgetChars 1085 - ZoneBCharReserve 192). THAT IS ~6 CHARS OF
	 *  HEADROOM, and ~7 extra characters of typed `order:` text re-collapse the
	 *  tail. A FOURTEENTH COMMANDABLE KIND FORCES A RE-TUNE. ⛔ Do NOT buy the
	 *  room by raising SnapshotTrimBudgetChars (its own comment forbids it, and
	 *  the three-role table is why); the one honest lever is ZoneBCharReserve,
	 *  which must be set from a PRINTED zoneB_chars reading and never a derived
	 *  one. (CONVENTIONS AS-§20.3.)
	 *
	 *  ⭐ AND THE CAP IS ONLY HALF THE FIX - READ AppendRosterBlock BEFORE
	 *  TOUCHING EITHER HALF. Because the roster is elastic, a longer sentence can
	 *  still collapse the tail at any cap; that is why `other_kinds:` now prints
	 *  the collapsed kinds' NAMES. A collapse may hide a kind's NUMBERS; it may
	 *  never hide its NAME. Lowering this constant does not re-open the defect,
	 *  but it does cost the model every collapsed kind's counts.
	 *
	 *  ✅ THIS CONVERGES THE TWO LANES RATHER THAN WIDENING THEM (FINE-TUNE §3 D2,
	 *  RESOLVED). The spike fixture always printed all 13 kinds and a hardcoded
	 *  `other_kinds: none`; the shipped builder printed 8 and a computed collapse
	 *  line. At 13 the shipped builder emits 13 rows and computes `none`, which is
	 *  byte-identical to the fixture - t0's sealed bytes were not touched and the
	 *  harness was not touched. The lane that moved is the one that was wrong
	 *  about the game.
	 */
	static constexpr int32 MaxRosterKinds = 13;

	/**
	 *  THE PER-LINE CAP ON PLAYER-CONTROLLED TEXT, COUNTED IN UTF-8 BYTES.
	 *
	 *  ⚠️ RENAMED FROM `MaxUtteranceChars` BY TASK-433 (BLOCKER-2), AND THE RENAME
	 *  IS THE FIX, NOT COSMETICS - a constant named "Chars" that measures bytes
	 *  would be a fresh instance of the very defect this closes. The old name
	 *  counted FString CODE UNITS (UTF-16 on Windows) while the budget it proxies
	 *  for is TOKENS OVER UTF-8, and the 2.71 chars/token ratio behind
	 *  SnapshotTrimBudgetChars (then named MaxSnapshotChars) was calibrated on
	 *  ASCII, where one char is exactly one byte. Outside ASCII the two diverge and
	 *  the cap silently OVER-ADMITS: 240 code units of emoji (surrogate pairs - 2
	 *  units and 4 bytes each) is ~120 glyphs and 360-480 tokens against a 400-token
	 *  B+C budget, while every character-based check reports 240 of 1085 and looks
	 *  healthy.
	 *
	 *  ⚠️ THIS IS THE ONE STRING IN THE SNAPSHOT THAT COMES FROM OUTSIDE THE
	 *  PROGRAM. The ASCII law at the top of SiegeAssistantSnapshot.cpp is scoped to
	 *  prompt LITERALS, which are author-written and were never the risk. The
	 *  utterance and the pending line are not literals, so nothing constrained
	 *  them: the assumption crossed the authorship boundary with its name and its
	 *  value unchanged, and stopped being true on the way.
	 *
	 *  WHY BYTES AND NOT A TOKENIZER. Bytes are correct-by-construction for ASCII
	 *  (1 byte = 1 char, so NO EXISTING MEASURED FIGURE MOVES) and strictly
	 *  conservative for everything else: the worst case per line falls from 720
	 *  bytes (240 code units x 3 bytes for BMP text such as CJK) to 240 - exactly
	 *  3x tighter, matching the ~3x over-admission that was measured.
	 *  ⚠️ IT NARROWS THE GAP WITHOUT CLOSING IT. Text that tokenizes at the
	 *  byte-fallback floor still costs ~1 token per byte, i.e. up to 240 tokens on
	 *  a line the 2.71 ratio prices at ~89. THAT RESIDUAL IS DELIBERATE AND
	 *  ACCEPTED: TASK-423 supersedes this by enforcing the budget against the
	 *  tokenizer directly, and a second tokenizer here would be the thing it has to
	 *  delete. Do not build one.
	 *
	 *  A pasted paragraph must not become the whole context - and per TASK-433
	 *  BLOCKER-1 the cut is NO LONGER SILENT: see SanitizeForPrompt and BuildZoneC.
	 */
	static constexpr int32 MaxUtteranceBytes = 240;

	/**
	 *  SURVEYS THE WORLD FOR ONE TYPED SENTENCE. Six LOGICAL passes - units,
	 *  castles, gold nodes, ancient grounds, the capture zone, and the hero -
	 *  run ONCE PER SENTENCE, NEVER PER TICK.
	 *
	 *  ⚠️ SIX LOGICAL PASSES IS NOT SIX TRAVERSALS, AND THE HONEST NUMBER IS UP TO
	 *  EIGHT (TASK-433). Counted from this file: SIX run unconditionally -
	 *  FindNearestCastleForTeam twice (own, enemy), FindNearestAncientGround for
	 *  the near ground, FindBestMineFor, the ACaptureZone loop and the
	 *  ASummonedUnit loop - plus the FAR ground whenever an enemy castle stands,
	 *  plus the AHeroCharacter fallback loop when the hero is not possessed. So
	 *  SEVEN in the normal shipped case and EIGHT at worst. CONVENTIONS §4 says
	 *  "six TActorIterator passes", which understates the traversals by ~33 %; its
	 *  ~0.2 ms conclusion survives that comfortably, but quote the count from HERE.
	 *
	 *  Five of those traversals happen inside SHIPPED STATIC FINDERS reused
	 *  verbatim rather than re-derived (CONVENTIONS §4 + the FindBestMineFor
	 *  idiom): ACastle::FindNearestCastleForTeam, AGoldNode::FindBestMineFor and
	 *  AAncientGround::FindNearestAncientGround - THREE functions, FIVE calls,
	 *  because two of them are called twice (own/enemy castle, near/far ground).
	 *  Each is exactly one TActorIterator, verified against their bodies. Still
	 *  unmeasurable next to one inference call: re-deriving their loops here to
	 *  save a traversal would trade a shipped, QA'd selection rule for a second
	 *  copy that drifts.
	 *
	 *  ⛔ AND TASK-547's REGION CAPTURE ADDED **ZERO** TRAVERSALS TO THAT COUNT -
	 *  SAY SO RATHER THAN LET A REVIEWER RE-COUNT. All three region boxes come off
	 *  actors the passes above ALREADY hold: NearGround / FarGround (pass 3) and
	 *  the ACaptureZone (pass 5), through the public GetZoneHalfExtent() on each.
	 *  A fresh TActorIterator for regions is a QA FAIL (CONVENTIONS AS-§21.4), and
	 *  so is a registry, an actor cache, a dirty flag or a subscription list (§4).
	 *
	 *  Everything is re-derived from scratch every call: this object holds no
	 *  actor pointers, no weak pointers and no lifetime state, so a unit dying
	 *  between two sentences can never leave a stale row behind.
	 *
	 *  NULL-SAFE: a null World (or a world with no castles, no grounds, no
	 *  player state) leaves the snapshot EMPTY rather than partially filled, and
	 *  every fixed key still prints - as `none`.
	 *
	 *  @param World world to survey; null ⇒ the snapshot resets to empty
	 *  @param Team the ORDERING player's team - "own"/"enemy" everywhere below is relative to it
	 */
	void Capture(UWorld* World, ETeamId Team);

	/**
	 *  ZONE A - THE STATIC PREAMBLE. System prompt, output schema, the intent
	 *  glossary, the fixed place vocabulary, the synonym table and three
	 *  few-shots.
	 *
	 *  ⚠️ READS NO MEMBER STATE, BY CONSTRUCTION - it is const and touches
	 *  nothing Capture() wrote, which is precisely what makes it byte-identical
	 *  for the life of the process and lets llama_memory_seq_rm keep the
	 *  prefix. It is a member (not a static) only because the §9 registry pins
	 *  it as one. Making it state-dependent is a QA FAIL.
	 *
	 *  ⚠️ THE SYMBOL ABOVE IS `llama_memory_seq_rm` (vendored llama.h:735, verified).
	 *  IT WAS DOCUMENTED HERE AND IN BuildZoneA's BODY AS `llama_kv_cache_seq_rm`,
	 *  WHICH EXISTS NOWHERE IN THE VENDORED HEADER - corrected by TASK-433 (WARN-3).
	 *  The dead spelling is spelled out ONCE, here, on purpose: this is the recorded
	 *  `use_mmap` lesson - "the name a search will fail to find" - so a reader who
	 *  arrives from an older doc grepping the wrong name lands on this correction
	 *  instead of on nothing. The plugin lane was always right (SiegeLlamaSpike.cpp
	 *  calls llama_memory_seq_rm); only the game lane's comments were wrong.
	 *
	 *  @param Vocabulary DA_AssistantVocabulary (TASK-421's asset instance); null ⇒ the synonym block prints `none`, deterministically
	 */
	FString BuildZoneA(const USiegeAssistantVocabulary* Vocabulary) const;

	/**
	 *  ZONE B - THE SLOW ZONE. Four fixed keys, always all four, always in this
	 *  order: own_castle_hp, enemy_castle_hp, mid, gold. Every value is a BAND,
	 *  not a number - HP to 10%, gold floored to 10s - so a unit taking 1 damage
	 *  or a single income tick does not invalidate the cached prefix.
	 */
	FString BuildZoneB() const;

	/**
	 *  ZONE C - THE FAST ZONE. Fixed key order: places, roster, stances, hero,
	 *  pending, order. Regenerated every turn by definition (the utterance is in
	 *  it), so it is the only zone allowed to vary in length.
	 *
	 *  Over-budget behaviour (CONVENTIONS §8): the ROSTER TAIL collapses
	 *  deterministically - first into a single `other_kinds:` line beyond
	 *  MaxRosterKinds, then by dropping further tail rows until the character
	 *  budget fits. The utterance and the pending line are never truncated by THAT
	 *  budget; the roster absorbs all of it.
	 *
	 *  ⚠️ BUT THEY ARE CAPPED, AND A CAP IS A TRUNCATION LIKE ANY OTHER.
	 *  MaxUtteranceBytes bounds each of the two player-text lines. This was
	 *  documented here as "a separate, always-on sanitiser" and thereby treated as
	 *  EXEMPT from the observable-truncation condition. THAT EXEMPTION WAS THE
	 *  DEFECT (TASK-433 BLOCKER-1). The condition binds "any enforcement of the
	 *  snapshot budget, by any mechanism, under any name" - it is anchored to THE
	 *  ACT OF TRUNCATING rather than to the constant performing it, precisely so
	 *  that renaming the cutter cannot orphan it, and "sanitiser" is a name.
	 *
	 *  ⚠️ EVERY COLLAPSE AND EVERY LINE CUT IS REPORTED, INCLUDING THE COLLAPSE AT
	 *  EXACTLY THE CAP. Three causes are named separately, because they call for
	 *  different fixes: the MaxRosterKinds cap, the character budget, and
	 *  MaxUtteranceBytes on a player-text line. Each reports at Warning on first
	 *  occurrence and on every escalation, plus an unlatched per-turn record at
	 *  Verbose. The roster used to log ONCE and only for the budget case, so a
	 *  >MaxRosterKinds board degraded the prompt silently forever (TASK-419
	 *  WARN-5); the two player lines used to log NOTHING AT ALL (TASK-433). See
	 *  WarnedRosterKindsPrinted and WarnedUtteranceBytes.
	 *
	 *  ⚠️ A CUT LINE IS ALSO MARKED IN THE PROMPT, NOT ONLY IN THE LOG - see
	 *  SanitizeForPrompt for why that design call went the way it did. A collapsed
	 *  roster still prints `other_kinds:`, so the model is TOLD something was
	 *  hidden; a truncated `order:` line left it nothing, and a half-sentence
	 *  still yields a well-formed command from a grammar-constrained sampler. That
	 *  is the valid-shaped-wrong-command failure CONVENTIONS §1 exists to prevent,
	 *  arriving through the one string the player actually typed.
	 *
	 *  @param Utterance   the player's raw typed sentence; flattened + capped, never interpreted here
	 *  @param PendingLine the FSM's GAME-AUTHORED pending-intent line (§1: each clarification turn is a fresh single-turn call, and this line - never the model's own previous output - is what carries context forward). Empty ⇒ `none`
	 */
	FString BuildZoneC(const FString& Utterance, const FString& PendingLine) const;

	/** Aggregated roster rows, sorted by fixed DT_Cards row order then GroupId. Empty until Capture() runs. */
	const TArray<FSiegeAssistantRosterEntry>& GetRoster() const { return Roster; }

	/**
	 *  The place symbols that ACTUALLY RESOLVE this match. This is what TASK-417's
	 *  grammar generates its `where` alternation from - so a place that does not
	 *  exist on this map cannot be emitted at all. Zone A still prints the FULL
	 *  FIXED vocabulary (it must stay static); the grammar is what enforces
	 *  existence.
	 *
	 *  ⚠️ THE ORDER IS "FIXED VOCABULARY, THEN MARKS", AND THE COMMENT SAYS SO
	 *  RATHER THAN STILL SAYING "fixed vocabulary order" (TASK-746). The seven
	 *  shipped symbols come first, in the PlaceVocabulary table's order - which is
	 *  the order Zone A prints them in, and that mirror is deliberate. The local
	 *  player's map marks (`circle_1` …) are APPENDED after them, in ascending
	 *  number order, by AppendMarkPlaces.
	 *
	 *  ⭐⭐ AND THAT APPEND IS THE ENTIRE AI HALF OF THE MAP-MARK FEATURE
	 *  (MARK-§1). The shipped Zone A defines `where` as "a place symbol from places
	 *  in [FORCES]" - BY REFERENCE TO THIS LIST, not to Zone A's own fixed one - so
	 *  a mark published here is a legal `where` by that line's own words, and
	 *  USiegeAssistantGrammar::Build makes it GBNF-samplable, both with ⛔ ZERO
	 *  prompt bytes and ⛔ ZERO grammar code. ⛔ Zone A is NOT edited for it.
	 */
	const TArray<FName>& GetPlaceNames() const { return PlaceNames; }

	/**
	 *  ── THE REGION ACCESSORS (TASK-547; CONVENTIONS AS-§21.4 + the AS-§21.9
	 *  pinned registry, character-for-character) ──
	 *
	 *  The REGION-BEARING SUBSET of GetPlaceNames(), in the same fixed vocabulary
	 *  order. A place is region-bearing IFF A SHIPPED `IsPointInZone` ANSWERS FOR
	 *  IT - that is the whole ruling, and it admits exactly three of the seven:
	 *  `mid` (ACaptureZone) and both ancient grounds (AAncientGround).
	 *
	 *  ⛔⛔ MAP MARKS ARE ⛔ NOT IN THIS LIST AND MAY ⛔ NEVER BE ADDED TO IT
	 *  (TASK-746, MARK-§3 M-6). A circle trivially satisfies AS-§21.4's test, so
	 *  this is a SCOPE FENCE rather than a capability claim - and it is the fence
	 *  that protects the byte-freeze. The `ZONE = ` line Zone A prints is GENERATED
	 *  FROM THE FIXED TABLE'S bHasRegion COLUMN (SiegeAssistantSnapshot.cpp
	 *  :1088-1107); a per-match entry there would make ZONE A VARY, destroying the
	 *  5658-byte freeze and the KV prefix with it. ⇒ the one shape that would cost
	 *  real Zone-A bytes is the one shape v1 refuses. Available later at a stated
	 *  price; ⛔ not by a tidying edit.
	 *
	 *  ⛔ THE OTHER FOUR ARE NOT MISSING FEATURES, THEY ARE A RULING. `own_castle`,
	 *  `enemy_castle`, `nearest_mine` and `hero` have NO region primitive anywhere
	 *  in the shipped code, so giving them one would mean INVENTING A RADIUS -
	 *  a number nobody chose, picked to make a feature compile. See
	 *  FPlaceDefinition::bHasRegion in the .cpp for the full argument and
	 *  AS-§21.11 for the designed consequence ("everyone at my castle" will not
	 *  filter, by ruling, and a report of it is not a bug).
	 *
	 *  This is what USiegeAssistantGrammar::Build generates its `zone` alternation
	 *  from - so a region the map does not have this match cannot be emitted at
	 *  all, exactly as GetPlaceNames() does for `where`. ⚠️ Zone A still prints the
	 *  FULL region vocabulary (it must stay byte-identical for process life); the
	 *  grammar is what enforces existence.
	 */
	const TArray<FName>& GetRegionPlaceNames() const { return RegionPlaceNames; }

	/**
	 *  THE COORDINATE AIRLOCK, SECOND DOOR (CONVENTIONS §3 / AS-§21.4). Maps a
	 *  region-bearing place symbol back to the 2D XY BOX the executor tests unit
	 *  positions against.
	 *
	 *  ⚖️ SNAPSHOT-TIME GEOMETRY, EXECUTION-TIME MEMBERSHIP - AND THE SPLIT IS THE
	 *  DESIGN, NOT AN ACCIDENT OF WHERE THE CODE SITS. This returns the box that
	 *  existed when Capture() ran; the "is this unit inside it" test runs later,
	 *  in the executor, against LIVE unit positions (FSiegeAssistantRegionStatics::
	 *  IsPointInRegion). ⚠️ THE UNITS MOVE, THE GROUNDS DO NOT, so a unit that
	 *  walked out of the ground between the sentence and the order landing must not
	 *  be selected - and it is not.
	 *
	 *  ⚠️ DECLARED RESIDUAL, STATED RATHER THAN LEFT TO BE FOUND: a ground DESTROYED
	 *  between capture and execution leaves a stale centre here. That is the
	 *  IDENTICAL staleness ResolvePlace already carries for the DESTINATION - the
	 *  same risk profile, not a new one - and it is bounded the same way: Capture()
	 *  runs once per typed sentence, so the window is one inference call.
	 *
	 *  ⛔ THE BOX IS PAIRED WITH THE SHIPPED PREDICATE, NEVER RE-DERIVED. The centre
	 *  is the actor location and the extent is GetZoneHalfExtent(), which is exactly
	 *  what AAncientGround::IsPointInZone and ACaptureZone::IsPointInZone test
	 *  against; reading a component bounds or a decal size instead would answer a
	 *  different question than the shipped membership test.
	 *
	 *  @return false - leaving BOTH out-params UNTOUCHED, exactly as ResolvePlace
	 *          does - for an unknown place, a place with no region primitive, or a
	 *          region-bearing place that did not resolve this match. ⛔ There is no
	 *          "whole map" fallback: a region named and not resolved must become a
	 *          REFUSAL, never an unfiltered order (AS-§21.5).
	 */
	bool ResolvePlaceRegion(FName Place, FVector& OutCentre, FVector2D& OutHalfExtent) const;

	/**
	 *  Every canonical unit symbol alive on the ordering team, sorted by fixed
	 *  DT_Cards row order. Feeds the grammar's `kind` alternation.
	 *
	 *  ⚠️ NEVER TRUNCATED, unlike the printed roster: the prompt may collapse
	 *  its tail into `other_kinds:` to stay in budget, but the GRAMMAR must
	 *  still admit every kind the player owns - otherwise a legitimate order
	 *  becomes unsayable, which is the valid-shaped-wrong-command failure
	 *  CONVENTIONS §1 exists to prevent.
	 */
	const TArray<FName>& GetUnitKinds() const { return UnitKinds; }

	/**
	 *  ── THE ORDERABILITY ACCESSORS (TASK-441; CONVENTIONS "Settings screen +
	 *  the assistant CONFIRM STEP + the non-orderable-kind guard (2026-08-03)"
	 *  §2, §6, §8 - ADDITIVE to the §9 pin, nothing renamed) ──
	 *
	 *  ⛔ THESE EXPOSE WHAT Capture() ALREADY TALLIES. THEY DO NOT RECOMPUTE, DO
	 *  NOT RE-TALLY AND DO NOT ADD A TRAVERSAL. KindOrderable has been filled on
	 *  every capture since TASK-416 and was merely private; the printed roster
	 *  line has been reporting it to the MODEL all along while no code path could
	 *  read it. ⛔ A unit registry, an actor cache, a dirty flag or a subscription
	 *  list for this is REJECTED ON SIGHT - cite CONVENTIONS "In-match LLM command
	 *  assistant" §4.
	 *
	 *  ⚠️ ORDERABLE, NOT FOLLOWABLE - AND THE DISTINCTION IS THE WHOLE POINT.
	 *  KindOrderable counts ASummonedUnit::IsGroupCommandEligible() ("may I SEND
	 *  these?" - send / guard / ambush, the zone orders). KindFollowable counts
	 *  IsFollowCommandEligible() ("may these FOLLOW?"). The two genuinely differ
	 *  on shipped units: the Cleric follows and cannot take zone orders, and the
	 *  Ogre / Sapper do neither. Exposing the wrong column here would be a silent
	 *  wrong answer, so it is verified against the code that fills it
	 *  (SiegeAssistantSnapshot.cpp, the ASummonedUnit loop in Capture) rather than
	 *  against the name.
	 *
	 *  ⚠️ AND THE M8 P2 FLAG TRAVELS WITH THEM, UNCHANGED: on a Red capture BOTH
	 *  eligibility predicates hardcode `Team == ETeamId::Blue`, so
	 *  Capture(World, ETeamId::Red) returns correct totals and an ALL-ZERO
	 *  orderable column. That is correct for v1 (host/standalone, Blue is the only
	 *  commanding player) and it means these accessors answer `false` / `0` for
	 *  every Red kind. The fix belongs in the SHIPPED predicates when P2 makes Red
	 *  a real commanding player - never here (CONVENTIONS §8's flagged item).
	 *
	 *  ⚠️ BOTH ANSWER 0 / false FOR "PRESENT BUT NONE ELIGIBLE" *AND* FOR "NOT ON
	 *  THE BOARD AT ALL", AND THAT COLLAPSE IS DELIBERATE - these are the cheap
	 *  in-hand questions the FSM asks about ONE kind. A caller that must tell the
	 *  two apart calls ValidateCommandAgainstSnapshot, which is handed the roster
	 *  rows and therefore has the presence answer that these two do not.
	 *
	 *  Declared in the CONVENTIONS §8 registry's order, character-for-character.
	 */
	bool IsKindOrderable(FName Kind) const;

	/** Companion to IsKindOrderable, and the one the shortfall path wants: the live count. 0 == not orderable. See IsKindOrderable's comment for the orderable-vs-followable distinction, the absent-vs-ineligible collapse and the Red-capture flag. */
	int32 GetOrderableCount(FName Kind) const;

	/**
	 *  THE COORDINATE AIRLOCK (CONVENTIONS §3). Maps a canonical place symbol
	 *  the model emitted back to the world location the executor needs.
	 *
	 *  ⚠️ THIS IS THE ONLY PLACE AN FVector LEAVES THIS OBJECT, AND IT ONLY EVER
	 *  GOES GAME-SIDE. The model sees `ancient_ground_near`; it never sees, and
	 *  can never emit, a number that means a position.
	 *
	 *  @return false (leaving OutLocation UNTOUCHED) for an unknown or unresolved place
	 */
	bool ResolvePlace(FName Place, FVector& OutLocation) const;

	// =========================================================================
	//  MAP MARKS - THE REFERENT (TASK-746; CONVENTIONS MARK-§1 / §2 / §3 M-6)
	//
	//  ⭐⭐ READ MARK-§1 BEFORE EDITING ANYTHING BELOW. IT IS THE PROOF THAT THIS
	//  FEATURE COSTS ZONE A **ZERO** BYTES, AND EVERY LINE HERE SHIPS INSIDE THAT
	//  PROOF RATHER THAN RE-DERIVING IT. Re-verified at source 2026-09-01:
	//
	//    :1109  `WHERE  = a place symbol from places in [FORCES], or "none"`
	//           ⇒ `where` is defined BY REFERENCE TO ZONE C's `places:` LINE, not
	//             by reference to Zone A's fixed list. A symbol published there is
	//             ALREADY a legal `where` by that line's own words.
	//    :1081-1083  "ZONE A PRINTS THE FULL FIXED VOCABULARY, THE GRAMMAR
	//           ENFORCES WHAT EXISTS THIS MATCH" - the fixed/per-match split this
	//           feature needs already exists and is already load-bearing.
	//    :1813  `Head += TEXT("places: ");` - the publication point.
	//    USiegeAssistantGrammar::Build(UnitKinds, PlaceNames, RegionPlaceNames)
	//           takes PlaceNames ⇒ appending here is GBNF-samplable with ZERO
	//           grammar-code change.
	//    SiegeAssistantVocabulary.cpp:223 - `hold` is ALREADY an alias of the
	//           `guard` intent, and `ambush` is already one of the seven intents
	//           ⇒ "move all units to hold 1" and "ambush 2" already parse in every
	//           part EXCEPT the place. ⛔ No new intent, ⛔ no eighth `who` shape.
	//
	//  ⛔ THE ONE SHAPE THAT WOULD COST REAL ZONE-A BYTES IS THE ONE v1 REFUSES
	//  (M-6): a mark is `where`-ONLY. RegionPlaceNames is NOT written below,
	//  because the `ZONE = ` line is generated from the FIXED TABLE's bHasRegion
	//  column (:1088-1107) and a per-match entry there would make Zone A VARY -
	//  destroying the byte-freeze and the KV prefix.
	// =========================================================================

	/**
	 *  THE WORLD Z A MARK'S SYMBOL RESOLVES AT.
	 *
	 *  `FSiegeMapMark` is 2D BY LAW (MARK-§5 pins `FVector2D WorldXY`, because the
	 *  war map is a 2D projection and the widget that places a mark has no Z), so
	 *  this file has to supply the third component from somewhere and the choice
	 *  is written down rather than left as a literal.
	 *
	 *  ⚖️ 0 IS THE ARENA'S WALK SURFACE - READ, NOT GUESSED. CONVENTIONS:131 pins
	 *  `SM_ArenaTerrain` as: "placed at (0,0,0) it reproduces the old ArenaGround
	 *  slab's walk surface at Z=0, so every existing actor transform stays valid."
	 *
	 *  ⛔ THIS IS NOT THE "PLAUSIBLE-LOOKING ORIGIN" ResolvePlace's failure comment
	 *  WARNS ABOUT, and the distinction matters. That warning is about an
	 *  UNRESOLVED place leaving a caller with a zero VECTOR it might march an army
	 *  to. This is a RESOLVED place whose X and Y are the player's own click; only
	 *  the Z is supplied, and it is supplied from a documented level fact.
	 *
	 *  ⚠️ THE CONSEQUENCE, WRITTEN BESIDE THE NUMBER (HIGH-§1): a mark drawn over a
	 *  hill resolves to the FLOOR beneath the hill, never the hill's surface. Unit
	 *  movement projects onto the navmesh, so the ORDER still lands; the
	 *  assistant's confirm-step decal (SiegeAssistantComponent's
	 *  ConfirmPositionDecal / ConfirmAttackDecal) is placed at this exact Z and
	 *  will sit under a hill face. That is a PREVIEW-COSMETICS degradation on hill
	 *  marks - declared here rather than discovered in a playtest.
	 *
	 *  ⛔ A DOWN-TRACE PER MARK PER SENTENCE WAS CONSIDERED AND REFUSED. It would
	 *  add world queries to Capture(), which AS-§21.4 already refused for regions
	 *  ("a fresh TActorIterator for regions would be a QA FAIL"), and it would make
	 *  the mark's meaning depend on when the sentence was typed. If hill-accurate
	 *  marks are ever wanted, the Z belongs ON FSiegeMapMark, written by the widget
	 *  that already traces the map - ⛔ never re-derived here.
	 */
	static constexpr float MarkPlaceGroundZ = 0.f;

	/**
	 *  ⭐⭐ THE MARK SEAM - AND IT IS A PURE STATIC ON PURPOSE, exactly as
	 *  `FSiegeMapMark::MakeSymbol` is.
	 *
	 *  ⚖️ THIS IS `WR-§6`'s UNFUNDED-MANDATE LESSON APPLIED AT AUTHORING TIME: the
	 *  publication rule is READABLE FROM ONE FUNCTION that needs ⛔ no world, ⛔ no
	 *  widget, ⛔ no subsystem and ⛔ no Capture() to assert. `Capture()` calls it
	 *  with the live arrays; the tests call it with local ones, and they exercise
	 *  the SAME code rather than a re-implementation of it.
	 *
	 *  ⛔⛔ THE AIRLOCK, AT ITS TIGHTEST POINT IN THE WHOLE FEATURE. Only
	 *  `MakeSymbol(Number)` reaches `InOutPlaceNames`, which is the ONE array
	 *  BuildZoneC prints. `WorldXY` goes to `InOutPlaceLocations`, which carries
	 *  the same GAME-SIDE-ONLY clause as every other resolved place (CONVENTIONS
	 *  §3) and is never serialised into any zone. `RadiusUU` is ⛔ NOT READ AT ALL -
	 *  a mark denotes a POINT to this object, and reading its radius would be the
	 *  first step toward the `{"in": circle_1}` shape M-6 refuses.
	 *
	 *  ⛔ ZERO HALF-EXTENTS ARE APPENDED, AND THE ARRAY IS KEPT PARALLEL RATHER
	 *  THAN SKIPPED (M-6). ResolvePlaceRegion asks `RegionPlaceNames.Contains`
	 *  FIRST, so a mark can never answer as a region; the zero extent is what keeps
	 *  the three arrays index-aligned, which is the invariant ResolvePlaceRegion's
	 *  IsValidIndex pair defends.
	 *
	 *  ⭐ MARKS ARE PUBLISHED IN ASCENDING `Number` ORDER, ⛔ NOT IN STORE ORDER.
	 *  M-1's lowest-free allocator means a store that has had a mark deleted holds
	 *  its array out of numeric order (add 1,2,3 - delete 2 - add ⇒ 1,3,2), and an
	 *  unstable `places:` line would make the same board emit different bytes on
	 *  different turns. Zone C is allowed to vary, but ⛔ not for a reason nobody
	 *  chose.
	 *
	 *  @param Marks                   the local player's marks, in store order. Numbers below FSiegeMapMark::FirstMarkNumber and duplicates are SKIPPED (see the body).
	 *  @param InOutPlaceNames         appended to. ⚠️ THE ONLY ONE OF THE THREE THAT IS EVER PRINTED.
	 *  @param InOutPlaceLocations     appended to, game-side only.
	 *  @param InOutPlaceHalfExtents   appended to with FVector2D::ZeroVector, to keep the three parallel.
	 *  @return how many marks were actually published - 0 on a de-synchronised input, which is REFUSED rather than repaired.
	 */
	static int32 AppendMarkPlaces(
		const TArray<FSiegeMapMark>& Marks,
		TArray<FName>& InOutPlaceNames,
		TArray<FVector>& InOutPlaceLocations,
		TArray<FVector2D>& InOutPlaceHalfExtents);

	/**
	 *  What the `hero` place symbol denotes this capture (GHOST-§ G-8).
	 *
	 *  ⭐ IT IS AN ENUM RATHER THAN A BOOL BECAUSE THERE ARE THREE OUTCOMES AND THE
	 *  THIRD IS THE ONE THAT MATTERS: `None` means the `hero` slot is NOT
	 *  published, so the symbol drops out of the vocabulary and the grammar cannot
	 *  even spell it. ⛔ That is the correct degradation and it is NOT a zero
	 *  vector - see ChooseHeroAnchorSource.
	 */
	enum class EHeroAnchorSource : uint8
	{
		/** Nothing anchors `hero` this capture ⇒ the symbol is not published at all. */
		None,
		/** The living hero's own location - the shipped behaviour, unchanged. */
		LivingHero,
		/** GHOST-§ G-8: the pawn the player drives while his hero is down. */
		Ghost
	};

	/**
	 *  ⭐ GHOST-§ G-8, AS A PURE DECISION FUNCTION - the whole rule, readable from
	 *  one place and assertable with ⛔ no world, ⛔ no pawn and ⛔ no Capture().
	 *  Same reasoning as AppendMarkPlaces above (`WR-§6`'s unfunded-mandate lesson).
	 *
	 *  ⚖️ WHY G-8 EXISTS AT ALL, in its author's terms: `follow` and `rally` are
	 *  HERO-RELATIVE intents (Zone A teaches "follow = they follow the hero",
	 *  "rally = hero rallies units near him"). Resolving `hero` to a hidden corpse
	 *  would SILENTLY WALK THE PLAYER'S ARMY TO WHERE HE DIED - a valid-shaped
	 *  wrong command, the exact failure class CONVENTIONS §1 exists to prevent.
	 *  ⭐ It costs ZERO extra prompt characters: same symbol, different resolution.
	 *
	 *  ⛔ THE GHOST BRANCH IS GATED ON THE HERO BEING DEAD, DELIBERATELY AND
	 *  NARROWLY. In normal play the controller possesses the hero itself, so
	 *  `bGhostAnchorAvailable` is false and this function returns exactly what
	 *  shipped before TASK-746. The branch can only fire in the one state
	 *  GHOST-§ G-8 describes.
	 *
	 *  ⛔ AND "NEITHER EXISTS" IS `None`, ⛔ NEVER A ZERO VECTOR THAT READS AS THE
	 *  MAP ORIGIN. The caller must leave the slot UNRESOLVED, which drops `hero`
	 *  from PlaceNames, which drops it from the grammar's `where` alternation -
	 *  the same fail-closed direction AS-§21.5 takes everywhere else: a place named
	 *  and not resolved becomes a REFUSAL, never an order to somewhere plausible.
	 *
	 *  @param bHeroExists            an AHeroCharacter for the ordering team was found at all
	 *  @param bHeroIsDead            ...and AHeroCharacter::IsDead() is true for it
	 *  @param bGhostAnchorAvailable  the ordering controller currently drives a pawn that is NOT that hero
	 */
	static EHeroAnchorSource ChooseHeroAnchorSource(bool bHeroExists, bool bHeroIsDead, bool bGhostAnchorAvailable);

private:

	/** Mid-zone ownership, resolved RELATIVE to the ordering team - the prompt never mentions Blue or Red (a colour is one more thing for a small model to get backwards). */
	enum class EMidOwner : uint8
	{
		/** No ACaptureZone in the world ⇒ prints `none`. */
		Absent,
		Neutral,
		Own,
		Enemy
	};

	/** Hero presence, resolved for the ordering team. */
	enum class EHeroPresence : uint8
	{
		/** No hero actor at all ⇒ prints `none`. */
		Absent,
		/** Present but AHeroCharacter::IsDead() ⇒ prints `down`. A dead hero is not a follow anchor (the shipped hero-death ruling). */
		Down,
		Alive
	};

	/** Wipes every field back to its empty-state default. Called first thing in Capture so a failed survey can never leave a half-filled snapshot behind. */
	void ResetSnapshot();

	/**
	 *  Appends the roster rows (already budget-trimmed by the caller) plus the
	 *  `other_kinds:` collapse line.
	 *
	 *  ⭐ THE COLLAPSE LINE NAMES THE KINDS IT HID, AND THAT IS THE ROOT-CAUSE FIX
	 *  FOR THE SORCERER DEFECT (TASK-517, CONVENTIONS AS-§20.2). Two shapes, both
	 *  always emitted (the fixed-key law - a missing key teaches the model that a
	 *  key is optional):
	 *
	 *      other_kinds: none
	 *      other_kinds: sorcerer, cleric (5 units)
	 *
	 *  Canonical symbols, comma-separated, in the SAME fixed DT_Cards row order the
	 *  rows above use, then the aggregate unit count.
	 *
	 *  ⛔ NEVER GO BACK TO COUNTS ONLY. `other_kinds: 5 kinds, 9 units` is what
	 *  shipped, and it is why a player who said "all units" never got Sorcerers:
	 *  the roster is elastic and the Sorcerer is the LAST DT_Cards row, so it was
	 *  always the first kind collapsed - and the collapsed line showed the model a
	 *  NUMBER where it needed a SYMBOL. Zone A refuses any unit "not a kind in
	 *  [FORCES]", so a name the prompt never printed is a unit the model refuses to
	 *  command even though it is alive on the board.
	 *
	 *  ✅ AND IT IS CHEAPER THAN WHAT IT REPLACES, so it cannot make the budget
	 *  worse: a collapsed symbol costs len(symbol) + 2 on this line, against the
	 *  36 + len(symbol) + digits a full roster row costs (43-49 on today's kinds).
	 *  The LINE is longer than the old one - measured +36 chars with 5 kinds
	 *  collapsed, +2 with one - but every char of that is bought by a row that is
	 *  no longer printed, and BuildZoneC's shrink loop re-measures the WHOLE block
	 *  after each step, so this line can never push the block past the budget.
	 *
	 *  ⚠️ ONE HONEST EXCEPTION, STATED SO IT IS NOT DISCOVERED: the shrink loop's
	 *  `KindsToPrint <= 0` floor breaks UNCONDITIONALLY, so a zero-row block is
	 *  emitted over budget if it ever comes to that - and this format makes that
	 *  overshoot bigger (all 13 symbols on one line is ~152 chars against the old
	 *  ~47). It is UNREACHABLE on today's board and the margin is not thin:
	 *  the smallest possible roster budget is ~202 chars (108-char head, and a tail
	 *  with BOTH player lines at MaxUtteranceBytes), while a one-kind block on a
	 *  13-kind board measures 182. A fourteenth kind narrows that too.
	 *
	 *  ⚠️ `(1 units)` IS DELIBERATE, NOT AN OVERSIGHT. The format is pinned by
	 *  CONVENTIONS AS-§20.2 as `(<N> units)`; a pluralisation branch would cost
	 *  characters out of a ~6-char headroom and make the line's bytes depend on the
	 *  board, and the model does not need the grammar lesson.
	 *
	 *  @param KindsToPrint  rows to print in full; clamped to [0, UnitKinds.Num()].
	 *                       Everything past it is named on the collapse line.
	 */
	void AppendRosterBlock(FString& Out, int32 KindsToPrint) const;

	/**
	 *  Flattens newlines/tabs/runs of whitespace and caps the line to
	 *  MaxUtteranceBytes - the prompt layout is line-oriented, so a pasted
	 *  paragraph must not be able to forge a key.
	 *
	 *  ⚠️ A CUT LINE IS MARKED IN THE PROMPT, AND THAT IS A DESIGN CALL, NOT AN
	 *  INCIDENT (TASK-433 BLOCKER-1). Stated with its reasoning so the next reader
	 *  can overturn it deliberately rather than by accident:
	 *
	 *  - WHY MARK AT ALL, WHEN A LOG WOULD SATISFY THE CONDITION'S LETTER. The
	 *    roster's collapse leaves `other_kinds:` in the prompt, so the model is
	 *    told something was hidden. A truncated `order:` line told it nothing, and
	 *    the sampler is grammar-constrained, so it produces a confident,
	 *    well-formed command from half a sentence. The log informs the DEVELOPER;
	 *    only the marker informs the MODEL, and the model is the one acting.
	 *  - WHY A VALUE AND NEVER A NEW KEY. A `truncated:` key would obey the
	 *    fixed-key law only by being emitted on EVERY turn, which adds its bytes to
	 *    every Zone C on every board - including all-ASCII ones - moving the
	 *    hand-verified Zone C figure and shrinking the roster budget for a case
	 *    that almost never fires. The marker rides on the VALUE of the `order:` /
	 *    `pending:` key, exactly as `mid: ours` and `mid: none` do, so the key set
	 *    is untouched.
	 *  - WHY IT COSTS NOTHING. The marker's bytes are budgeted INSIDE
	 *    MaxUtteranceBytes rather than added on top, so a line's maximum length is
	 *    unchanged and no downstream figure moves. On any line that fits, the
	 *    output is BYTE-IDENTICAL to the pre-TASK-433 sanitiser.
	 *  - WHY THIS WORDING. Zone A is byte-identical for the life of the process and
	 *    is another task's text, so the model CANNOT be taught what the marker
	 *    means. It is therefore plain editorial English - an ellipsis and a
	 *    bracketed word - which is what "this quotation was cut" looks like
	 *    everywhere in pretraining, rather than a symbol that would need teaching.
	 *  - WHY IT IS SAFE UNTAUGHT. TASK-417's GBNF constrains every emitted symbol
	 *    to the live kind/place alternations, so the model physically cannot echo
	 *    the marker into a command; and nothing parses Zone C (the executor and the
	 *    FSM read none of it), so it cannot break a reader. The worst case is that
	 *    the model ignores it, which is exactly today's behaviour.
	 *
	 *  @param In                 raw player- or FSM-authored line
	 *  @param OutFlattenedBytes  UTF-8 byte length of the FLATTENED line before the cap; > MaxUtteranceBytes means it was cut (this is the magnitude BuildZoneC escalates on)
	 */
	static FString SanitizeForPrompt(const FString& In, int32& OutFlattenedBytes);

	/**
	 *  Reports one truncated player-text line under the SAME escalating-latch
	 *  discipline the roster collapse uses: an unlatched Verbose record every turn,
	 *  plus a Warning on the first cut and on every DEEPER one. It lives here
	 *  rather than in SanitizeForPrompt because that function is static by design
	 *  (a pure text transform, no `this`), and the latches are member state.
	 *
	 *  @param LineKey         the prompt key whose value was cut - "order" or "pending"
	 *  @param FlattenedBytes  SanitizeForPrompt's OutFlattenedBytes for that line
	 *  @param WarnedBytes     the caller's per-line latch: the deepest cut already reported
	 */
	void ReportLineTruncation(const TCHAR* LineKey, int32 FlattenedBytes, int32& WarnedBytes) const;

	/** Aggregated rows, (Kind, GroupId) ⇒ Count. */
	UPROPERTY(Transient)
	TArray<FSiegeAssistantRosterEntry> Roster;

	/** Resolvable place symbols: the fixed vocabulary in table order, then the local player's map marks in ascending number order (TASK-746). Parallel to PlaceLocations and PlaceHalfExtents. */
	UPROPERTY(Transient)
	TArray<FName> PlaceNames;

	/** Resolved world locations, parallel to PlaceNames. ⚠️ GAME-SIDE ONLY - never serialized into any zone (CONVENTIONS §3). */
	UPROPERTY(Transient)
	TArray<FVector> PlaceLocations;

	/**
	 *  Zone half-extents (XY), parallel to PlaceNames / PlaceLocations. ZeroVector
	 *  for a place with no region primitive - which is why RegionPlaceNames, and
	 *  not "is this extent non-zero", is what ResolvePlaceRegion asks first.
	 *
	 *  ⚠️ GAME-SIDE ONLY, on the SAME clause as PlaceLocations (CONVENTIONS §3): an
	 *  extent is a distance, a distance is a number that means a position, and no
	 *  zone ever prints one. ⭐ THAT IS THE WHOLE REASON THIS FEATURE COSTS ZONE C
	 *  NOTHING - the model names a region SYMBOL, exactly as it already names a
	 *  destination symbol, and the geometry never leaves this object except through
	 *  ResolvePlaceRegion into the executor.
	 */
	UPROPERTY(Transient)
	TArray<FVector2D> PlaceHalfExtents;

	/**
	 *  The region-bearing subset of PlaceNames, in the same fixed vocabulary order
	 *  (AS-§21.4). Feeds the grammar's `zone` alternation and gates
	 *  ResolvePlaceRegion. ⚠️ Empty is a LEGAL state - a map with no capture zone
	 *  and no ancient ground - and it correctly makes the whole `{"in":ZONE}` shape
	 *  unsamplable rather than making it sampleable and unresolvable.
	 */
	UPROPERTY(Transient)
	TArray<FName> RegionPlaceNames;

	/** Canonical unit symbols, DT_Cards row order. Parallel to KindTotals / KindOrderable / KindFollowable. */
	UPROPERTY(Transient)
	TArray<FName> UnitKinds;

	/** Live count per kind. */
	UPROPERTY(Transient)
	TArray<int32> KindTotals;

	/** Per kind: how many may take a SELECTION-BEARING order (send/guard/ambush) - ASummonedUnit::IsGroupCommandEligible(), the SHIPPING predicate, never reimplemented. */
	UPROPERTY(Transient)
	TArray<int32> KindOrderable;

	/** Per kind: how many may FOLLOW - ASummonedUnit::IsFollowCommandEligible(). Deliberately a SECOND number: the Cleric-follows-but-cannot-hold split is already law, and one merged count would lie about one of the two verbs. */
	UPROPERTY(Transient)
	TArray<int32> KindFollowable;

	/** Ungrouped live units on the ordering team. */
	UPROPERTY(Transient)
	int32 StanceFree = 0;

	/** Live units in a Follow-type group order. */
	UPROPERTY(Transient)
	int32 StanceFollowing = 0;

	/** Live units in a Hold-type group order. */
	UPROPERTY(Transient)
	int32 StanceHolding = 0;

	/** Live units in an Ambush-type group order. */
	UPROPERTY(Transient)
	int32 StanceAmbushing = 0;

	/** Own castle HP quantized to 10% steps, or INDEX_NONE for "no standing own castle" ⇒ `none`. A standing castle never prints 0% (it clamps to 10) - 0 would read as destroyed. */
	UPROPERTY(Transient)
	int32 OwnCastleHPBand = INDEX_NONE;

	/** Enemy castle HP band, same contract. */
	UPROPERTY(Transient)
	int32 EnemyCastleHPBand = INDEX_NONE;

	/** Own gold FLOORED to 10s (floored, not rounded: a band must never overstate what the player can afford), or INDEX_NONE ⇒ `none`. */
	UPROPERTY(Transient)
	int32 GoldBand = INDEX_NONE;

	/** Mid-zone owner relative to the ordering team. */
	EMidOwner MidOwner = EMidOwner::Absent;

	/** Hero presence for the ordering team. */
	EHeroPresence HeroPresence = EHeroPresence::Absent;

	/**
	 *  ⚠️ TRUNCATION-VISIBILITY LATCH (TASK-419 WARN-5, closed here by TASK-413).
	 *
	 *  This pair REPLACES a single `bWarnedSnapshotTruncated` bool that made the
	 *  most common degradation completely INVISIBLE. The old warning fired on
	 *  `KindsToPrint < FMath::Min(UnitKinds.Num(), MaxRosterKinds)`, which is
	 *  FALSE at exactly the cap - so a 13-kind board collapsing five kinds into
	 *  `other_kinds:` logged nothing at all, on every sentence, forever. That is
	 *  the case that actually costs accuracy, because GetUnitKinds() is NEVER
	 *  truncated: the grammar still admits every collapsed kind, so the sampler
	 *  can emit a symbol the prompt never showed the model.
	 *
	 *  With SnapshotTrimBudgetChars binding at a real measured budget, truncation
	 *  gets MORE likely, and A TIGHTER CAP THAT SILENTLY DEGRADES THE PROMPT IS
	 *  WORSE THAN THE LOOSE ONE THAT DID NOT. So the cap correction and this
	 *  latch ship together; neither is complete alone.
	 *
	 *  ESCALATING, NOT ONE-SHOT: a Warning is emitted the first time the roster
	 *  fails to print in full, and again whenever the degradation gets WORSE than
	 *  anything already reported. A steady state still logs once - which is the
	 *  per-sentence spam the original latch existed to prevent - but a new and
	 *  deeper collapse can never hide behind an earlier, milder one.
	 *
	 *  `mutable` because the zone builders are const by the §9 signature pin.
	 */
	mutable int32 WarnedRosterKindsPrinted = MAX_int32;

	/** Companion to WarnedRosterKindsPrinted: the most kinds ever reported collapsed. See its comment. */
	mutable int32 WarnedRosterKindsCollapsed = 0;

	/**
	 *  ⚠️ PLAYER-TEXT TRUNCATION LATCHES (TASK-433 BLOCKER-1). The deepest cut
	 *  already reported for each of the two capped lines, in flattened UTF-8 bytes;
	 *  0 = nothing reported yet.
	 *
	 *  SEPARATE PER LINE, DELIBERATELY. `order:` carries what the player just
	 *  typed; `pending:` carries the FSM's own state across a clarification turn
	 *  (CONVENTIONS §1 - it is how context moves forward WITHOUT feeding the model
	 *  its own previous output). A deep cut on one must never be able to hide
	 *  behind an earlier, deeper cut on the other, because the two mean different
	 *  things and are fixed in different places.
	 *
	 *  ⚠️ THE PENDING LINE IS THE LATENT HALF. Today's authored pending lines run
	 *  ~72 bytes and cannot trip this; the first FSM task that makes them longer
	 *  will, and before TASK-433 it would have corrupted the FSM's own carried
	 *  state with no way for the FSM to learn it happened. That is exactly the
	 *  profile of a defect that first fires long after the code that caused it
	 *  shipped - which is why the latch exists BEFORE the line grows.
	 *
	 *  `mutable` for the same reason as the roster latches: the zone builders are
	 *  const by the §9 signature pin.
	 */
	mutable int32 WarnedUtteranceBytes = 0;

	/** Companion to WarnedUtteranceBytes, for the `pending:` line. See its comment. */
	mutable int32 WarnedPendingBytes = 0;

	/** One-shot log latch for a Zone B that outgrew ZoneBCharReserve (cannot happen with four fixed keys - it is a tripwire for a later key being added without raising the reserve). */
	mutable bool bWarnedZoneBOverReserve = false;

	/** One-shot log latch for an unresolvable /Game/Data/DT_Cards (the roster then falls back to lexical order, which is still fixed and still deterministic). */
	bool bWarnedMissingCardTable = false;
};

// ---------------------------------------------------------------------------
// THE NON-ORDERABLE-KIND GUARD (TASK-441)
// CONVENTIONS "Settings screen + the assistant CONFIRM STEP + the
// non-orderable-kind guard (2026-08-03)" §6 + §8 (pinned signature).
//
// M8 DECLARATION DUTY (stated verbatim as required): "adds no replicated
// property, no new replicated class, no new relevancy tier." Everything added
// by TASK-441 is a const read of a client-local survey object plus one pure free
// function over plain arrays; nothing here crosses the wire.
// ---------------------------------------------------------------------------

/**
 *  Why the guard rejected a command. PLAIN `enum class`, NOT a `UENUM` - pinned
 *  that way in the §8 registry and correct for the reason the widget-param law
 *  gives: this is a code-level reason token the FSM maps to a game-authored
 *  template, and it crosses to UMG as a `uint8` if it ever crosses at all.
 *
 *  ⛔ THREE VALUES, AND THE SET IS PART OF THE PIN. TASK-443 switches on it.
 *  Adding a value here without re-pinning it breaks that link.
 */
enum class ESiegeAssistantRejectReason : uint8
{
	/** Nothing to refuse - the only value that accompanies a `true` return. */
	None,

	/** The kind is on the board and NOT ONE of them may take this order (the DEV-04 shape: `sapper`, which the roster line itself prints as `orderable=0`). */
	KindNotOrderable,

	/** The kind is not on the board at all - no live unit of it exists on the ordering team. */
	KindUnknown
};

/**
 *  ⛔ THE MODEL-INDEPENDENT COMMAND-LAYER REFUSAL. Returns false when the
 *  command names a unit kind the live snapshot says cannot take it, BEFORE
 *  anything executes.
 *
 *  ── WHY THIS EXISTS, AND THE HONEST LIMIT FIRST ──
 *
 *  CONVENTIONS "In-match LLM command assistant" §1 says "grammar guarantees
 *  existence, EXECUTOR GUARANTEES LEGALITY, FSM owns the conversation", and §12f
 *  measured that the executor half was asserted in the document and ABSENT FROM
 *  THE CODE: `DEV-04` ("send the catapults at the enemy base") emitted a live,
 *  well-formed order for `sapper` - a kind the roster line prints as
 *  `orderable=0` - identically on all three runs. Orderability is per-match
 *  STATE, not identity; §1 forbids the grammar from encoding it, and four
 *  prompt-level attempts did not teach it. Only the command layer can refuse it.
 *
 *  ⛔ AND IT DOES NOT MAKE `DEV-04` PASS THE EVAL. THE EVAL SCORES EMITTED JSON,
 *  NOT EXECUTED ACTIONS. This guard changes what the game DOES; it changes
 *  nothing about what the model EMITS. It is SHIPPED SAFETY, not accuracy
 *  progress, and it is not a route to bar #5. Anything that files it under an
 *  accuracy heading is factually wrong.
 *
 *  ── THE SHAPE IS THE POINT ──
 *
 *  ⚠️ IT TAKES THE ROSTER ARRAY, NOT THE SNAPSHOT OBJECT - DELIBERATELY, for the
 *  same reason USiegeAssistantGrammar::Build takes TArray<FName>. A pure function
 *  over plain data needs NO UWorld, NO Capture() and NO engine state, so its
 *  automation tests run with no model resident, no PIE and no editor world.
 *  ⛔ A later "tidy-up" that changes it to take a `const USiegeAssistantSnapshot*`
 *  deletes that property and is REJECTED ON SIGHT.
 *
 *  ── EXACT CONTRACT ──
 *
 *  - `Command.Kinds` EMPTY ⇒ true. `who:"none"` and `who:"all"` both parse to an
 *    empty selection, so army-wide verbs (charge / fallback / rally) and
 *    "everything eligible" have NO KIND TO VALIDATE and are never refused here.
 *  - A named kind with NO live units on the ordering team ⇒ false,
 *    `KindUnknown`. Checked for EVERY intent, because a hallucinated unit is a
 *    hallucinated unit whatever verb carries it.
 *  - A named kind that is present but has ZERO orderable units ⇒ false,
 *    `KindNotOrderable` - ⚠️ AND ONLY FOR THE ZONE-ORDER VERBS (Send / Guard /
 *    Ambush). See the Follow scoping note below; it is the difference between a
 *    guard and a regression.
 *  - MULTI-KIND SELECTIONS ARE REFUSED AS A WHOLE. The first offending kind in
 *    `Kinds` order is named and the function returns immediately; the good kinds
 *    are NOT executed and are NOT silently dropped. ⚠️ Silently dropping one kind
 *    of a multi-kind order is precisely the valid-shaped-wrong-command failure
 *    this architecture exists to prevent - a player who asked for footmen AND a
 *    sorcerer and got only footmen was answered wrongly, confidently.
 *  - BOTH OUT-PARAMS ARE ALWAYS WRITTEN, on every path, including success
 *    (`None` / `NAME_None`). ⚠️ This is the OPPOSITE of ResolvePlace, which
 *    deliberately leaves its FVector untouched, and the two differ for a reason:
 *    an untouched FVector cannot become a plausible-looking origin an army
 *    marches to, whereas an untouched reason code CAN become a stale refusal
 *    reason from the previous sentence. Initialise the one; leave the other.
 *
 *  ── WHAT IT DELIBERATELY DOES **NOT** CHECK, STATED SO NOBODY READS MORE INTO IT ──
 *
 *  - ⚠️ `Follow` IS NOT GATED BY THE ORDERABLE COLUMN, AND GATING IT WOULD BREAK A
 *    SHIPPED COMMAND. `IsGroupCommandEligible()` covers the ZONE orders only:
 *    the Cleric follows and CANNOT take zone orders, so a Cleric row is
 *    `Count > 0, Orderable == 0`. Refusing `follow` on that column would refuse
 *    "clerics follow me" - which is a legal shipped order, and is verbatim the
 *    eval's own `DEV-20` utterance. The followable column exists on the snapshot
 *    (KindFollowable) but the pinned reason enum has no code for it and TASK-443
 *    has no template for it, so v1 does not refuse here. It is NOT an open hole:
 *    the executor's selector filters on `IsFollowCommandEligible()` and a
 *    non-followable kind arrives at the SHORTFALL path instead.
 *  - COUNTS ARE NOT CHECKED. "You asked for 10 and 8 exist" is the shortfall /
 *    clarification path and it belongs to the FSM. Clamping here would make that
 *    clarification undetectable, which is the defect CONVENTIONS §1 names when it
 *    says to constrain identity hard and leave quantity soft.
 *  - `Where` IS NOT CHECKED. Place existence is the grammar's job (the `where`
 *    alternation is generated from the live resolvable places) and ResolvePlace
 *    is the game-side airlock.
 *  - `TriggerKind` IS NOT CHECKED, AND THAT IS CORRECT RATHER THAN AN OVERSIGHT.
 *    A deferred intent means "fire once at least N of these EXIST", so a trigger
 *    kind that is absent RIGHT NOW is the whole point of the wait. Validating it
 *    as if it were a `who[]` entry would refuse every deferred order that was
 *    doing its job.
 *  - IT PRODUCES NO PLAYER-FACING STRING (CONVENTIONS §3). It returns a REASON
 *    CODE; TASK-443 routes it to the EXISTING `{"ask":"unsupported"}` outcome,
 *    which already has a game-authored template. ⛔ No new player-facing surface
 *    is invented here or downstream.
 *
 *  @param Command          the parsed command. Only `Intent` and `Kinds` are read.
 *  @param Roster           the live snapshot roster - USiegeAssistantSnapshot::GetRoster(), or a hand-populated array in a test.
 *  @param OutReason        always written; `None` on success.
 *  @param OutOffendingKind always written; `NAME_None` on success, otherwise the FIRST offending symbol in `Kinds` order.
 *  @return                 true when the command may proceed to execution.
 */
bool ValidateCommandAgainstSnapshot(const FSiegeAssistantCommand& Command,
                                    const TArray<FSiegeAssistantRosterEntry>& Roster,
                                    ESiegeAssistantRejectReason& OutReason,
                                    FName& OutOffendingKind);
