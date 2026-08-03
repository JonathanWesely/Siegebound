// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
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
 *  legally NAME (CONVENTIONS §8): unit KINDS with counts, a FIXED named-place
 *  vocabulary, and quantized match facts. There are no individually-addressable
 *  units in v1 - nobody says "Footman #7" - and there are no coordinates
 *  anywhere in any zone (§3): every resolved FVector stays GAME-SIDE behind
 *  ResolvePlace(). That single rule deletes the hallucinated-number failure
 *  class outright, because the model has no number in front of it that means a
 *  position.
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
	 *  THE ≤400-TOKEN SNAPSHOT CAP, ENFORCED AS CHARACTERS.
	 *
	 *  ⚠️ CORRECTED FROM MEASUREMENT ON 2026-08-03 BY TASK-413's SPIKE RUN:
	 *  1440 → 1085. This is the ONE-PASS correction CONVENTIONS §10 flagged and
	 *  §8's RESOLUTION trigger armed; the trigger has now FIRED. The number is no
	 *  longer a guess and is not to be guessed a second time.
	 *
	 *      1085 = 400 tokens × 2.71 chars/token
	 *
	 *  2.71 is the MEASURED chars/token of ZONE B + ZONE C — the region this cap
	 *  actually governs — taken from the shipping tokenizer with the model
	 *  resident (`Siege.Llama.SpikePrompt`, fixture t0: 955 chars / 352 tokens).
	 *  BOTH OPERANDS ARE MEASUREMENTS.
	 *
	 *  ⚠️ WHY 1440 WAS NOT MERELY IMPRECISE BUT WRONG IN THE UNSAFE DIRECTION.
	 *  It was derived as 400 × 3.6, and §8 called 3.6 "conservative". Every
	 *  measured reading is BELOW it — Zone B 2.13, Zone C 2.77, B+C 2.71, whole
	 *  assembled prompt 3.56 — so 3.6 was OPTIMISTIC on every single one. At the
	 *  real 2.71 a 1440-char snapshot admits 1440 / 2.71 ≈ 531 tokens against a
	 *  400-token budget: 33 % OVER. The live board did not breach it (955 chars,
	 *  352 tok), so the breach was LATENT — the cap simply permitted a snapshot a
	 *  third over budget the moment the roster or the order line grew.
	 *
	 *  ⚠️ 2.13 IS THE STRICTER BOUND AND IS DELIBERATELY NOT THE ONE USED. Zone
	 *  B measures 2.13 chars/token, which would give 400 × 2.13 ≈ 850 chars. Zone
	 *  B is 68 characters of dense key/value symbols and digits and tokenizes far
	 *  worse per character than Zone C's roster lines, so applying its ratio to
	 *  the whole region would over-tighten by ~22 %. IF ZONE C EVER BECOMES AS
	 *  SYMBOL-DENSE AS ZONE B, ~850 IS THE NUMBER TO MOVE TO — and the way to
	 *  move it is to re-run `Siege.Llama.SpikePrompt` and read the ratio, never
	 *  to re-guess.
	 *
	 *  ⚠️ TASK-423 SUPERSEDES THIS CONSTANT AND THE WHOLE QUESTION. It enforces
	 *  the ≤400-token budget against THE TOKENIZER DIRECTLY, at which point this
	 *  cap demotes to a cheap pre-filter that stops a pathological snapshot ever
	 *  reaching the tokenizer. Do not implement that here.
	 *
	 *  ⚠️ THE CAP COVERS ZONE B + ZONE C - the live snapshot - NOT Zone A.
	 *  Zone A is the static preamble the three-zone budget accounts for
	 *  separately, and the board's own clause says over-cap truncation "must
	 *  never truncate Zone A or the utterance". A cap covering A would leave
	 *  B+C ~50 tokens, which is not a readable snapshot; so the cap is on what
	 *  Capture() produced.
	 *
	 *  ⚠️ Zone A measures 4,314 chars / 1,139 tok as built with the vocabulary
	 *  asset attached (TASK-413, replacing the "~600 tok" estimate) - see the
	 *  zone table on the class above for why growing it PASSES the KV-reuse bar
	 *  and trimming it toward 350 marginally FAILS it. Do not read this constant
	 *  as licence to shrink Zone A (TASK-419 WARN-5).
	 *
	 *  ⚠️ THIS CAP IS NOW COUPLED TO MaxRosterKinds, WHICH IT WAS NOT AT 1440.
	 *  Zone C is budgeted at MaxSnapshotChars - ZoneBCharReserve = 1085 - 192 =
	 *  893 chars. At the shipped MaxRosterKinds = 8 the roster prints ~220 chars
	 *  shorter than the spike's 13-kind fixture (887 chars), so a realistic board
	 *  clears the budget with ~220 to spare. RAISE MaxRosterKinds TOWARD 13 AND
	 *  THE CHARACTER BUDGET STARTS BITING IMMEDIATELY (887 against 893). That is
	 *  correct behaviour against a real budget rather than a defect - and it is
	 *  exactly why BuildZoneC's truncation now logs every time it degrades.
	 *  ⛔ DO NOT RAISE THIS CAP TO BUY THAT ROOM: it would restore the 33 %
	 *  over-admission this correction exists to remove. The honest lever is
	 *  ZoneBCharReserve, which over-charges a Zone B that measures 68 chars by
	 *  124 - re-measure it, do not eyeball it.
	 */
	static constexpr int32 MaxSnapshotChars = 1085;

	/**
	 *  Zone B's guaranteed slice of MaxSnapshotChars. Zone B is four short fixed
	 *  keys and never approaches this; the reserve exists so BuildZoneC can size
	 *  its roster budget WITHOUT calling BuildZoneB (which would double the work
	 *  and couple the two builders). BuildZoneB logs once if it ever exceeds it.
	 */
	static constexpr int32 ZoneBCharReserve = 192;

	/**
	 *  Roster kinds printed in full before the tail collapses into one
	 *  `other_kinds:` line. Over budget ⇒ aggregate harder, never spill: the
	 *  deck holds far more kinds than any one match fields, and eight covers a
	 *  realistic mid-match board.
	 */
	static constexpr int32 MaxRosterKinds = 8;

	/** Player text is capped and flattened before it reaches the prompt (a pasted paragraph must not become the whole context). */
	static constexpr int32 MaxUtteranceChars = 240;

	/**
	 *  SURVEYS THE WORLD FOR ONE TYPED SENTENCE. Six logical passes - units,
	 *  castles, gold nodes, ancient grounds, the capture zone, and the hero -
	 *  run ONCE PER SENTENCE, NEVER PER TICK.
	 *
	 *  Four of those passes are SHIPPED STATIC FINDERS reused verbatim rather
	 *  than re-derived (CONVENTIONS §4 + the FindBestMineFor idiom):
	 *  ACastle::FindNearestCastleForTeam, AGoldNode::FindBestMineFor and
	 *  AAncientGround::FindNearestAncientGround. Two of them are called TWICE
	 *  (own/enemy castle, near/far ground), which is why the honest traversal
	 *  count is higher than six - and is still unmeasurable next to one
	 *  inference call. Re-deriving their loops here to save a traversal would
	 *  trade a shipped, QA'd selection rule for a second copy that drifts.
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
	 *  for the life of the process and lets llama_kv_cache_seq_rm keep the
	 *  prefix. It is a member (not a static) only because the §9 registry pins
	 *  it as one. Making it state-dependent is a QA FAIL.
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
	 *  budget fits. The utterance and the pending line are never truncated by the
	 *  budget (only by MaxUtteranceChars, which is a separate, always-on
	 *  sanitiser).
	 *
	 *  ⚠️ EVERY COLLAPSE IS LOGGED, INCLUDING THE ONE AT EXACTLY THE CAP. Both
	 *  causes are reported and named - the MaxRosterKinds cap and the character
	 *  budget - at Warning on first occurrence and on every escalation, plus a
	 *  per-turn record at Verbose. It used to log ONCE and only for the budget
	 *  case, which meant a >MaxRosterKinds board degraded the prompt silently
	 *  forever (TASK-419 WARN-5). See WarnedRosterKindsPrinted.
	 *
	 *  @param Utterance   the player's raw typed sentence; flattened + capped, never interpreted here
	 *  @param PendingLine the FSM's GAME-AUTHORED pending-intent line (§1: each clarification turn is a fresh single-turn call, and this line - never the model's own previous output - is what carries context forward). Empty ⇒ `none`
	 */
	FString BuildZoneC(const FString& Utterance, const FString& PendingLine) const;

	/** Aggregated roster rows, sorted by fixed DT_Cards row order then GroupId. Empty until Capture() runs. */
	const TArray<FSiegeAssistantRosterEntry>& GetRoster() const { return Roster; }

	/**
	 *  The place symbols that ACTUALLY RESOLVE this match, in fixed vocabulary
	 *  order. This is what TASK-417's grammar generates its `where` alternation
	 *  from - so a place that does not exist on this map cannot be emitted at
	 *  all. Zone A still prints the FULL vocabulary (it must stay static); the
	 *  grammar is what enforces existence.
	 */
	const TArray<FName>& GetPlaceNames() const { return PlaceNames; }

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

	/** Appends the roster rows (already budget-trimmed by the caller) plus the `other_kinds:` collapse line. */
	void AppendRosterBlock(FString& Out, int32 KindsToPrint) const;

	/** Flattens newlines/tabs/runs of whitespace and caps to MaxUtteranceChars - the prompt layout is line-oriented, so a pasted paragraph must not be able to forge a key. */
	static FString SanitizeForPrompt(const FString& In);

	/** Aggregated rows, (Kind, GroupId) ⇒ Count. */
	UPROPERTY(Transient)
	TArray<FSiegeAssistantRosterEntry> Roster;

	/** Resolvable place symbols, fixed vocabulary order. Parallel to PlaceLocations. */
	UPROPERTY(Transient)
	TArray<FName> PlaceNames;

	/** Resolved world locations, parallel to PlaceNames. ⚠️ GAME-SIDE ONLY - never serialized into any zone (CONVENTIONS §3). */
	UPROPERTY(Transient)
	TArray<FVector> PlaceLocations;

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
	 *  With MaxSnapshotChars now binding at a real measured budget, truncation
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

	/** One-shot log latch for a Zone B that outgrew ZoneBCharReserve (cannot happen with four fixed keys - it is a tripwire for a later key being added without raising the reserve). */
	mutable bool bWarnedZoneBOverReserve = false;

	/** One-shot log latch for an unresolvable /Game/Data/DT_Cards (the roster then falls back to lexical order, which is still fixed and still deterministic). */
	bool bWarnedMissingCardTable = false;
};
