// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SiegeAssistantVocabulary.generated.h"

/**
 *  One canonical symbol and the words a player might use for it (batch
 *  LLM-ASSISTANT, TASK-417 — CONVENTIONS "In-match LLM command assistant" §9).
 *
 *  Aliases are free-form player language ("footmen", "the lads"); Canonical is
 *  the wire symbol the grammar will actually emit ("footman").
 */
USTRUCT(BlueprintType)
struct FSiegeAssistantSynonym
{
	GENERATED_BODY()

	/** The canonical lower snake_case symbol, e.g. "footman" / "enemy_castle" / "send". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Assistant")
	FName Canonical = NAME_None;

	/** Player-language words that mean Canonical. Case and ordering are normalised when the table is built. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Assistant")
	TArray<FString> Aliases;
};

/**
 *  The assistant's synonym vocabulary — the Zone-A block that teaches the model
 *  which player words map to which canonical symbols (batch LLM-ASSISTANT,
 *  TASK-417; CONVENTIONS "In-match LLM command assistant" §8, §9).
 *
 *  ⚠️ THE VOCABULARY FEEDS THE PROMPT, NEVER THE GRAMMAR. This is the line that
 *  keeps the design honest. The grammar's alternatives are canonical symbols
 *  drawn from live game state, so an alias can only ever help the model CHOOSE
 *  among symbols that already exist — it can never widen what is expressible.
 *  A future task that generates grammar alternatives from this asset has
 *  dissolved the grounding guarantee.
 *
 *  ⚠️ IT MUST DISAMBIGUATE SORCERER FROM WIZARD. They are different cards
 *  (Wizard: ranged fire caster, 24 gold. Sorcerer: ritualist that cannot attack
 *  and empowers friendlies standing on an ancient ground, 60 gold). A shared
 *  alias like "mage" or "caster" would make the assistant confidently command
 *  the wrong unit, which is worse than failing — so the ambiguous words are
 *  assigned to NEITHER and the notes block tells the model to ask instead.
 *  CONVENTIONS §11 makes this a mandatory row in the sealed evaluation corpus.
 *
 *  BuildSynonymTable's output must be BYTE-STABLE, because it is part of Zone A
 *  and Zone A being byte-identical for the life of the process is what lets
 *  llama_kv_cache_seq_rm keep the prefix and turn a ~500-token prefill into
 *  ~150 from turn two onward. Ordering is therefore normalised inside the
 *  builder rather than trusted to the order someone happened to type rows into
 *  the DataAsset.
 *
 *  The class ships with sane C++ defaults so the feature works before any asset
 *  exists; the /Game/Data/DA_AssistantVocabulary instance is TASK-421's
 *  deliverable and overrides them wholesale.
 *
 *  ⚖️ NET RELEVANCY TIER: adds no replicated property, no new replicated class,
 *  no new relevancy tier. (M8 DECLARATION DUTY, CONVENTIONS §8.)
 */
UCLASS(BlueprintType)
class GITCLAUDEUNREALTEST_API USiegeAssistantVocabulary : public UDataAsset
{
	GENERATED_BODY()

public:

	USiegeAssistantVocabulary();

	/** Player words for unit kinds. Canonical values are the lower snake_case CardIDs the snapshot reports. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant")
	TArray<FSiegeAssistantSynonym> UnitSynonyms;

	/** Player words for named places. Canonical values are the fixed place vocabulary (CONVENTIONS §8). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant")
	TArray<FSiegeAssistantSynonym> PlaceSynonyms;

	/** Player words for the seven verbs. Canonical values are the ESiegeAssistantIntent symbols. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Assistant")
	TArray<FSiegeAssistantSynonym> IntentSynonyms;

	/**
	 *  Renders the Zone-A synonym block.
	 *
	 *  DETERMINISTIC BY CONSTRUCTION, not by convention: canonical symbols are
	 *  lower-cased and sorted, each alias list is lower-cased, trimmed, de-duped
	 *  and sorted, and entries with no aliases are dropped (they teach the model
	 *  nothing the symbol lists have not already said, so they are pure cost).
	 *
	 *  ⚠️ DROPPING EMPTY ROWS IS A TIDINESS RULE, NOT A BUDGET RULE, AND THIS IS
	 *  NOT A LICENCE TO TRIM ZONE A. This block roughly DOUBLES Zone A (~2158
	 *  chars without it, ~4250 with the shipped defaults) and that is deliberate:
	 *  bar #3's KV-reuse ratio is (B+C)/(A+B+C), so a bigger Zone A makes the bar
	 *  PASS more comfortably, while cutting it toward the plan's original "~350
	 *  tok" estimate flips 78% to 68% and marginally FAILS. See the zone table on
	 *  USiegeAssistantSnapshot (CONVENTIONS §8; TASK-419 WARN-5).
	 *
	 *  Calling this twice on the same asset returns
	 *  byte-identical strings; two assets with the same rows in a different order
	 *  also return byte-identical strings.
	 *
	 *  The trailing notes block is authored in C++ rather than in the asset,
	 *  because it encodes disambiguation LAW (Sorcerer vs Wizard, guard vs
	 *  fallback) rather than content — it must survive an artist editing the
	 *  DataAsset.
	 *
	 *  @return the block, newline-terminated. Never empty: the notes always ship.
	 */
	FString BuildSynonymTable() const;
};
