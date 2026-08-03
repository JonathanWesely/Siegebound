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

	/**
	 *  THE PER-LINE CAP ON PLAYER-CONTROLLED TEXT, COUNTED IN UTF-8 BYTES.
	 *
	 *  ⚠️ RENAMED FROM `MaxUtteranceChars` BY TASK-433 (BLOCKER-2), AND THE RENAME
	 *  IS THE FIX, NOT COSMETICS - a constant named "Chars" that measures bytes
	 *  would be a fresh instance of the very defect this closes. The old name
	 *  counted FString CODE UNITS (UTF-16 on Windows) while the budget it proxies
	 *  for is TOKENS OVER UTF-8, and the 2.71 chars/token ratio behind
	 *  MaxSnapshotChars was calibrated on ASCII, where one char is exactly one
	 *  byte. Outside ASCII the two diverge and the cap silently OVER-ADMITS: 240
	 *  code units of emoji (surrogate pairs - 2 units and 4 bytes each) is ~120
	 *  glyphs and 360-480 tokens against a 400-token B+C budget, while every
	 *  character-based check reports 240 of 1085 and looks healthy.
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
