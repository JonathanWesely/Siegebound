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
 *      ZONE A  ~600 tok  STATIC  system prompt, schema, place vocabulary,
 *                                synonym table, 3 few-shots
 *      ZONE B  ~120 tok  SLOW    castle HP bands, mid owner, gold band
 *      ZONE C  ~150 tok  FAST    roster, pending-intent line, the utterance
 *
 *  ⚠️ ZONE A IS ~600 tok AS BUILT, NOT THE "~350" THE ORIGINAL PLAN ESTIMATED,
 *  AND IT MUST NOT BE TRIMMED TOWARD 350. Measured: 2158 chars with
 *  `synonyms: none`; with DA_AssistantVocabulary loaded the synonym table roughly
 *  doubles it (~4250 chars, ~1180 tok). That is DELIBERATE and it makes the spike
 *  BETTER, not worse. Bar #3 is the KV-reuse drop, whose ratio is (B+C)/(A+B+C):
 *  at the shipped sizes 165/765 = a 78% drop and the bar PASSES, whereas cutting
 *  Zone A to the nominal 350 gives 165/515 = 68% and the bar MARGINALLY FAILS.
 *  Zone A is also prefilled once and cached, so its size costs turn-1 TTFT only
 *  and barely touches bar #2, which measures a warm prefix. Trimming here would
 *  optimise the wrong number and could turn a GO into a GO-WITH-RESCOPE.
 *  (CONVENTIONS §8; TASK-419 WARN-5. Real tokenizer counts are TASK-413's.)
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
	 *  ⚠️ FLAGGED TUNABLE (CONVENTIONS §10) - the ≤400-token snapshot cap
	 *  enforced as characters at a conservative 3.6 chars/token, until
	 *  TASK-413's spike supplies the real tokenizer count and this constant is
	 *  CORRECTED FROM THE MEASUREMENT (never guessed a second time).
	 *
	 *  ⚠️ THE CAP COVERS ZONE B + ZONE C - the live snapshot - NOT Zone A.
	 *  Zone A is the static preamble the three-zone budget accounts for
	 *  separately, and the board's own clause says over-cap truncation "must
	 *  never truncate Zone A or the utterance". A cap covering A would leave
	 *  B+C ~50 tokens, which is not a readable snapshot; so the cap is on what
	 *  Capture() produced.
	 *
	 *  ⚠️ Zone A measures ~600 tok as built (~1180 with the vocabulary asset),
	 *  NOT the "~350" the original plan estimated - see the zone table on the
	 *  class above for why growing it PASSES the KV-reuse bar and trimming it
	 *  toward 350 marginally FAILS it. Do not read this constant as licence to
	 *  shrink Zone A (TASK-419 WARN-5).
	 */
	static constexpr int32 MaxSnapshotChars = 1440;

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
	 *  budget fits - and logs ONCE. The utterance and the pending line are never
	 *  truncated by the budget (only by MaxUtteranceChars, which is a separate,
	 *  always-on sanitiser).
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

	/** One-shot log latch for the character-budget truncation - `mutable` because the builders are const by the §9 pin, and a per-sentence warning would spam. */
	mutable bool bWarnedSnapshotTruncated = false;

	/** One-shot log latch for a Zone B that outgrew ZoneBCharReserve (cannot happen with four fixed keys - it is a tripwire for a later key being added without raising the reserve). */
	mutable bool bWarnedZoneBOverReserve = false;

	/** One-shot log latch for an unresolvable /Game/Data/DT_Cards (the roster then falls back to lexical order, which is still fixed and still deterministic). */
	bool bWarnedMissingCardTable = false;
};
