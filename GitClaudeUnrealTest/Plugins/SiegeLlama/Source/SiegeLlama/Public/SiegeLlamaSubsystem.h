#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

// LogSiegeLlama is DECLARED in SiegeLlamaLog.h and DEFINED ONCE in
// SiegeLlamaModule.cpp.
//
// CONVENTIONS section 5 pins the category to "SiegeLlamaSubsystem.h/.cpp", but
// TASK-409 needed it before this file existed and left a standing instruction in
// SiegeLlamaLog.h: include the header here, NEVER re-declare or re-define the
// category. A second DECLARE_LOG_CATEGORY_EXTERN / DEFINE_LOG_CATEGORY is a
// duplicate-symbol LINK error, not a compile error, so it surfaces late and
// confusingly. Including the header satisfies section 5 transitively: anyone who
// includes SiegeLlamaSubsystem.h gets the category.
#include "SiegeLlamaLog.h"

#include "SiegeLlamaSubsystem.generated.h"

class FSiegeLlamaWorker;

/**
 *  Completion callback. PINNED CHARACTER-FOR-CHARACTER by CONVENTIONS section 9;
 *  Wave 1's game-lane FSM compiles against this exact shape.
 *
 *  ALWAYS fires on the GAME THREAD, on every path -- success, model failure,
 *  budget rejection, hard timeout, cancellation and GGML fault alike.
 */
DECLARE_DELEGATE_ThreeParams(FSiegeLlamaCompletionSignature, bool /*bSuccess*/, const FString& /*Output*/, const FString& /*Error*/);

/**
 *  SOFT TIMEOUT (SoftTimeoutSeconds, 4.0). UX ONLY -- NOTHING IS ABORTED and the
 *  request runs on to the hard ceiling. Broadcast at most ONCE per request, on
 *  the GAME THREAD, and only for requests that actually pass 4 s.
 *
 *  It exists because CONVENTIONS section 10's soft timeout is specified as
 *  "tells the FSM to say something", and a constant the FSM has to poll for is
 *  not a signal. No parameters, so it carries no lane-crossing data at all.
 */
DECLARE_MULTICAST_DELEGATE(FSiegeLlamaSoftTimeoutSignature);

/**
 *  Which offload tier the model was actually loaded on. Chosen at load time from
 *  the MEASURED free VRAM of one PINNED device -- never assumed, never "all
 *  available devices". See the tier table in SiegeLlamaSubsystem.cpp.
 */
enum class ESiegeLlamaOffloadTier : uint8
{
	/** No model resident yet, or the load failed. */
	None,
	/** n_gpu_layers = 0. MEASURED to abort 2 of 5 generations at the 10 s ceiling (TASK-413 bar #2). */
	CpuOnly,
	/** n_gpu_layers = 18 of 36. The tier the frame-time bar is written against, and the default. */
	Partial,
	/** n_gpu_layers = -1 (all). MEASURED 2702.8 MiB on a 7891 MiB card, 417 ms TTFT, zero aborts. */
	FullOffload,
};

/**
 *  THE LOCAL INFERENCE SUBSYSTEM. One model, one context, one thread, one
 *  request at a time, no conversation state.
 *
 *  ============================ THE FIVE INVARIANTS ==========================
 *
 *  1. THE PLUGIN KNOWS NOTHING ABOUT SIEGEBOUND (CONVENTIONS section 8).
 *     No game-lane include, no Siegebound type in any signature. Everything that
 *     crosses the lane boundary is an opaque FString. This class cannot name a
 *     unit kind, a place, an intent or a snapshot, and must never learn to.
 *
 *  2. ONE llama_context, TOUCHED BY EXACTLY ONE THREAD. llama_context is not
 *     thread-safe. A long-lived FRunnable at TPri_BelowNormal owns it for the
 *     process lifetime, which makes single-thread ownership STRUCTURAL rather
 *     than a comment somebody can violate by accident. Not the task graph, not
 *     Async(), not a context per request.
 *
 *  3. QUEUE DEPTH 1, ENFORCED BY EARLY RETURN AND NOT BY A QUEUE.
 *     RequestCompletion returns false immediately when the subsystem is not
 *     ready or is already busy. There is deliberately NO request queue: a queue
 *     reintroduces the out-of-order / stale-context bug class this design
 *     deletes. Do not add one "for robustness".
 *
 *  4. NO MULTI-TURN LOOP AND NO CONVERSATION STATE (CONVENTIONS section 1).
 *     One utterance + one snapshot -> one constrained JSON. This class holds the
 *     PREVIOUS PROMPT'S TOKENS -- that is a KV cache key, not a conversation --
 *     and never feeds a model's own output back to the model.
 *
 *  5. A FAILURE IS NEVER A CRASH AND NEVER BLOCKS MATCH START.
 *     The model loads ASYNCHRONOUSLY on the worker; Initialize() returns
 *     immediately. A missing .gguf, a failed load, a GGML fault or a clamped
 *     context all latch bAssistantFaulted, log once on LogSiegeLlama, leave
 *     IsReady() false forever after, and leave every keyboard command working.
 *
 *  It is a UGameInstanceSubsystem for one concrete reason: it SURVIVES LEVEL
 *  TRAVEL, so "Play Again" never reloads 2.5 GB (the USiegeSessionSubsystem
 *  precedent).
 *
 *  M8 DECLARATION DUTY: adds no replicated property, no new replicated class,
 *  no new relevancy tier.
 */
UCLASS()
class SIEGELLAMA_API USiegeLlamaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// =====================================================================
	// TUNABLES (CONVENTIONS section 10). ALL FLAGGED FOR JONATHAN'S FEEL PASS.
	//
	// They are named constants rather than EditDefaultsOnly UPROPERTYs
	// deliberately: a UGameInstanceSubsystem has no editable asset, so an
	// EditDefaultsOnly property here would be invisible in the editor and would
	// merely LOOK tunable. Section 10 permits either form; the game lane reads
	// these symbols directly so there is exactly one copy of every number.
	// =====================================================================

	/**
	 *  UX ONLY. The request KEEPS RUNNING; this is the point at which the FSM
	 *  should say something rather than sit silent. It aborts nothing.
	 */
	static constexpr double SoftTimeoutSeconds = 4.0;

	/**
	 *  THE CEILING -- AND WHICH REGIME IT IS A PROMISE IN IS PART OF THE NUMBER.
	 *
	 *  llama.h:382-385, verbatim: "Abort callback / if it returns true, execution
	 *  of llama_decode() will be aborted / currently works only with CPU
	 *  execution."
	 *
	 *  So on the CpuOnly tier this deadline is AUTHORITATIVE: the callback is
	 *  polled inside llama_decode and cuts the run mid-graph.
	 *
	 *  ON Partial AND FullOffload IT IS A BACKSTOP -- AND WHAT MAKES IT ONE IS THE
	 *  LOOP, NOT THE CALLBACK. Because abort_callback is documented CPU-only, on
	 *  those tiers it is not a deadline mechanism at all. The enforcement there is
	 *  the explicit deadline test the worker runs BETWEEN PREFILL SLICES and AFTER
	 *  EVERY DECODED TOKEN (SiegeLlamaSubsystem.cpp, RunRequestUnguarded). That
	 *  test reads a plain clock on the worker's own thread, so it is entirely
	 *  tier-independent: the ceiling binds on cpu, partial and full alike.
	 *
	 *  THE GRANULARITY IS PART OF THE PROMISE AND IT IS NOT ZERO. The run is cut
	 *  at the first slice boundary strictly AFTER the deadline, so the overshoot
	 *  is bounded by the ONE llama_decode call that was in flight when it passed
	 *  -- at most llama_n_batch(ctx) prompt tokens during prefill (512 requested),
	 *  exactly ONE token during decode. What is NOT claimed on the GPU tiers is a
	 *  mid-graph cut; what IS claimed is that the run stops, on every tier, within
	 *  one slice of the ceiling.
	 *
	 *  AND THE CLOCK STARTS ON THE WORKER, NOT AT THE KEYPRESS. It is armed from
	 *  the top of RunRequestUnguarded, so the wake and handoff latency ahead of it
	 *  is outside the ceiling. That interval is a wake on an already-triggered
	 *  FEvent, so it is small -- but this is a bound on INFERENCE time, not on
	 *  what the player experiences end to end, and the two are not the same claim.
	 *
	 *  CORRECTED 2026-08-03 (qa/TASK-424.md WARN-2): until that date the prefill's
	 *  between-slice test omitted the deadline entirely, so on the offload tiers
	 *  this paragraph promised a backstop the code did not implement. The CODE was
	 *  changed to match this claim, not the claim to match the code -- a truncated
	 *  or runaway command is unusable rather than merely slow.
	 */
	static constexpr double HardTimeoutSeconds = 10.0;

	/**
	 *  THE BOUND ON A RUN'S LENGTH, and on the GPU tiers it is still the first
	 *  bound a normal run meets. The deadline above bounds a run's DURATION and
	 *  now does so on every tier; this bounds how many tokens it may emit. The two
	 *  are different axes and neither is a substitute for the other.
	 */
	static constexpr int32 MaxOutputTokens = 96;

	/**
	 *  THE AUTHORITY on the Zone B + Zone C budget, enforced BY TOKENIZING
	 *  (CONVENTIONS section 8 resolution 2). This subsystem has llama_tokenize
	 *  resident, so it counts tokens instead of estimating them from characters.
	 *  A subsystem that ships enforcing only a character cap has not implemented
	 *  that clause.
	 */
	static constexpr int32 MaxSnapshotTokens = 400;

	/**
	 *  THE PRE-FILTER, AND IT IS A SEPARATE CONSTANT FROM THE RETIRED AUTHORITY
	 *  VALUE ON PURPOSE (CONVENTIONS section 8, instance 4 of the unchecked-carry
	 *  class). Duties: CHEAP, FINITE, NEVER BINDING IN NORMAL PLAY.
	 *
	 *  SAFE DIRECTION INVERTS WITH THE ROLE. An AUTHORITY is safe when it assumes
	 *  the SMALLEST plausible chars/token ratio; a PRE-FILTER must never reject
	 *  what the authority would accept, so it must assume the LARGEST. The old
	 *  game-lane MaxSnapshotChars = 1085 is an authority value and CARRYING IT
	 *  INTO THIS ROLE UNDER ANY NAME IS A QA FAIL.
	 *
	 *  Sizing, from already-printed operands: hard floor 1424 = 400 x 3.56, the
	 *  measured WHOLE-PROMPT ratio and the highest ever recorded on this project.
	 *  Ruled value 3000 = 400 x 7.5, about twice the highest measured ratio
	 *  (3.79, Zone A), 2.1x the floor and 3.1x as-built B+C (955 chars). Erring
	 *  high costs one wasted tokenize on pathological input; erring low silently
	 *  truncates real content. Raising it later needs no argument. Lowering it
	 *  toward the floor is not an optimisation.
	 */
	static constexpr int32 SnapshotPreFilterMaxChars = 3000;

	/**
	 *  REQUESTED context size. FROZEN AT 2048 -- bars #1, #3 and #4 all PASSED at
	 *  this value and n_ctx drives both KV-cache size and prefill cost, so moving
	 *  it silently invalidates three passing measurements. It is not a tuner's
	 *  lever; it moves on a measurement, on Jonathan's call.
	 *
	 *  IT IS A REQUEST. llama.h:551-556 says the context is entitled to use a
	 *  different one. Everything that sizes work reads llama_n_ctx(ctx) instead.
	 */
	static constexpr int32 ContextTokens = 2048;

	/**
	 *  The startup budget assertion's margin. NAMED AND NON-ZERO by law.
	 *
	 *  DERIVED FROM TWO MEASURED BANDS, NOT EYEBALLED:
	 *    - the chat-template wrapper measured at ~13 tokens (turn1_prompt 1517 vs
	 *      assembled_total 1504), of which ~8 sit AHEAD of Zone A;
	 *    - CONVENTIONS section 12g's measured tokenisation error band, -15 to +4
	 *      with UNPREDICTABLE SIGN, whose own ruling is that anything with under
	 *      ~20 tokens of derived headroom is UNDECIDED until measured.
	 *  13 + 20 = 33 is the floor-of-floors; 48 clears it by ~1.45x.
	 *
	 *  IT IS A FLOOR, NOT A VALUE: setting it exactly to the measured 13 would
	 *  satisfy the arithmetic and defeat the purpose -- a margin's whole job is
	 *  the case that was not measured. Both bands are TEMPLATE- AND
	 *  MODEL-SPECIFIC and are RE-MEASURED, never carried, if either changes.
	 *
	 *  Sanity, on measured operands: 1139 + 400 + 96 + 48 = 1683 of 2048, and at
	 *  section 12c's zoneA_tok ceiling of 1389 it is 1933 of 2048. This margin
	 *  does not invalidate that ceiling.
	 */
	static constexpr int32 SafetyMarginTokens = 48;

	/**
	 *  Physical batch size. SMALL ON PURPOSE: n_ubatch is the primary frame-time
	 *  mitigation named by the plan's risk list -- it is what bounds how long one
	 *  graph submission occupies the device, and therefore how coarse the abort
	 *  and the hitch both are on the GPU tiers.
	 */
	static constexpr int32 UBatchTokens = 64;

	/** Logical batch ceiling. The prefill is submitted in slices of at most llama_n_batch(ctx). */
	static constexpr int32 BatchTokens = 512;

	// =====================================================================
	// USubsystem
	// =====================================================================
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// =====================================================================
	// THE PINNED API (CONVENTIONS section 9 -- character-for-character,
	// access levels included). TASK-443 compiles against exactly this.
	// =====================================================================

	/** Model loaded, worker alive, not faulted. */
	bool IsReady() const;

	/** A request is in flight (queue depth 1). */
	bool IsBusy() const;

	/**
	 *  Returns false IMMEDIATELY when !IsReady() or IsBusy(). OnComplete ALWAYS
	 *  fires on the GAME THREAD.
	 *
	 *  A false return means NOTHING WAS STARTED and OnComplete will NEVER fire --
	 *  the caller owns that path. A true return means OnComplete fires exactly
	 *  once, later, on the game thread, whatever happens.
	 *
	 *  Must be called from the game thread.
	 */
	bool RequestCompletion(const FString& Prompt, const FString& Grammar, const FSiegeLlamaCompletionSignature& OnComplete);

	/**
	 *  Routes through llama's abort_callback, which is the ONLY correct
	 *  mechanism: polling a flag after generation is far too late during a long
	 *  prefill, and the prefill is where the time is.
	 *
	 *  REGIME HONESTY, because the same call means two different things:
	 *    - CpuOnly       : the callback is polled INSIDE llama_decode, so the
	 *                      abort lands mid-graph, genuinely mid-prefill.
	 *    - Partial/Full  : abort_callback is documented CPU-only, so it is not
	 *                      what lands the cancel here. The worker's own
	 *                      between-slice and per-token tests are, and they bound
	 *                      the wait to the ONE llama_decode call in flight -- at
	 *                      most llama_n_batch(ctx) prompt tokens during prefill,
	 *                      exactly one token during decode. Bounded, but NOT
	 *                      mid-graph.
	 *
	 *  CORRECTED 2026-08-03: this read "at most one ubatch of prefill away".
	 *  UBatchTokens = 64 is the size of the graphs llama builds INTERNALLY inside
	 *  one llama_decode call; the worker never returns between them, so it cannot
	 *  bound anything at that granularity. The slice -- llama_n_batch(ctx), 512
	 *  requested -- is the boundary this code actually observes, which is 8x
	 *  coarser than the figure previously stated here and in
	 *  handoffs/TASK-450-programmer.md. The guarantee (bounded, not unbounded) is
	 *  unchanged; only the number was wrong, and it was wrong in the flattering
	 *  direction.
	 *
	 *  Idempotent, safe to call when idle, safe to call from the game thread only.
	 */
	void CancelActiveRequest();

	// =====================================================================
	// DECLARED DEVIATION -- read SiegeLlamaSubsystem.cpp's "THE STATIC PREFIX"
	// block before ruling on it. It takes an opaque FString and returns a bool;
	// it names no Siegebound type and needs no game-lane include.
	// =====================================================================

	/**
	 *  Registers the process-lifetime STATIC PROMPT PREFIX (the game lane's Zone
	 *  A) as an opaque string.
	 *
	 *  WHY IT EXISTS: CONVENTIONS section 8's ZONE-A SIZE BOUND makes a startup
	 *  budget assertion MANDATORY and requires that ZoneA_tokens be TOKENIZED
	 *  from the assembled Zone-A string -- never a recorded constant, because a
	 *  constant cannot grow when the place vocabulary widens, so the assertion
	 *  would pass most confidently at the exact moment it should fail. The plugin
	 *  cannot build that string (it may not know Siegebound), so the string has
	 *  to cross the boundary. It crosses as an FString, exactly like Prompt.
	 *
	 *  It is OPTIONAL. Without it the subsystem still runs, but the snapshot
	 *  budget CANNOT BE MEASURED and says so loudly, once -- see the .cpp.
	 *
	 *  @return false when a request is in flight (the prefix may not move under a
	 *          live KV cache) or when the string is empty.
	 */
	bool SetStaticPrefix(const FString& InStaticPrefix);

	/** The tier the resident model was actually loaded on. None until the load completes. */
	ESiegeLlamaOffloadTier GetOffloadTier() const;

	/** Human-readable one-line state, for logs and the Siege.Llama.Status command. */
	FString DescribeState() const;

	/**
	 *  Fires on the GAME THREAD, at most once per request, when SoftTimeoutSeconds
	 *  passes with the request still running. NOTHING IS ABORTED.
	 */
	FSiegeLlamaSoftTimeoutSignature OnRequestSoftTimeout;

	/**
	 *  INTERNAL -- the worker's game-thread landing point. Public only because the
	 *  completion task must reach it through a TWeakObjectPtr; it is not part of
	 *  the section 9 API and no game-lane code should call it.
	 */
	void HandleWorkerCompletion(bool bSuccess, const FString& Output, const FString& Error, bool bSoftTimeoutExceeded);

	/**
	 *  INTERNAL -- the worker's game-thread landing point for the soft timeout,
	 *  posted AT THE MOMENT the 4 s mark is crossed rather than carried on the
	 *  completion. Same visibility reason as HandleWorkerCompletion.
	 */
	void HandleSoftTimeout();

private:

	/**
	 *  Owns the model, the context, the vocab and the previous turn's tokens, and
	 *  is the ONLY thread that ever touches any of them. Declared in the .cpp so
	 *  no llama.h type reaches this header -- llama.h is a PRIVATE dependency and
	 *  must not land on the game module's include path.
	 *
	 *  A raw pointer rather than TUniquePtr on purpose: TUniquePtr to an
	 *  incomplete type needs an out-of-line destructor, and a UCLASS destructor
	 *  is not reliably out-of-line.
	 */
	FSiegeLlamaWorker* Worker = nullptr;

	/**
	 *  THE IN-FLIGHT COMPLETION CALLBACK, AND IT LIVES ON THE GAME THREAD AND
	 *  NOWHERE ELSE. The worker never sees it, never copies it and never executes
	 *  it -- it posts a plain (bool, FString, FString) to the game thread and this
	 *  object does the executing. That is what makes "OnComplete ALWAYS fires on
	 *  the GAME THREAD" a property of the structure rather than a promise.
	 */
	FSiegeLlamaCompletionSignature ActiveDelegate;
};
