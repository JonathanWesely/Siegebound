#include "SiegeLlamaSubsystem.h"

#include "SiegeLlamaModule.h"
#include "SiegeLlamaSettings.h"

#include "Async/Async.h"
#include "Containers/StringConv.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformAtomics.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

#if PLATFORM_WINDOWS
#include <excpt.h>
#endif

// The vendored C API. PRIVATE to this module by law -- llama.h must never reach
// the game module's include path (CONVENTIONS section 8: the whole surface
// between the lanes is (prompt, gbnf) -> string).
THIRD_PARTY_INCLUDES_START
#include "ggml-backend.h"
#include "llama.h"
THIRD_PARTY_INCLUDES_END

// LogSiegeLlama is DECLARED in SiegeLlamaLog.h (included via the header) and
// DEFINED ONCE in SiegeLlamaModule.cpp. There is deliberately NO
// DEFINE_LOG_CATEGORY here -- a second one is a duplicate-symbol link error.

// ===========================================================================
// 0. TAGS AND MEASURED CONSTANTS
// ===========================================================================

namespace SiegeLlamaPrivate
{
	static const TCHAR* const TagLoad    = TEXT("LLM_LOAD");
	static const TCHAR* const TagBudget  = TEXT("LLM_BUDGET");
	static const TCHAR* const TagRequest = TEXT("LLM_REQUEST");
	static const TCHAR* const TagGen     = TEXT("LLM_GEN");
	static const TCHAR* const TagFault   = TEXT("LLM_FAULT");

	/**
	 *  THE TIER TABLE. EVERY NUMBER BELOW IS A MEASUREMENT FROM TASK-413 ON
	 *  Qwen3-4B-Q4_K_M AT ctx=2048, ON ONE PINNED RTX 5070 Laptop (7891 MiB
	 *  total). They are properties of THAT model at THAT context size and are
	 *  RE-MEASURED, never carried, if either changes.
	 *
	 *    tier     | n_gpu_layers | model-attributable vram_delta | TTFT   | wall    | ms/token | aborts
	 *    ---------|--------------|-------------------------------|--------|---------|----------|--------
	 *    cpu      | 0 / 36       | 366.7 MiB (Vulkan scratch)    | 2716ms | 8730 ms | 156-182  | 2 of 5
	 *    partial  | 18 / 36      | 793.5 then 1264.0 MiB         | 1655ms | 6472 ms | 117-131  | 0 of 5
	 *    full     | 36 / 36      | 2702.8 then 2701.8 MiB        |  417ms | ~2100ms |  36-49   | 0
	 *
	 *  TWO THINGS THE TABLE SAYS THAT A SUMMARY WOULD LOSE:
	 *
	 *  1. THE CPU TIER MEASURABLY FAILS. 2 of 5 generations hit the 10 s ceiling
	 *     and returned TRUNCATED JSON. Selecting it is a DEGRADED state and this
	 *     file says so at Warning every time it happens. It is still selected
	 *     rather than refusing outright, because Jonathan's ruling 1 is that CPU
	 *     fallback is REQUIRED -- no vendor lock-in.
	 *
	 *  2. THE PARTIAL TIER'S OWN NUMBER HAS A SPREAD. 793.5 MiB and 1264.0 MiB
	 *     are two readings of the same configuration in two sessions -- 1.6x
	 *     apart. CONVENTIONS section 12h: the spread travels with the number, and
	 *     a bound is sized from the WORSE reading, never the flattering one.
	 */
	static constexpr uint64 FullOffloadCostMiB    = 2703;
	static constexpr uint64 PartialOffloadCostMiB = 1264;

	/**
	 *  VRAM that must remain free for the game's own rendering AFTER the model is
	 *  resident.
	 *
	 *  DERIVED FROM A MEASURED OPERAND: the FULL-OFFLOAD run went
	 *  vram_free_before = 3320.2 -> after = 617.4 MiB (handoffs/TASK-413-buildmaster.md:638),
	 *  and frame time was still acceptable with the card in that state -- the p99
	 *  attributable delta is "+2.0 to +4.6 ms on EVERY TIER" (:625). So ~617 MiB
	 *  was empirically survivable on this content; 768 is that survivable floor
	 *  rounded up with margin. It is not a guess about what a GPU needs in general.
	 *
	 *  THE CITATION IS WHAT WAS WRONG HERE, AND ONLY THE CITATION -- THE VALUE IS
	 *  UNCHANGED AT 768 (corrected 2026-08-03, qa/TASK-424.md WARN-3). This read
	 *  "bar #1 (frame time) still PASSED on p99 in exactly that state". 617.4 MiB
	 *  is the FULL-tier reading, but bar #1's result rows are scoped to the
	 *  PARTIAL tier in both of them (:599, :1103) -- so the sentence borrowed a
	 *  partial-tier bar's authority for a full-tier VRAM state. :625 is the
	 *  reading that actually spans every tier, which is why the CONCLUSION
	 *  survives the correction intact.
	 *
	 *  WHY A CITATION FIX IS WORTH THE LINES: a correct number resting on a figure
	 *  measured against a different noun is still the defect, because the next
	 *  reader to re-derive 768 follows the citation and lands on a measurement
	 *  that cannot support it. The number was never the problem.
	 */
	static constexpr uint64 VramGameReserveMiB = 768;

	/**
	 *  DERIVED, NOT MEASURED. IT IS SEPARATELY NAMED FOR EXACTLY THAT REASON.
	 *
	 *  WHY IT EXISTS. ContextTokens moved 2048 -> 3072 on Jonathan's ruling 2
	 *  (CONVENTIONS AS-section-21.0). KV-cache VRAM scales with n_ctx, but
	 *  FullOffloadCostMiB and PartialOffloadCostMiB above are MEASUREMENTS TAKEN
	 *  AT 2048 -- so raising the context leaves them stale LOW, and ChooseTier
	 *  PROMOTES on them. Stale-low on a promoting gate is stale in the UNSAFE
	 *  direction: it would let a machine promote into a tier it can no longer
	 *  run, and "the tier selector picks a tier it cannot run in" is already a
	 *  named defect on this project (CONVENTIONS FT-section-6).
	 *
	 *  WHY IT IS A NEW CONSTANT INSTEAD OF 2703 BECOMING 2854. Editing a
	 *  measured constant by arithmetic is A DERIVATION WEARING A MEASUREMENT'S
	 *  AUTHORITY -- the precise defect AS-section-23 names (a measurement's BASE
	 *  is part of the measurement) and AS-section-12g bans. The measured numbers
	 *  keep their provenance; the uncertainty stays visible in the source
	 *  instead of being folded into a number that LOOKS measured.
	 *
	 *  THE DERIVATION, WITH ITS ASSUMPTION NAMED:
	 *    36 layers x 8 KV heads x 128 head-dim x 2 (K+V) x 2 bytes (f16)
	 *      = 147,456 bytes per token
	 *    x 1024 added tokens (3072 - 2048) = 150,994,944 bytes = 144 MiB exactly.
	 *  WARNING: THE 128 HEAD-DIM IS AN ASSUMPTION, not a read of the GGUF. So is f16
	 *  KV; a quantised KV cache would make this smaller, never larger.
	 *
	 *  WHY 151 AND NOT 144, STATED SO IT IS A CHOICE RATHER THAN A SLIP. 144 MiB
	 *  is the honest binary conversion; 151 is the same quantity in decimal MB
	 *  (150.99 MB), which is the figure AS-section-21.7 records. The LARGER of
	 *  the two is kept deliberately. AS-section-19's safe-direction rule: A GATE
	 *  THAT PROMOTES MUST ASSUME THE LARGER COST. Erring high costs a machine on
	 *  the boundary one tier of speed; erring low costs it a tier it cannot run.
	 *
	 *  IT IS APPLIED TO BOTH GATES, AND ON THE PARTIAL GATE THAT IS KNOWINGLY
	 *  CONSERVATIVE. At PartialGpuLayers = 18 of 36 only the offloaded half of
	 *  the KV cache is GPU-resident, so the layer-scaled figure would be roughly
	 *  half of this. Applying the full figure to both comparisons is the ruled
	 *  shape and the safe one; it is NOT an oversight, and it is NOT to be
	 *  "corrected" by scaling it -- a second derived number stacked on the first
	 *  is not more knowledge, it is more assumption.
	 *
	 *  THIS IS A PLACEHOLDER FOR A READING THAT DOES NOT EXIST YET. TASK-552
	 *  (Stage 5) prints the LIVE vram_delta at 3072 on the load line, and that
	 *  reading REPLACES this value -- at which point this constant either goes
	 *  away or becomes a measured one with its own provenance block.
	 */
	static constexpr uint64 ContextGrowthVramReserveMiB = 151;

	/** 18 of 36 -- the exact partial configuration every partial-tier number above was taken on. */
	static constexpr int32 PartialGpuLayers = 18;

	/** The layer count those ratios were measured against. A different count invalidates the 18. */
	static constexpr int32 MeasuredModelLayers = 36;

	static constexpr uint64 BytesPerMiB = 1048576ull;

	static const TCHAR* TierName(ESiegeLlamaOffloadTier Tier)
	{
		switch (Tier)
		{
			case ESiegeLlamaOffloadTier::CpuOnly:     return TEXT("cpu");
			case ESiegeLlamaOffloadTier::Partial:     return TEXT("partial");
			case ESiegeLlamaOffloadTier::FullOffload: return TEXT("full");
			default:                                  return TEXT("none");
		}
	}

	/**
	 *  THE ABORT REGIME, PRINTED WHEREVER THE DEADLINE IS. The same 10.0 s means
	 *  two different things on two different tiers and the difference is in the
	 *  vendored header rather than inferable from a log:
	 *
	 *    llama.h:382-385, verbatim: "Abort callback / if it returns true,
	 *    execution of llama_decode() will be aborted / currently works only with
	 *    CPU execution."
	 *
	 *  Without this label the next reader compares a cpu wall time against a
	 *  full-offload wall time at the same deadline and concludes something false
	 *  about the knob.
	 *
	 *  WHAT THE LABEL DESCRIBES IS WHERE THE CUT LANDS, NOT WHETHER IT HAPPENS
	 *  (reworded 2026-08-03, qa/TASK-424.md WARN-2). The deadline now binds on
	 *  every tier because RunRequestUnguarded tests it in the loops themselves;
	 *  the regime is the GRANULARITY of that cut, and on the GPU tiers it is one
	 *  llama_decode call rather than mid-graph. The previous GPU wording ended
	 *  "MaxOutputTokens is the real bound", which was true of the code as it then
	 *  stood and is no longer true of this one.
	 */
	static const TCHAR* AbortRegime(ESiegeLlamaOffloadTier Tier)
	{
		return (Tier == ESiegeLlamaOffloadTier::CpuOnly)
			? TEXT("AUTHORITATIVE (cpu tier: abort_callback cuts llama_decode mid-graph, llama.h:382-385)")
			: TEXT("BACKSTOP AT SLICE GRANULARITY (layers on device: abort_callback is documented CPU-only -- llama.h:382-385 -- so the worker's own between-slice and per-token tests are what enforce the deadline here; the cut lands at the end of the llama_decode call in flight, at most one n_batch prefill slice or one decoded token late, NOT mid-graph)");
	}

	// enum ggml_backend_dev_type is SPELLED OUT deliberately: ggml declares a
	// FUNCTION of exactly that name (ggml-backend.h:182), which hides the enum
	// type in C++. The unelaborated spelling names the function and does not
	// compile. Upstream writes it the same way for the same reason.
	static const TCHAR* DeviceTypeName(const enum ggml_backend_dev_type Type)
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

	static int64 NowMicroseconds()
	{
		return static_cast<int64>(FPlatformTime::Seconds() * 1000000.0);
	}

	/** Leave the machine two cores. Generation is TPri_BelowNormal but it is still work. */
	static int32 DefaultThreadCount()
	{
		const int32 Cores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
		return FMath::Max(1, Cores - 2);
	}
}

// ===========================================================================
// 1. THE WORKER -- THE ONLY THREAD THAT EVER TOUCHES llama_context
// ===========================================================================

/**
 *  ONE DEDICATED FRunnable AT TPri_BelowNormal, ALIVE FOR THE SUBSYSTEM'S WHOLE
 *  LIFETIME.
 *
 *  WHY A LONG-LIVED RUNNABLE AND NOT THE TASK GRAPH OR Async():
 *
 *    llama_context is NOT thread-safe. Every design that satisfies that with
 *    task-graph work or Async() satisfies it by CONVENTION -- "we only ever
 *    submit one at a time", "the tasks are serialised by the FSM" -- which is a
 *    comment, enforced by nobody, on a codebase where seven tasks run in
 *    parallel and any of them can add a second submission site.
 *
 *    A single runnable that OWNS the pointers makes it STRUCTURAL. Model,
 *    context, vocab and LastPromptTokens are members of this object; no other
 *    object holds them; every function that dereferences them is a member of
 *    this class and runs on Run(). The subsystem holds a pointer to the worker,
 *    never to the context. There is no path by which a second thread can reach
 *    llama_context without adding a new member function AND calling it from off
 *    the run loop -- which is a visible, reviewable act rather than an omission.
 *
 *    The task graph additionally CANNOT give this guarantee even in principle:
 *    its threads are shared, a task may resume on a different worker, and
 *    TPri_BelowNormal for a multi-second CPU burn is not something to hand to a
 *    pool the renderer also uses.
 *
 *  THREAD-OWNERSHIP MAP, so a reviewer can check it by reading rather than by
 *  reasoning:
 *
 *    GAME THREAD ONLY   : Initialize, Deinitialize, RequestCompletion,
 *                         CancelActiveRequest, SetStaticPrefix, IsReady, IsBusy,
 *                         the completion delegate, ActiveDelegate, the console
 *                         commands, USiegeLlamaSettings::ResolveModelPath,
 *                         FSiegeLlamaModule::EnsureBackendsLoaded.
 *    WORKER THREAD ONLY : Model, Context, Vocab, LastPromptTokens,
 *                         StaticPrefixTokenCount, every llama_ and ggml_ call
 *                         except the two callbacks below.
 *    ANY THREAD (atomic): State, bCancelRequested, bStopRequested,
 *                         AbortDeadlineUsec, and the RequestLock-guarded
 *                         handoff slot. Nothing else.
 *    GGML COMPUTE THREADS: AbortThunk only, which reads three atomics and
 *                         touches nothing else.
 */
class FSiegeLlamaWorker : public FRunnable
{
public:

	enum class EState : int32
	{
		Loading = 0,
		Idle,
		Busy,
		Faulted,
		Stopped,
	};

	explicit FSiegeLlamaWorker(const FString& InModelPath, TWeakObjectPtr<USiegeLlamaSubsystem> InOwner)
		: Owner(InOwner)
		, ModelPath(InModelPath)
		, State(static_cast<int32>(EState::Loading))
	{
		WakeEvent = FPlatformProcess::GetSynchEventFromPool(false);
		Thread = FRunnableThread::Create(this, TEXT("SiegeLlamaWorker"), 0, TPri_BelowNormal);
	}

	virtual ~FSiegeLlamaWorker() override
	{
		JoinAndDestroy();
	}

	/** Game thread. Stops the run loop, aborts an in-flight decode AND an in-flight model load, joins. */
	void JoinAndDestroy()
	{
		bStopRequested = true;

		if (WakeEvent != nullptr)
		{
			WakeEvent->Trigger();
		}

		if (Thread != nullptr)
		{
			// Blocks until Run() returns. Run() unloads the model on its own
			// thread before returning, which is what keeps the free off the game
			// thread and out of a race with an aborting decode.
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
		}

		if (WakeEvent != nullptr)
		{
			FPlatformProcess::ReturnSynchEventToPool(WakeEvent);
			WakeEvent = nullptr;
		}
	}

	//~ FRunnable
	//
	// ALL FOUR SIGNATURES RE-CHECKED AGAINST THE BASE CLASS ITSELF, NOT AGAINST
	// HABIT (Engine/Source/Runtime/Core/Public/HAL/Runnable.h:32-61). Run() is
	// the ONLY one of the four that is not void:
	//
	//     virtual bool   Init()     -- matches
	//     virtual uint32 Run() = 0  -- NOT void; it returns the thread exit code
	//     virtual void   Stop()     -- matches
	//     virtual void   Exit()     -- matches
	//
	// This read "virtual void Run() override" until 2026-08-03: error C2555,
	// "return type differs and is not covariant". Two things about that are
	// worth leaving here rather than deleting with the fix:
	//
	//   1. The compiler DOES catch it -- but only in THIS translation unit,
	//      because the worker class is private to the .cpp. The two overrides
	//      in the PUBLIC header (Initialize/Deinitialize) are re-validated by
	//      every game-lane TU that includes it; nothing else ever sees these
	//      four. This class gets exactly one reader, so it gets one chance.
	//   2. GetSingleThreadInterface() is deliberately NOT overridden. The base
	//      returns nullptr, which means this runnable is not ticked when
	//      FPlatformProcess::SupportsMultithreading() is false. That is correct
	//      for a multi-second CPU burn that must never run on the game thread:
	//      on such a platform the assistant is simply unavailable, which is the
	//      same degradation as a missing model file.
	//
	// Do not "tidy" Run() back to void.
	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	virtual void Stop() override { bStopRequested = true; if (WakeEvent) { WakeEvent->Trigger(); } }
	virtual void Exit() override {}
	//~ End FRunnable

	// --- state, readable from any thread -----------------------------------
	EState GetState() const { return static_cast<EState>(State.GetValue()); }
	bool IsReadyState() const { return GetState() == EState::Idle; }
	bool IsBusyState() const { return GetState() == EState::Busy; }

	ESiegeLlamaOffloadTier GetTier() const { return static_cast<ESiegeLlamaOffloadTier>(TierValue.GetValue()); }
	int32 GetActualContextSize() const { return ActualContextSize.GetValue(); }
	int32 GetStaticPrefixTokens() const { return StaticPrefixTokens.GetValue(); }
	const FString& GetModelPath() const { return ModelPath; }

	/**
	 *  GAME THREAD. Publishes the request, takes the Idle -> Busy transition, and
	 *  wakes the worker.
	 *
	 *  The caller (RequestCompletion) has already CHECKED IsReady() and IsBusy();
	 *  it does not take the transition -- MarkBusy() below does, and it can refuse
	 *  it if the worker latched a fault in between. That refusal does not strand
	 *  the caller: see MarkBusy and the run loop's dispatch branch.
	 */
	void SubmitRequest(const FString& InPrompt, const FString& InGrammar)
	{
		// ORDER MATTERS AND IT IS THE WHOLE HANDOFF PROTOCOL:
		//   1. publish the payload under the lock,
		//   2. flip Idle -> Busy,
		//   3. raise bWorkPending,
		//   4. wake.
		// The worker only DISPATCHES once it observes bWorkPending, and it
		// re-tests Busy before it does, so it can never see a half-published
		// request. Both writes are interlocked, and FEvent::Trigger before
		// FEvent::Wait is not lost -- the wait returns immediately.
		{
			FScopeLock Lock(&RequestLock);
			PendingPrompt = InPrompt;
			PendingGrammar = InGrammar;
		}
		bCancelRequested = false;

		// STEPS 2 AND 3 WERE THE OTHER WAY ROUND UNTIL 2026-08-03, AND THE ORDER
		// IS LOAD-BEARING RATHER THAN TIDY (qa/TASK-424.md WARN-1).
		//
		// MarkBusy() now REFUSES to leave a Faulted or Stopped state, and the run
		// loop re-tests Busy before dispatching. Those two changes are only safe
		// together with this one: if bWorkPending were raised FIRST, the worker's
		// 200 ms bounded wait could tick in the gap between the two statements,
		// observe pending work, find the state still Idle -- because the game
		// thread has not reached MarkBusy() yet -- and refuse a PERFECTLY HEALTHY
		// request. Publishing Busy first collapses that window: by construction,
		// bWorkPending == true implies MarkBusy() has already run.
		//
		// The comment above claimed this order before the code did. It does now.
		MarkBusy();

		// TWO SEPARATE FLAGS, ON PURPOSE. bWorkPending means "the worker has
		// something to do" and the WORKER clears it; Busy means "a request is in
		// flight" and the GAME THREAD clears it, later, when the completion
		// lands. Driving the run loop off Busy alone would re-enter the same
		// request in the window between PostCompletion and the game thread
		// getting to ClearBusy -- and it would re-enter it with an empty payload.
		// The run loop's added state test does NOT change that: it is an extra
		// CONDITION on the bWorkPending edge, never a replacement for it.
		bWorkPending = true;

		if (WakeEvent != nullptr)
		{
			WakeEvent->Trigger();
		}
	}

	/** GAME THREAD. Queues an opaque static prefix for tokenisation + the budget assertion on the worker. */
	void SubmitStaticPrefix(const FString& InPrefix)
	{
		{
			FScopeLock Lock(&RequestLock);
			PendingStaticPrefix = InPrefix;
			bStaticPrefixDirty = true;
		}
		if (WakeEvent != nullptr)
		{
			WakeEvent->Trigger();
		}
	}

	/** GAME THREAD. The prefix as the game lane last registered it (for the char pre-filter). */
	FString GetStaticPrefixCopy() const
	{
		FScopeLock Lock(&RequestLock);
		return AppliedStaticPrefix;
	}

	/** ANY THREAD. Idempotent. Routed into abort_callback; see ShouldAbortNow. */
	void RequestCancel() { bCancelRequested = true; }

	/**
	 *  GAME THREAD, from HandleWorkerCompletion only. Busy -> Idle, and it never
	 *  clears a Faulted or Stopped state -- a fault is a session latch.
	 */
	void ClearBusy()
	{
		if (GetState() == EState::Busy)
		{
			bCancelRequested = false;
			State.Set(static_cast<int32>(EState::Idle));
		}
	}

	/**
	 *  GAME THREAD, from RequestCompletion only, after it has won the Idle -> Busy
	 *  transition.
	 *
	 *  SYMMETRIC WITH ClearBusy BY CONSTRUCTION, NOT BY HABIT (added 2026-08-03,
	 *  qa/TASK-424.md WARN-1). A fault is a SESSION latch, so NEITHER direction
	 *  may cross it. ClearBusy has always refused to clear Faulted or Stopped;
	 *  this refused nothing, so the pair only LOOKED like a pair.
	 *
	 *  THE WINDOW THAT MADE IT REACHABLE, NAMED SO IT CAN BE CHECKED: the game
	 *  thread sits between RequestCompletion's IsReady() and this call while the
	 *  WORKER runs ApplyPendingStaticPrefix() -> RunBudgetAssertion() ->
	 *  LatchFaulted(), which is the one worker transition out of Idle. Unguarded,
	 *  this then overwrote Faulted with Busy and the request decoded on a session
	 *  that had already declared itself unusable.
	 *
	 *  A REFUSAL HERE IS NOT A DROPPED REQUEST. bWorkPending is raised after this
	 *  either way, and the run loop completes the request on its refusal branch --
	 *  which is what keeps RequestCompletion's "a true return means OnComplete
	 *  fires exactly once" true through this path as well.
	 */
	void MarkBusy()
	{
		if (GetState() != EState::Idle)
		{
			return;
		}
		State.Set(static_cast<int32>(EState::Busy));
	}

	/**
	 *  SEH ENTRY POINTS. Public only so the file-static thunks in
	 *  SiegeLlamaPrivate can reach them: MSVC forbids __try in a function that
	 *  needs object unwinding, so the guarded bodies have to be one call away.
	 *  Never call these directly -- LoadModelGuarded and RunRequestGuarded are
	 *  the entry points, and they are the ones that latch a fault.
	 */
	void InvokeLoadUnguarded() { LoadModelUnguarded(); }
	void InvokeRequestUnguarded() { RunRequestUnguarded(); }

	/**
	 *  CALLED FROM GGML'S COMPUTE THREADS. Reads three atomics and nothing else.
	 *  Anything heavier here runs inside the inner graph loop.
	 */
	bool ShouldAbortNow() const
	{
		if (bCancelRequested || bStopRequested)
		{
			return true;
		}
		const int64 Deadline = FPlatformAtomics::AtomicRead(&AbortDeadlineUsec);
		return Deadline > 0 && SiegeLlamaPrivate::NowMicroseconds() > Deadline;
	}

	/** CALLED FROM llama's MODEL LOADER. Returning false aborts the load -- which is how shutdown does not wait 2.5 GB. */
	bool ShouldContinueLoad() const { return !bStopRequested; }

	FString DescribeState() const;

private:

	// --- run-loop steps (WORKER THREAD ONLY) -------------------------------
	void LoadModelGuarded();
	void LoadModelUnguarded();
	void ApplyPendingStaticPrefix();
	void RunRequestGuarded();
	void RunRequestUnguarded();
	void UnloadModel();

	bool Tokenize(const FString& Text, bool bAddSpecial, TArray<llama_token>& OutTokens) const;
	ESiegeLlamaOffloadTier ChooseTier(ggml_backend_dev_t& OutDevice, FString& OutDeviceLabel, FString& OutReason) const;
	void RunBudgetAssertion();

	/** Posts the completion to the game thread. The ONLY way a result leaves this thread. */
	void PostCompletion(bool bSuccess, const FString& Output, const FString& Error, bool bSoftTimeoutExceeded);

	/**
	 *  Posts the SOFT timeout to the game thread AT THE MOMENT IT IS CROSSED.
	 *
	 *  It has to be its own post rather than a flag on the completion: the whole
	 *  point of a soft timeout is to give the FSM something to say WHILE the
	 *  player is waiting, and a flag that arrives with the answer arrives after
	 *  the wait it was meant to cover.
	 */
	void NotifySoftTimeout();

	/**
	 *  Crosses the soft deadline at most once per request, from EITHER loop.
	 *  Checked in the prefill loop as well as the decode loop because on the cpu
	 *  tier the prefill alone can outlast 4 s -- a soft timeout that only watches
	 *  the decode is silent for exactly the slowest turns.
	 */
	void CheckSoftTimeout(double SoftDeadlineSeconds, int32 OutputTokensSoFar, bool& bInOutFired);

	/** Latches faulted, logs once, and leaves IsReady() false for the rest of the session. */
	void LatchFaulted(const FString& Reason);

	// --- owned by the worker thread, touched by nothing else ---------------
	llama_model* Model = nullptr;
	llama_context* Context = nullptr;
	const llama_vocab* Vocab = nullptr;

	/** The previous turn's prompt tokens. A KV CACHE KEY, NOT A CONVERSATION -- see invariant 4. */
	TArray<llama_token> LastPromptTokens;

	/** The registered static prefix, tokenised. Worker-thread copy. */
	TArray<llama_token> StaticPrefixTokenArray;

	int32 EffectiveBatchSize = USiegeLlamaSubsystem::BatchTokens;
	int32 ModelLayerCount = 0;
	uint64 ModelSizeBytes = 0;
	bool bWarnedNoStaticPrefix = false;

	// --- cross-thread -------------------------------------------------------
	TWeakObjectPtr<USiegeLlamaSubsystem> Owner;
	FString ModelPath;

	FRunnableThread* Thread = nullptr;
	FEvent* WakeEvent = nullptr;

	FThreadSafeCounter State;
	FThreadSafeCounter TierValue{ static_cast<int32>(ESiegeLlamaOffloadTier::None) };
	FThreadSafeCounter ActualContextSize{ 0 };
	FThreadSafeCounter StaticPrefixTokens{ 0 };

	FThreadSafeBool bCancelRequested{ false };
	FThreadSafeBool bStopRequested{ false };

	/** Set by the game thread at submit, cleared by the WORKER when it picks the request up. See SubmitRequest. */
	FThreadSafeBool bWorkPending{ false };

	/** 0 = no deadline armed. Microseconds on FPlatformTime's clock. Written by the worker, read by ggml threads. */
	mutable volatile int64 AbortDeadlineUsec = 0;

	mutable FCriticalSection RequestLock;
	FString PendingPrompt;
	FString PendingGrammar;
	FString PendingStaticPrefix;
	FString AppliedStaticPrefix;
	bool bStaticPrefixDirty = false;
};

// ===========================================================================
// 2. SEH -- A GGML FAULT MUST NOT KILL THE GAME
// ===========================================================================
//
// This also covers the delay-load trap TASK-409 flagged: calling any llama_ or
// ggml_ entry point while the vendored DLLs are unresolved raises a structured
// exception rather than returning an error.
//
// THE WORDING ABOVE IS DELIBERATE. Writing the two prefixes as a glob pair puts
// a literal comment terminator inside a comment and ends it early -- the
// CONVENTIONS section 10 compile trap.

namespace SiegeLlamaPrivate
{
	static void LoadThunk(void* Context);
	static void RequestThunk(void* Context);

	/**
	 *  MSVC forbids __try in a function that needs object unwinding, so this
	 *  function has NO destructible locals. The thunks it calls may have as many
	 *  as they like.
	 *
	 *  A fault here does NOT run the destructors of anything on the aborted
	 *  frame. That is accepted: the session is faulted from that point on and no
	 *  further inference is attempted, so a leaked sampler chain on a dead path
	 *  is strictly better than taking the editor down with it.
	 */
	static bool SehInvoke(void (*Thunk)(void*), void* Context, uint32& OutExceptionCode)
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

	/** llama's abort_callback. GGML COMPUTE THREADS. Three atomic reads, nothing else. */
	static bool AbortThunk(void* Data)
	{
		const FSiegeLlamaWorker* Self = static_cast<const FSiegeLlamaWorker*>(Data);
		return Self != nullptr && Self->ShouldAbortNow();
	}

	/** llama's progress_callback. Returning FALSE aborts the model load outright (llama.h:323-325). */
	static bool LoadProgressThunk(float /*Progress*/, void* Data)
	{
		const FSiegeLlamaWorker* Self = static_cast<const FSiegeLlamaWorker*>(Data);
		return Self == nullptr || Self->ShouldContinueLoad();
	}
}

// ===========================================================================
// 3. THE RUN LOOP
// ===========================================================================

uint32 FSiegeLlamaWorker::Run()
{
	LoadModelGuarded();

	while (!bStopRequested)
	{
		// Bounded wait rather than an infinite one: Stop() triggers the event,
		// but a bounded wait means a missed trigger costs 200 ms instead of a
		// hung shutdown.
		if (WakeEvent != nullptr)
		{
			WakeEvent->Wait(200u);
		}

		if (bStopRequested)
		{
			break;
		}

		// APPLIED BEFORE THE WORK BRANCH, AND THAT ORDER IS A CROSS-LANE CONTRACT
		// -- the game lane registers a prefix and submits a request in the same
		// frame and relies on this to make the token authority live on THAT
		// request (SiegeAssistantComponent.cpp, the static-prefix backstop). The
		// order is UNCHANGED by the state test added below.
		ApplyPendingStaticPrefix();

		if (bWorkPending)
		{
			bWorkPending = false;

			// THE STATE IS RE-TESTED HERE, NOT INFERRED FROM bWorkPending
			// (qa/TASK-424.md WARN-1). SubmitRequest sets Busy BEFORE it raises
			// the flag, so the healthy path always passes this -- but
			// ApplyPendingStaticPrefix() one line above can LatchFaulted() on the
			// very iteration that is about to dispatch, and MarkBusy() refuses to
			// leave a Faulted state. Dispatching on the flag alone would decode a
			// request on a session that has already declared itself unusable.
			if (GetState() == EState::Busy)
			{
				RunRequestGuarded();
			}
			else
			{
				// A COMPLETION IS OWED HERE AND IT IS THE CONTRACT THAT OWES IT,
				// NOT TIDINESS. RequestCompletion has ALREADY returned true to its
				// caller, and that return means "OnComplete fires exactly once,
				// later, on the game thread, whatever happens". Dropping the
				// request instead would strand the FSM in "thinking" for the rest
				// of the match with no error, no log and no crash -- the most
				// expensive failure shape this project has.
				{
					// The payload is drained so a stale prompt cannot be picked
					// up by a later request that reuses this slot.
					FScopeLock Lock(&RequestLock);
					PendingPrompt.Reset();
					PendingGrammar.Reset();
				}

				const FString Reason = FString::Printf(
					TEXT("the session left the Busy state before the worker picked the request up, so nothing was decoded (%s)"),
					*DescribeState());

				UE_LOG(LogSiegeLlama, Warning,
					TEXT("%s: REQUEST REFUSED BEFORE DISPATCH -- %s. The completion still fires, on the game thread, with bSuccess=false."),
					SiegeLlamaPrivate::TagRequest, *Reason);

				PostCompletion(false, FString(), Reason, false);
			}
		}
	}

	// Freed HERE, on the worker, before Run() returns and WaitForCompletion
	// releases the game thread. That is what makes "shutdown joins the worker
	// cleanly and frees the model" a single ordered sequence rather than a race
	// between a free on the game thread and a decode on this one.
	UnloadModel();
	State.Set(static_cast<int32>(EState::Stopped));

	// THIS IS THE ONE AND ONLY EXIT FROM Run(), AND THAT PROPERTY IS WHAT MADE
	// THE void -> uint32 CONVERSION SAFE (2026-08-03, C2555). Traced before it
	// was typed, because the conversion "forces a return on every exit path"
	// and this loop is where the ratified MarkBusy() / bWorkPending ordering
	// lives.
	//
	// THE TRACE: this function contains ZERO other return statements. The one
	// `break` above leaves the WHILE LOOP, not the function, so BOTH exits --
	// the break and the loop condition going false -- converge on this same
	// tail, AFTER UnloadModel() and AFTER the Stopped transition. Adding one
	// return here therefore introduced NO new way to leave a loop, so there is
	// no new early exit for a later guard to be checked against. The dispatch
	// branch, its else branch and its PostCompletion are untouched and still
	// run to completion before control ever reaches this line.
	//
	// DO NOT convert that `break` into a `return`. It reads as equivalent and
	// is not: it would skip UnloadModel() and the Stopped transition, leaking
	// the ~2.5 GB model and letting JoinAndDestroy's WaitForCompletion hand the
	// game thread a worker whose state still reads Loading, Idle or Busy.
	//
	// The value is FRunnable's thread exit code (HAL/Runnable.h:42). Nothing in
	// this plugin reads it, and 0 is the engine-wide convention for "ran to
	// completion". A fault is reported through EState::Faulted and the
	// completion delegate -- never through this return value, which is why a
	// non-zero code here would be a claim no caller could act on.
	return 0;
}

void FSiegeLlamaWorker::LatchFaulted(const FString& Reason)
{
	State.Set(static_cast<int32>(EState::Faulted));
	UE_LOG(LogSiegeLlama, Error,
		TEXT("%s: bAssistantFaulted LATCHED for the rest of this session -- %s. IsReady() is false from here on. The match is unaffected: every keyboard command still works and the console simply stays unavailable."),
		SiegeLlamaPrivate::TagFault, *Reason);
}

// ===========================================================================
// 4. LOAD -- DEVICE PINNING, TIER SELECTION, THE ACTUALS, THE ASSERTION
// ===========================================================================

namespace SiegeLlamaPrivate
{
	static void LoadThunk(void* Context)
	{
		// One call away from the __try, so the body may hold destructible locals.
		static_cast<FSiegeLlamaWorker*>(Context)->InvokeLoadUnguarded();
	}
}

void FSiegeLlamaWorker::LoadModelGuarded()
{
	uint32 ExceptionCode = 0;
	if (!SiegeLlamaPrivate::SehInvoke(&SiegeLlamaPrivate::LoadThunk, this, ExceptionCode))
	{
		LatchFaulted(FString::Printf(
			TEXT("the model load FAULTED (SEH code 0x%08X). The editor was not taken down with it"),
			ExceptionCode));
		return;
	}

	if (GetState() == EState::Loading)
	{
		// LoadModelUnguarded ran to completion without setting a terminal state.
		if (Model != nullptr && Context != nullptr)
		{
			State.Set(static_cast<int32>(EState::Idle));
		}
		else
		{
			LatchFaulted(TEXT("the load completed without producing a model and a context"));
		}
	}
}

ESiegeLlamaOffloadTier FSiegeLlamaWorker::ChooseTier(ggml_backend_dev_t& OutDevice, FString& OutDeviceLabel, FString& OutReason) const
{
	using namespace SiegeLlamaPrivate;

	OutDevice = nullptr;
	OutDeviceLabel = TEXT("<none>");
	OutReason.Reset();

	// --- enumerate, and PRINT, every device -------------------------------
	// The subject of a VRAM figure goes on the page rather than into someone's
	// head. llama.h:307 documents `devices` as "if NULL, all available devices
	// are used" and split_mode defaults to LAYER, so the default lets the weights
	// straddle two cards while a memory sample reports the free mark of ONE. A
	// tier must mean one named device.
	const int32 DeviceCount = static_cast<int32>(ggml_backend_dev_count());
	if (DeviceCount == 0)
	{
		OutReason = TEXT("ggml enumerates ZERO devices, so there is nothing to offload to and no VRAM figure has a subject");
		return ESiegeLlamaOffloadTier::CpuOnly;
	}

	ggml_backend_dev_t BestDiscrete = nullptr;
	ggml_backend_dev_t BestIntegrated = nullptr;
	uint64 BestDiscreteFree = 0;
	uint64 BestIntegratedFree = 0;
	FString BestDiscreteLabel;
	FString BestIntegratedLabel;

	for (int32 Index = 0; Index < DeviceCount; ++Index)
	{
		ggml_backend_dev_t Device = ggml_backend_dev_get(static_cast<size_t>(Index));
		if (Device == nullptr)
		{
			continue;
		}

		const char* NameUtf8 = ggml_backend_dev_name(Device);
		const char* DescUtf8 = ggml_backend_dev_description(Device);
		const FString Name = NameUtf8 != nullptr ? FString(UTF8_TO_TCHAR(NameUtf8)) : FString(TEXT("<unnamed>"));
		const FString Desc = DescUtf8 != nullptr ? FString(UTF8_TO_TCHAR(DescUtf8)) : FString(TEXT("<no description>"));
		const enum ggml_backend_dev_type Type = ggml_backend_dev_type(Device);

		size_t FreeBytes = 0;
		size_t TotalBytes = 0;
		ggml_backend_dev_memory(Device, &FreeBytes, &TotalBytes);

		const FString Label = FString::Printf(TEXT("dev=%d/%s/%s"), Index, *Name, DeviceTypeName(Type));

		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s device %s free=%.1f MiB total=%.1f MiB desc=\"%s\""),
			TagLoad, *Label,
			static_cast<double>(FreeBytes) / static_cast<double>(BytesPerMiB),
			static_cast<double>(TotalBytes) / static_cast<double>(BytesPerMiB),
			*Desc);

		if (Type == GGML_BACKEND_DEVICE_TYPE_GPU && BestDiscrete == nullptr)
		{
			BestDiscrete = Device;
			BestDiscreteFree = static_cast<uint64>(FreeBytes);
			BestDiscreteLabel = Label;
		}
		else if (Type == GGML_BACKEND_DEVICE_TYPE_IGPU && BestIntegrated == nullptr)
		{
			BestIntegrated = Device;
			BestIntegratedFree = static_cast<uint64>(FreeBytes);
			BestIntegratedLabel = Label;
		}
	}

	uint64 FreeMiB = 0;
	if (BestDiscrete != nullptr)
	{
		OutDevice = BestDiscrete;
		OutDeviceLabel = BestDiscreteLabel;
		FreeMiB = BestDiscreteFree / BytesPerMiB;
	}
	else if (BestIntegrated != nullptr)
	{
		// An iGPU shares system RAM and TASK-413 measured one reporting free
		// (47690 MiB) GREATER than total (37020 MiB) -- an incoherent pair. It is
		// used, because Jonathan's ruling 1 requires no vendor lock-in, but its
		// headroom reading is not trusted to promote a tier.
		OutDevice = BestIntegrated;
		OutDeviceLabel = BestIntegratedLabel;
		FreeMiB = BestIntegratedFree / BytesPerMiB;
		OutReason = TEXT("no DISCRETE GPU is enumerated. The integrated GPU is pinned, its free/total pair is not trusted to promote a tier, and the tier is held at PARTIAL");
		return ESiegeLlamaOffloadTier::Partial;
	}
	else
	{
		OutReason = TEXT("no GPU device is enumerated at all -- every layer stays on the CPU");
		return ESiegeLlamaOffloadTier::CpuOnly;
	}

	if (FreeMiB == 0)
	{
		// A device exists but reports nothing usable. PARTIAL is the documented
		// default: it is the tier the frame-time bar is written against, and it
		// is the conservative choice when headroom is unknown rather than known
		// to be large.
		OutReason = TEXT("the pinned device reports 0 MiB free, which is a missing reading rather than a measurement. Defaulting to PARTIAL, the tier bar #1 was measured on");
		return ESiegeLlamaOffloadTier::Partial;
	}

	// BOTH PROMOTING COMPARISONS CARRY ContextGrowthVramReserveMiB, and every
	// reason string below NAMES it. A selector whose reason omits an operand it
	// actually used is a log that lies quietly -- the next reader re-adds the
	// measured numbers by hand, gets a different threshold, and concludes the
	// code is doing something other than what it is doing.
	if (FreeMiB >= FullOffloadCostMiB + VramGameReserveMiB + ContextGrowthVramReserveMiB)
	{
		OutReason = FString::Printf(
			TEXT("%llu MiB free >= %llu (measured full-offload cost, taken at ctx=2048) + %llu (game reserve) + %llu (DERIVED ctx-growth KV reserve for 2048->3072) = %llu"),
			FreeMiB, FullOffloadCostMiB, VramGameReserveMiB, ContextGrowthVramReserveMiB,
			FullOffloadCostMiB + VramGameReserveMiB + ContextGrowthVramReserveMiB);
		return ESiegeLlamaOffloadTier::FullOffload;
	}

	if (FreeMiB >= PartialOffloadCostMiB + VramGameReserveMiB + ContextGrowthVramReserveMiB)
	{
		OutReason = FString::Printf(
			TEXT("%llu MiB free is below %llu needed for full offload but >= %llu (worse of the two measured partial readings) + %llu (game reserve) + %llu (DERIVED ctx-growth KV reserve for 2048->3072) = %llu"),
			FreeMiB, FullOffloadCostMiB + VramGameReserveMiB + ContextGrowthVramReserveMiB,
			PartialOffloadCostMiB, VramGameReserveMiB, ContextGrowthVramReserveMiB,
			PartialOffloadCostMiB + VramGameReserveMiB + ContextGrowthVramReserveMiB);
		return ESiegeLlamaOffloadTier::Partial;
	}

	// THE FALL-THROUGH ITSELF IS UNCHANGED -- same condition, same tier, same
	// known defect (the CPU tier measurably fails: 2 of 5 generations hit the
	// ceiling). Only the THRESHOLD IT REPORTS moved, because the partial gate
	// above moved. Reporting the old number here would print a threshold the
	// code never tested against.
	OutReason = FString::Printf(
		TEXT("only %llu MiB free, below the %llu needed even for the partial tier (%llu measured + %llu game reserve + %llu DERIVED ctx-growth KV reserve)"),
		FreeMiB, PartialOffloadCostMiB + VramGameReserveMiB + ContextGrowthVramReserveMiB,
		PartialOffloadCostMiB, VramGameReserveMiB, ContextGrowthVramReserveMiB);
	return ESiegeLlamaOffloadTier::CpuOnly;
}

void FSiegeLlamaWorker::LoadModelUnguarded()
{
	using namespace SiegeLlamaPrivate;

	const double LoadStart = FPlatformTime::Seconds();

	// --- 1. TIER ------------------------------------------------------------
	ggml_backend_dev_t PinnedDevice = nullptr;
	FString DeviceLabel;
	FString TierReason;
	const ESiegeLlamaOffloadTier Tier = ChooseTier(PinnedDevice, DeviceLabel, TierReason);
	TierValue.Set(static_cast<int32>(Tier));

	int32 GpuLayers = 0;
	switch (Tier)
	{
		case ESiegeLlamaOffloadTier::FullOffload: GpuLayers = -1; break;
		case ESiegeLlamaOffloadTier::Partial:     GpuLayers = PartialGpuLayers; break;
		default:                                  GpuLayers = 0; break;
	}

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s tier=%s gpu_layers=%d device=%s reason: %s"),
		TagLoad, TierName(Tier), GpuLayers, *DeviceLabel, *TierReason);

	if (Tier == ESiegeLlamaOffloadTier::CpuOnly)
	{
		// MEASURED, not feared: TASK-413's cpu tier aborted 2 of 5 generations at
		// the 10 s ceiling and returned truncated JSON. Saying so is the honest
		// form of "CPU fallback is required".
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: running on the CPU tier. This tier MEASURED 2 of 5 generations aborting at the %.1fs ceiling with TRUNCATED JSON (TASK-413 bar #2, mean wall 8730 ms). The assistant will work and will sometimes fail to answer in time; that is a known property of this tier, not a new bug."),
			TagLoad, USiegeLlamaSubsystem::HardTimeoutSeconds);
	}

	// --- 2. MODEL -----------------------------------------------------------
	llama_model_params ModelParams = llama_model_default_params();
	ModelParams.n_gpu_layers = GpuLayers;

	// ONE DEVICE, NAMED. main_gpu indexes THIS list, so with a one-entry list it
	// is unambiguously 0 -- no assumption about where ggml's own filtered
	// ordering would have placed the card. The array must outlive the load call
	// and does: llama copies it during llama_model_load_from_file and this
	// function does not return first.
	ggml_backend_dev_t PinnedDeviceList[2] = { PinnedDevice, nullptr };
	if (PinnedDevice != nullptr && Tier != ESiegeLlamaOffloadTier::CpuOnly)
	{
		ModelParams.devices = PinnedDeviceList;
		ModelParams.split_mode = LLAMA_SPLIT_MODE_NONE;
		ModelParams.main_gpu = 0;
	}

	// MEMORY-MAPPED LOADING IS LOAD-BEARING FOR THE 8 GB CLAIM -- the weights stay
	// page-cache-backed rather than resident.
	//
	// THE FIELD NAME CHANGED AND THE REQUIREMENT DID NOT. CONVENTIONS section 7
	// and the original spec both said `use_mmap = true`. In the vendored build
	// that bool is GONE; `load_mode` replaced it and LLAMA_LOAD_MODE_MMAP
	// (llama.h:207) is its exact successor. Verified against the vendored header
	// rather than taken from the spec: a grep for use_mmap across
	// Source/ThirdParty/LlamaCpp/include returns nothing.
	ModelParams.load_mode = LLAMA_LOAD_MODE_MMAP;

	// Shutdown must not wait for 2.5 GB. Returning false from the progress
	// callback aborts the load outright (llama.h:323-325).
	ModelParams.progress_callback = &SiegeLlamaPrivate::LoadProgressThunk;
	ModelParams.progress_callback_user_data = this;

	Model = llama_model_load_from_file(TCHAR_TO_UTF8(*ModelPath), ModelParams);
	if (Model == nullptr)
	{
		if (bStopRequested)
		{
			UE_LOG(LogSiegeLlama, Log, TEXT("%s: model load aborted by shutdown. Nothing is faulted."), TagLoad);
			State.Set(static_cast<int32>(EState::Stopped));
			return;
		}
		LatchFaulted(FString::Printf(TEXT("llama_model_load_from_file returned NULL for '%s'"), *ModelPath));
		return;
	}

	ModelLayerCount = llama_model_n_layer(Model);
	ModelSizeBytes = llama_model_size(Model);
	Vocab = llama_model_get_vocab(Model);

	if (Tier == ESiegeLlamaOffloadTier::Partial && ModelLayerCount != MeasuredModelLayers)
	{
		// The 18 is 18-of-36 and nothing else. On a different layer count it is
		// an arbitrary fraction and the partial-tier measurements do not describe
		// this model.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: the partial tier offloads %d layers, which was measured as %d of %d. This model has %d layers, so the measured partial-tier VRAM and latency figures DO NOT describe it -- re-measure before quoting them."),
			TagLoad, PartialGpuLayers, PartialGpuLayers, MeasuredModelLayers, ModelLayerCount);
	}

	// --- 3. CONTEXT ---------------------------------------------------------
	llama_context_params ContextParams = llama_context_default_params();
	ContextParams.n_ctx = static_cast<uint32>(USiegeLlamaSubsystem::ContextTokens);
	ContextParams.n_batch = static_cast<uint32>(FMath::Max(USiegeLlamaSubsystem::BatchTokens, USiegeLlamaSubsystem::UBatchTokens));
	ContextParams.n_ubatch = static_cast<uint32>(USiegeLlamaSubsystem::UBatchTokens);
	ContextParams.n_threads = DefaultThreadCount();
	ContextParams.n_threads_batch = ContextParams.n_threads;
	ContextParams.abort_callback = &SiegeLlamaPrivate::AbortThunk;
	ContextParams.abort_callback_data = this;
	ContextParams.no_perf = true;

	Context = llama_init_from_model(Model, ContextParams);
	if (Context == nullptr)
	{
		LatchFaulted(TEXT("llama_init_from_model returned NULL"));
		return;
	}

	// --- 4. THE ACTUALS, QUERIED -- NEVER THE REQUEST -----------------------
	// llama.h:551-556: "it is recommended to query the actual values using these
	// functions ... the requested values via llama_context_params may differ from
	// the actual values used by the context."
	//
	// WHY THIS IS NOT A NICETY: if n_batch came back clamped below the request,
	// a prefill slice sized from the request is larger than the context accepts,
	// llama_decode rejects it outright, and EVERY run reads "inference failed" --
	// a total-failure mode that presents as a model problem while the .ini value
	// that caused it looks perfectly correct. A clamped n_ctx does the mirror
	// image to the budget assertion below.
	//
	// Requested and actual are printed SIDE BY SIDE, once, so a clamp is VISIBLE
	// rather than inferred.
	EffectiveBatchSize = static_cast<int32>(llama_n_batch(Context));
	const int32 ActualCtx = static_cast<int32>(llama_n_ctx(Context));
	const int32 ActualUBatch = static_cast<int32>(llama_n_ubatch(Context));
	const int32 ActualThreads = llama_n_threads(Context);
	ActualContextSize.Set(ActualCtx);

	const double LoadMs = (FPlatformTime::Seconds() - LoadStart) * 1000.0;

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s LOADED in %.0f ms. model='%s' size=%.1f MiB layers=%d tier=%s\n")
		TEXT("  n_ctx    requested=%d actual=%d%s\n")
		TEXT("  n_batch  requested=%d actual=%d%s\n")
		TEXT("  n_ubatch requested=%d actual=%d%s\n")
		TEXT("  n_threads actual=%d   deadline=%.1fs %s"),
		TagLoad, LoadMs, *ModelPath,
		static_cast<double>(ModelSizeBytes) / static_cast<double>(BytesPerMiB),
		ModelLayerCount, TierName(Tier),
		USiegeLlamaSubsystem::ContextTokens, ActualCtx,
		(ActualCtx != USiegeLlamaSubsystem::ContextTokens) ? TEXT("  <== CLAMPED") : TEXT(""),
		FMath::Max(USiegeLlamaSubsystem::BatchTokens, USiegeLlamaSubsystem::UBatchTokens), EffectiveBatchSize,
		(EffectiveBatchSize != FMath::Max(USiegeLlamaSubsystem::BatchTokens, USiegeLlamaSubsystem::UBatchTokens)) ? TEXT("  <== CLAMPED") : TEXT(""),
		USiegeLlamaSubsystem::UBatchTokens, ActualUBatch,
		(ActualUBatch != USiegeLlamaSubsystem::UBatchTokens) ? TEXT("  <== CLAMPED") : TEXT(""),
		ActualThreads,
		USiegeLlamaSubsystem::HardTimeoutSeconds, AbortRegime(Tier));

	// --- 5. THE BUDGET ASSERTION -------------------------------------------
	RunBudgetAssertion();
}

/**
 *  THE MANDATORY STARTUP BUDGET ASSERTION (CONVENTIONS section 8, ZONE-A SIZE
 *  BOUND, ruling 2):
 *
 *      ZoneA_tokens + MaxSnapshotTokens + MaxOutputTokens + safety_margin
 *          <= n_ctx_actual
 *
 *  EVERY OPERAND THAT DESCRIBES THE WORLD IS READ FROM THE LIVE SYSTEM:
 *  ZoneA_tokens from llama_tokenize on the REGISTERED prefix string, n_ctx from
 *  llama_n_ctx(ctx). The three budgets are correctly constants. The line that
 *  decides which is which: A BUDGET MAY BE A CONSTANT, A MEASUREMENT MAY NOT.
 *
 *  A ZoneA_tokens implemented as the recorded 1139 would be an operand that
 *  CANNOT GROW, so the assertion would pass forever -- and pass most confidently
 *  at the exact moment a vocabulary pass pushed Zone A past the ceiling. A
 *  guardrail that cannot observe the thing it guards is worse than none, because
 *  it reports safe.
 *
 *  IT RUNS WHENEVER BOTH OPERANDS EXIST, and re-runs on every prefix change --
 *  which is precisely the growth path section 9a sanctions.
 */
void FSiegeLlamaWorker::RunBudgetAssertion()
{
	using namespace SiegeLlamaPrivate;

	const int32 ActualCtx = ActualContextSize.GetValue();
	if (ActualCtx <= 0 || Context == nullptr)
	{
		return;
	}

	const int32 PrefixTokens = StaticPrefixTokens.GetValue();
	const bool bPrefixKnown = !AppliedStaticPrefix.IsEmpty();

	const int32 Total = PrefixTokens
		+ USiegeLlamaSubsystem::MaxSnapshotTokens
		+ USiegeLlamaSubsystem::MaxOutputTokens
		+ USiegeLlamaSubsystem::SafetyMarginTokens;

	if (!bPrefixKnown)
	{
		// STATE THE INCOMPLETENESS RATHER THAN LET IT LOOK LIKE A PASS. The
		// context-side half is a real check and it runs; the ZoneA_tokens half
		// cannot be evaluated because nothing has registered a prefix.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s ASSERTION IS INCOMPLETE -- NO STATIC PREFIX HAS BEEN REGISTERED, so ZoneA_tokens is UNMEASURED and is being treated as 0. What IS checked: MaxSnapshotTokens=%d + MaxOutputTokens=%d + SafetyMarginTokens=%d = %d vs n_ctx_actual=%d. This is NOT the section 8 guardrail; call SetStaticPrefix with the assembled Zone A to complete it."),
			TagBudget,
			USiegeLlamaSubsystem::MaxSnapshotTokens, USiegeLlamaSubsystem::MaxOutputTokens,
			USiegeLlamaSubsystem::SafetyMarginTokens, Total, ActualCtx);
	}

	if (Total > ActualCtx)
	{
		// LOUD, AT LOAD, NAMING WHICH OPERAND OVERFLOWED -- never at the first
		// sentence of a match and never as a silent truncation. Both of those
		// failure shapes point the investigator at the wrong component: a
		// llama_decode rejection reads as "inference failed" and a silent cut
		// reads as "the model is bad".
		const TCHAR* Culprit = (PrefixTokens > ActualCtx / 2)
			? TEXT("ZoneA_tokens -- the static prefix is the operand that overflowed; a vocabulary or few-shot pass grew it past the context")
			: TEXT("the fixed budgets -- ZoneA_tokens is small, so n_ctx_actual itself is too small for MaxSnapshotTokens + MaxOutputTokens + SafetyMarginTokens");

		LatchFaulted(FString::Printf(
			TEXT("PROMPT BUDGET OVERFLOW at load. ZoneA_tokens=%d + MaxSnapshotTokens=%d + MaxOutputTokens=%d + SafetyMarginTokens=%d = %d, against n_ctx_actual=%d (requested %d). OVERFLOWED BY %d. Culprit: %s"),
			PrefixTokens, USiegeLlamaSubsystem::MaxSnapshotTokens,
			USiegeLlamaSubsystem::MaxOutputTokens, USiegeLlamaSubsystem::SafetyMarginTokens,
			Total, ActualCtx, USiegeLlamaSubsystem::ContextTokens, Total - ActualCtx, Culprit));
		return;
	}

	if (bPrefixKnown)
	{
		UE_LOG(LogSiegeLlama, Display,
			TEXT("%s ASSERTION PASSED. ZoneA_tokens=%d (TOKENIZED from the registered %d-char prefix at this instant, never a recorded constant) + MaxSnapshotTokens=%d + MaxOutputTokens=%d + SafetyMarginTokens=%d = %d <= n_ctx_actual=%d (requested %d). Slack=%d."),
			TagBudget, PrefixTokens, AppliedStaticPrefix.Len(),
			USiegeLlamaSubsystem::MaxSnapshotTokens, USiegeLlamaSubsystem::MaxOutputTokens,
			USiegeLlamaSubsystem::SafetyMarginTokens, Total, ActualCtx,
			USiegeLlamaSubsystem::ContextTokens, ActualCtx - Total);
	}
}

void FSiegeLlamaWorker::UnloadModel()
{
	if (Context != nullptr)
	{
		llama_free(Context);
		Context = nullptr;
	}
	if (Model != nullptr)
	{
		llama_model_free(Model);
		Model = nullptr;
	}
	Vocab = nullptr;
	LastPromptTokens.Reset();
	StaticPrefixTokenArray.Reset();
}

// ===========================================================================
// 5. THE STATIC PREFIX
// ===========================================================================
//
// THE DECLARED DEVIATION, AND THE ARGUMENT FOR IT, IN ONE PLACE SO IT CAN BE
// RULED ON RATHER THAN DISCOVERED.
//
// CONVENTIONS section 8 states BOTH of these, and they are in tension:
//
//   (a) "The entire API surface between the two lanes is (prompt, gbnf) ->
//       string." The plugin may not include a game-lane header or name a
//       Siegebound type.
//   (b) The ZONE-A SIZE BOUND ruling makes a startup budget assertion MANDATORY
//       and requires ZoneA_tokens to be TOKENIZED AT LOAD FROM THE ASSEMBLED
//       ZONE-A STRING, never a recorded constant -- with the explicit reason
//       that a constant cannot grow when section 9a widens the place vocabulary.
//
// The plugin cannot build Zone A: it is USiegeAssistantSnapshot::BuildZoneA, in
// the game module, behind the very boundary (a) draws. So under (a) alone, (b)
// is unimplementable in its specified form, and the only remaining options are a
// baked constant -- which (b) forbids by name -- or no assertion at all.
//
// The resolution taken here is the smallest one that satisfies both: the Zone-A
// STRING crosses the boundary, as an opaque FString, exactly the way Prompt and
// Grammar already do. No game-lane include. No Siegebound type. This file still
// cannot name a unit kind, a place, an intent or a snapshot.
//
// IT IS OPTIONAL, AND WHAT IT COSTS WHEN IT IS ABSENT IS STATED OUT LOUD ONCE
// PER SESSION rather than left to look like a pass -- see the warning in
// RunRequestUnguarded.
//
// AND IT IS CHECKED, NOT TRUSTED: every request verifies that the prompt
// actually starts with the registered prefix, case-sensitively. A game lane that
// drifts gets a log line instead of a silently mis-attributed token split.

void FSiegeLlamaWorker::ApplyPendingStaticPrefix()
{
	FString NewPrefix;
	{
		FScopeLock Lock(&RequestLock);
		if (!bStaticPrefixDirty)
		{
			return;
		}
		bStaticPrefixDirty = false;
		NewPrefix = PendingStaticPrefix;
	}

	if (Context == nullptr || Vocab == nullptr)
	{
		// Nothing to tokenize with yet. The flag is consumed; the game lane
		// re-registers if it cares, and the load path re-runs the assertion.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: a static prefix was registered before the model finished loading and was DISCARDED. Register it after IsReady() goes true."),
			SiegeLlamaPrivate::TagBudget);
		return;
	}

	TArray<llama_token> Tokens;
	if (!Tokenize(NewPrefix, /*bAddSpecial=*/true, Tokens))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: the registered static prefix (%d chars) FAILED TO TOKENIZE. ZoneA_tokens stays unmeasured and the budget assertion stays incomplete."),
			SiegeLlamaPrivate::TagBudget, NewPrefix.Len());
		return;
	}

	StaticPrefixTokenArray = MoveTemp(Tokens);
	StaticPrefixTokens.Set(StaticPrefixTokenArray.Num());

	{
		FScopeLock Lock(&RequestLock);
		AppliedStaticPrefix = NewPrefix;
	}

	// THE SHIPPED LANE'S OWN TOKEN COUNT, MEASURED. Recorded because CONVENTIONS
	// section 12g carries a standing WARN that NO COMMAND PRINTS THE SHIPPED
	// BuildZoneA -- every zoneA_tok figure on the record came from the spike's
	// frozen copy. This line is the shipped lane's first measured reading.
	//
	// IT IS A COUNT OF THE PREFIX TOKENIZED STANDALONE. Section 12g's own
	// finding is that a small edit is dominated by BOUNDARY RE-TOKENISATION of
	// text it did not touch, so the prefix's contribution INSIDE the full prompt
	// can differ from this by a token or two at the seam. That is what
	// SafetyMarginTokens is sized to absorb; it is not a defect, and it is stated
	// so the number is not quoted as exact.
	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s STATIC PREFIX REGISTERED: %d chars -> %d tokens (MEASURED on the SHIPPED string, at this instant, with add_special=true). Re-running the budget assertion."),
		SiegeLlamaPrivate::TagBudget, NewPrefix.Len(), StaticPrefixTokenArray.Num());

	RunBudgetAssertion();
}

// ===========================================================================
// 6. TOKENIZATION
// ===========================================================================

bool FSiegeLlamaWorker::Tokenize(const FString& Text, bool bAddSpecial, TArray<llama_token>& OutTokens) const
{
	OutTokens.Reset();

	if (Vocab == nullptr)
	{
		return false;
	}

	const FTCHARToUTF8 Utf8(*Text);

	// With n_tokens_max = 0 llama_tokenize returns the NEGATED required count.
	const int32 Probe = llama_tokenize(Vocab, Utf8.Get(), Utf8.Length(), nullptr, 0, bAddSpecial, true);
	if (Probe == INT32_MIN)
	{
		return false;
	}

	const int32 Needed = -Probe;
	if (Needed < 0)
	{
		return false;
	}
	if (Needed == 0)
	{
		return true;
	}

	OutTokens.SetNumUninitialized(Needed);
	const int32 Written = llama_tokenize(Vocab, Utf8.Get(), Utf8.Length(),
		OutTokens.GetData(), Needed, bAddSpecial, true);
	if (Written < 0)
	{
		OutTokens.Reset();
		return false;
	}

	OutTokens.SetNum(Written, EAllowShrinking::No);
	return true;
}

// ===========================================================================
// 7. GENERATION
// ===========================================================================

namespace SiegeLlamaPrivate
{
	static void RequestThunk(void* Context)
	{
		static_cast<FSiegeLlamaWorker*>(Context)->InvokeRequestUnguarded();
	}
}

void FSiegeLlamaWorker::RunRequestGuarded()
{
	uint32 ExceptionCode = 0;
	if (!SiegeLlamaPrivate::SehInvoke(&SiegeLlamaPrivate::RequestThunk, this, ExceptionCode))
	{
		// The cache is in an unknown state and the context may be corrupt. Latch
		// and stop offering inference -- the delegate still fires, on the game
		// thread, so the FSM is never left waiting on a call that died.
		LastPromptTokens.Reset();
		const FString Reason = FString::Printf(
			TEXT("a generation FAULTED (SEH code 0x%08X). The job was abandoned and the game was not taken down with it"),
			ExceptionCode);
		LatchFaulted(Reason);
		PostCompletion(false, FString(), Reason, false);
	}
}

void FSiegeLlamaWorker::RunRequestUnguarded()
{
	using namespace SiegeLlamaPrivate;

	FString Prompt;
	FString Grammar;
	FString Prefix;
	{
		FScopeLock Lock(&RequestLock);
		Prompt = MoveTemp(PendingPrompt);
		Grammar = MoveTemp(PendingGrammar);
		PendingPrompt.Reset();
		PendingGrammar.Reset();
		Prefix = AppliedStaticPrefix;
	}

	const ESiegeLlamaOffloadTier Tier = GetTier();
	const double StartSeconds = FPlatformTime::Seconds();
	const int32 ActualCtx = ActualContextSize.GetValue();

	if (Context == nullptr || Vocab == nullptr)
	{
		PostCompletion(false, FString(), TEXT("no model is resident"), false);
		return;
	}

	// --- 1. TOKENIZE --------------------------------------------------------
	TArray<llama_token> PromptTokens;
	if (!Tokenize(Prompt, /*bAddSpecial=*/true, PromptTokens) || PromptTokens.Num() == 0)
	{
		PostCompletion(false, FString(), TEXT("tokenize failed -- llama_decode was never called, so there is no return code"), false);
		return;
	}
	// Captured now: PromptTokens is moved into LastPromptTokens on success, and
	// deriving this from the decode cursor afterwards is off by one whenever the
	// loop breaks between the two increments.
	const int32 PromptTokenCount = PromptTokens.Num();

	// --- 2. THE SNAPSHOT BUDGET, ENFORCED BY TOKENIZING ---------------------
	//
	// THE OBSERVABLE-TRUNCATION CONDITION binds ANY enforcement of the snapshot
	// budget, by ANY mechanism, under ANY name: if the snapshot reaches the model
	// shorter than it was built, something says so.
	//
	// THIS SUBSYSTEM SATISFIES IT BY NEVER TRUNCATING. Both cut sites REJECT the
	// request instead, loudly, and the caller's completion carries the reason.
	// That is a DELIBERATE choice over silent-or-logged truncation, and the
	// reason is specific to this feature rather than general tidiness: the
	// snapshot's LAST line is the player's own sentence, so a tail truncation
	// removes the utterance and leaves a well-formed prompt about nothing. The
	// model would then emit a syntactically perfect, confidently wrong command --
	// and this assistant ORDERS UNITS. A refusal the FSM can say out loud is
	// strictly better than a valid-shaped wrong command, which is the exact
	// failure CONVENTIONS section 1 is built around.
	//
	// Rejecting is also strictly stronger than the condition asks for: nothing
	// ever reaches the model shorter than it was built, because nothing reaches
	// it at all.
	const bool bPrefixKnown = !Prefix.IsEmpty();
	bool bPrefixMatches = false;

	if (bPrefixKnown)
	{
		// CHECKED, NOT TRUSTED. Case-SENSITIVE: a case-insensitive match here
		// would accept a prefix whose casing had drifted and then mis-attribute
		// every token in the split.
		bPrefixMatches = Prompt.StartsWith(Prefix, ESearchCase::CaseSensitive);
		if (!bPrefixMatches)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: the prompt does NOT begin with the registered %d-char static prefix. The snapshot budget cannot be attributed for this request and is NOT enforced on it. Either the prefix was registered from a different builder than the one that assembled this prompt, or Zone A stopped being byte-stable -- both destroy the KV prefix as well."),
				TagBudget, Prefix.Len());
		}
	}
	else if (!bWarnedNoStaticPrefix)
	{
		bWarnedNoStaticPrefix = true;
		// ONCE PER SESSION, AT WARNING. A guardrail that cannot observe what it
		// guards must SAY it cannot, or its silence reads as a pass.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: NO STATIC PREFIX IS REGISTERED, so the snapshot region cannot be separated from the prompt and NEITHER SNAPSHOT BUDGET IS ENFORCED -- not MaxSnapshotTokens=%d and not SnapshotPreFilterMaxChars=%d. The CONTEXT budget below is still enforced on every request, so there is no unguarded path to llama_decode; but section 8's B+C budget is inert until SetStaticPrefix is called. Logged ONCE."),
			TagBudget, USiegeLlamaSubsystem::MaxSnapshotTokens, USiegeLlamaSubsystem::SnapshotPreFilterMaxChars);
	}

	if (bPrefixKnown && bPrefixMatches)
	{
		const int32 SnapshotTokens = PromptTokens.Num() - StaticPrefixTokens.GetValue();
		if (SnapshotTokens > USiegeLlamaSubsystem::MaxSnapshotTokens)
		{
			const FString Error = FString::Printf(
				TEXT("snapshot budget exceeded: %d tokens against MaxSnapshotTokens=%d (prompt %d tok, static prefix %d tok, %d chars of snapshot text)"),
				SnapshotTokens, USiegeLlamaSubsystem::MaxSnapshotTokens,
				PromptTokens.Num(), StaticPrefixTokens.GetValue(), Prompt.Len() - Prefix.Len());

			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s AUTHORITY CUT SITE FIRED -- REQUEST REJECTED, NOT TRUNCATED. %s. The snapshot was MEASURED with the tokenizer, not estimated from characters. This firing is a bug report about Zone C's shape, not a signal to raise the budget."),
				TagBudget, *Error);

			PostCompletion(false, FString(), Error, false);
			return;
		}
	}

	// --- 3. THE CONTEXT BUDGET, ALWAYS ENFORCED -----------------------------
	// Against the EFFECTIVE n_ctx, never the requested one. Caught here rather
	// than as an opaque llama_decode return code: a prompt that does not fit is a
	// BUDGET finding and must read as one.
	if (PromptTokens.Num() + USiegeLlamaSubsystem::MaxOutputTokens > ActualCtx)
	{
		const FString Error = FString::Printf(
			TEXT("prompt is %d tokens and the output budget is %d, which exceeds the context's ACTUAL n_ctx=%d (requested %d). This is a budget failure, not a model failure -- llama_decode was never called"),
			PromptTokens.Num(), USiegeLlamaSubsystem::MaxOutputTokens, ActualCtx, USiegeLlamaSubsystem::ContextTokens);
		UE_LOG(LogSiegeLlama, Error, TEXT("%s: %s"), TagBudget, *Error);
		PostCompletion(false, FString(), Error, false);
		return;
	}

	// --- 4. KV PREFIX REUSE -------------------------------------------------
	// The longest common TOKEN prefix with the previous turn. Zone A and Zone B
	// are byte-identical between turns by construction, so the reuse point should
	// land at the start of Zone C. If it does not, the LAYOUT is wrong -- the
	// symptom is latency, not a wrong answer, which is why it needs a log line to
	// be visible at all.
	int32 CommonPrefix = 0;
	while (CommonPrefix < PromptTokens.Num()
		&& CommonPrefix < LastPromptTokens.Num()
		&& PromptTokens[CommonPrefix] == LastPromptTokens[CommonPrefix])
	{
		++CommonPrefix;
	}

	// At least one token must be decoded, or there are no logits to sample from.
	CommonPrefix = FMath::Clamp(CommonPrefix, 0, PromptTokens.Num() - 1);

	llama_memory_t Memory = llama_get_memory(Context);

	// THE RETURN VALUE IS CHECKED, AND THAT IS NOT DEFENSIVE PADDING.
	// llama_memory_seq_rm is documented to return false when a PARTIAL sequence
	// cannot be removed (llama.h:730-739; SWA and recurrent memories are the
	// cases). Ignoring it leaves the cache holding tokens this turn's positions
	// are about to overwrite, and the resulting garbage surfaces as a BAD ANSWER
	// with no log line naming the cause -- while the reuse figure happily reports
	// a large, entirely fictional prefix. Falling back to a full clear costs one
	// slow turn and keeps both true.
	//
	// THE SYMBOL: llama_memory_seq_rm (llama.h:735). llama_kv_cache_seq_rm --
	// which several documents still name -- does not exist anywhere in the
	// vendored header. Verified by grep, not taken from the spec.
	if (CommonPrefix > 0 && !llama_memory_seq_rm(Memory, 0, CommonPrefix, -1))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: llama_memory_seq_rm refused a partial removal at %d -- clearing the whole cache and re-prefilling. This turn's KV-reuse figure is 0 BY CONSTRUCTION, not by measurement."),
			TagGen, CommonPrefix);
		llama_memory_clear(Memory, true);
		CommonPrefix = 0;
	}
	else if (CommonPrefix == 0)
	{
		llama_memory_seq_rm(Memory, 0, 0, -1);
	}

	// --- 5. ARM THE DEADLINE ------------------------------------------------
	//
	// ONE DEADLINE, TWO REPRESENTATIONS, AND THEY MOVE TOGETHER OR NOT AT ALL:
	//   - HardDeadlineSeconds, a worker-local double, read by the between-slice
	//     and per-token tests in the two loops below;
	//   - AbortDeadlineUsec, an atomic, read by AbortThunk on ggml's compute
	//     threads via ShouldAbortNow().
	// Both are computed from StartSeconds + HardTimeoutSeconds on the two lines
	// that follow, so there is exactly one place either can drift from the other,
	// and it is visible in one glance. The atomic is DISARMED (set to 0) on every
	// exit path below.
	//
	// WHY BOTH EXIST RATHER THAN JUST THE ATOMIC: the atomic reaches inside
	// llama_decode and is therefore the only mechanism that can cut mid-graph --
	// but llama.h:382-385 documents it as working "only with CPU execution", so
	// on the offload tiers it enforces nothing. The loop tests are what make the
	// ceiling tier-independent. Neither is a substitute for the other.
	const double SoftDeadlineSeconds = StartSeconds + USiegeLlamaSubsystem::SoftTimeoutSeconds;
	const double HardDeadlineSeconds = StartSeconds + USiegeLlamaSubsystem::HardTimeoutSeconds;
	FPlatformAtomics::InterlockedExchange(&AbortDeadlineUsec, static_cast<int64>(HardDeadlineSeconds * 1000000.0));

	bool bSoftTimeoutExceeded = false;
	bool bAborted = false;
	bool bCancelled = false;
	bool bDecodeFailed = false;
	FString FailureDetail;

	// --- 6. PREFILL, IN n_batch SLICES --------------------------------------
	// llama_decode rejects a batch larger than n_batch outright, and a real
	// prompt here is ~1500 tokens against a 512-token n_batch -- a single-batch
	// prefill would fail on the very first call. The slicing is also what the
	// small n_ubatch's contention mitigation rests on, and it is what bounds how
	// coarse a cancel is on the GPU tiers.
	const double PrefillStart = FPlatformTime::Seconds();
	{
		int32 Offset = CommonPrefix;
		while (Offset < PromptTokens.Num())
		{
			// CHECKED BETWEEN SLICES AS WELL AS INSIDE llama_decode. The
			// abort_callback is authoritative only on the cpu tier; this loop is
			// what bounds a cancel on the GPU tiers.
			//
			// GRANULARITY, CORRECTED 2026-08-03: this test runs once per
			// llama_decode CALL, and a prefill call carries up to
			// EffectiveBatchSize = llama_n_batch(ctx) tokens (512 requested),
			// which llama splits into n_ubatch graphs internally WITHOUT
			// returning here. So the bound is ONE n_BATCH SLICE, not "at most one
			// ubatch" as this comment and handoffs/TASK-450-programmer.md both
			// claimed -- the loop cannot observe a boundary it never returns
			// through. The decode loop below is the per-TOKEN one.
			if (bCancelRequested || bStopRequested)
			{
				bCancelled = true;
				FailureDetail = FString::Printf(
					TEXT("CANCELLED during PREFILL at prompt offset %d of %d (%s)"),
					Offset, PromptTokens.Num(), AbortRegime(Tier));
				break;
			}

			const int32 ChunkTokens = FMath::Min(FMath::Max(1, EffectiveBatchSize), PromptTokens.Num() - Offset);
			llama_batch Batch = llama_batch_get_one(PromptTokens.GetData() + Offset, ChunkTokens);
			const int32 DecodeResult = llama_decode(Context, Batch);
			if (DecodeResult != 0)
			{
				bDecodeFailed = true;
				// llama.h:971-975 enumerates the codes: 2 = aborted, 1 = no KV
				// slot, -1 = invalid batch, < -1 = fatal. Only 2 is our callback.
				bAborted = (DecodeResult == 2);
				bCancelled = bAborted && bCancelRequested;
				FailureDetail = FString::Printf(
					TEXT("PREFILL llama_decode=%d at prompt offset %d (chunk %d, n_batch %d)%s"),
					DecodeResult, Offset, ChunkTokens, EffectiveBatchSize,
					(DecodeResult == 2)
						? TEXT(" -- code 2 is ABORTED: abort_callback fired INSIDE the prefill, i.e. a cancel, a shutdown OR the hard deadline landing mid-graph. All three feed ShouldAbortNow(), so the code alone does not say which; bCancelled below is what distinguishes a cancel from the other two")
						: TEXT(""));
				break;
			}

			Offset += ChunkTokens;

			// The cpu tier's prefill alone can outlast 4 s, so the soft timeout
			// is watched HERE too, not only in the decode loop.
			CheckSoftTimeout(SoftDeadlineSeconds, 0, bSoftTimeoutExceeded);

			// THE HARD DEADLINE AS A REAL BETWEEN-SLICE TERM. THIS IS THE LINE
			// THAT MAKES THE CEILING A BACKSTOP ON EVERY TIER RATHER THAN ONLY
			// WHERE abort_callback HAPPENS TO WORK (added 2026-08-03,
			// qa/TASK-424.md WARN-2).
			//
			// THE GAP IT CLOSES, STATED PRECISELY BECAUSE THE HEADER ASSERTED THE
			// FIX BEFORE IT EXISTED: the test above covers cancel and stop only.
			// The deadline reached the prefill by exactly one route --
			// AbortThunk -> ShouldAbortNow -> AbortDeadlineUsec -- and llama.h:384
			// says that route "currently works only with CPU execution". So on
			// tier=partial and tier=full a runaway prefill had NO 10 s
			// enforcement whatsoever until the decode loop's own check, which is
			// on the far side of it. A header promising a backstop the code did
			// not provide is worse than no promise: it is written in the place a
			// reviewer goes to check.
			//
			// AT THE BOTTOM OF THE BODY, NOT THE TOP, AND THE PLACEMENT IS THE
			// WHOLE POINT. From turn two the prefill is ~347 tokens against a
			// 512-token n_batch (CONVENTIONS section 8, DROP = 77.1 %), so this
			// loop runs its body ONCE and a top-of-body test would never fire on
			// the hot path -- the deadline would first be consulted after a token
			// had already been sampled. Testing after every slice covers the LAST
			// slice, which is the only slice on the common path.
			//
			// A SEPARATE BRANCH RATHER THAN AN EXTRA TERM IN THE TEST ABOVE, AND
			// THE REASON IS THE CALLER'S: a timeout is not a cancel. Folding it in
			// would set bCancelled, and the completion would then read
			// "cancelled -- ..." for a run nobody cancelled, so the FSM would tell
			// the player their order was cancelled when it in fact ran out of
			// time. Same exit, different fact, and the FSM says them differently.
			if (FPlatformTime::Seconds() > HardDeadlineSeconds)
			{
				// bAborted, NOT bDecodeFailed: llama_decode returned 0 every time
				// it was called. Nothing failed -- the LOOP stopped. This is the
				// same flag and the same distinction the decode loop's hard
				// timeout already uses.
				bAborted = true;
				// THE "elapsed" FIELD IS MEASURED, NOT THE CEILING RESTATED
				// (qa/TASK-446.md WARN-3, fixed 2026-08-03). Until this line, the
				// argument was HardTimeoutSeconds itself, so the field ALWAYS read
				// "10.0s" whatever actually happened -- and the OVERSHOOT, which is
				// the one quantity the header promises is bounded and the only
				// quantity this line can ever carry, was invisible.
				//
				// StartSeconds, NOT PrefillStart, AND THE BASE IS THE WHOLE POINT:
				// the deadline is armed as StartSeconds + HardTimeoutSeconds (see
				// ARM THE DEADLINE above), so this subtraction shares the deadline's
				// origin and (printed - 10.0) IS the overshoot, directly. Rebasing
				// it on PrefillStart would print a number that cannot be compared
				// with the ceiling at all.
				FailureDetail = FString::Printf(
					TEXT("HARD TIMEOUT during PREFILL: %.1fs elapsed after %d of %d prompt tokens, BEFORE any output token was sampled. llama_decode never returned an error -- the LOOP stopped, so this is the ceiling cutting a slow run, not a generation failure. Regime: %s"),
					FPlatformTime::Seconds() - StartSeconds, Offset, PromptTokens.Num(), AbortRegime(Tier));
				break;
			}
		}
	}
	const double PrefillMs = (FPlatformTime::Seconds() - PrefillStart) * 1000.0;

	// bAborted IS A NEW TERM HERE AND IT IS PROVABLY INERT ON EVERY PRE-EXISTING
	// PATH (added 2026-08-03 with the between-slice deadline above). Until that
	// line existed, bAborted was set in exactly one place in this loop -- the
	// llama_decode != 0 branch -- and always alongside bDecodeFailed, so it could
	// never be the term that decided this test. The prefill hard timeout is the
	// first exit that sets it ALONE.
	//
	// WITHOUT IT THE NEW EXIT WOULD FALL THROUGH INTO THE SAMPLER, AND THAT IS
	// THE DANGEROUS DIRECTION, NOT MERELY AN UNTIDY ONE: the prompt is only
	// partially prefilled at that point, so llama_sampler_sample would sample
	// from the logits of the LAST SLICE THAT LANDED and generate a fluent
	// continuation of a truncated prompt. This assistant ORDERS UNITS. A
	// syntactically perfect command derived from half a prompt is exactly the
	// valid-shaped wrong answer section 1 of CONVENTIONS is built around, and it
	// is strictly worse than the refusal the caller gets instead.
	if (bCancelled || bDecodeFailed || bAborted)
	{
		// The cache is in an unknown state, so the next turn must not claim a
		// prefix it cannot prove.
		LastPromptTokens.Reset();
		FPlatformAtomics::InterlockedExchange(&AbortDeadlineUsec, 0);

		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: %s"), TagGen, *FailureDetail);
		PostCompletion(false, FString(),
			bCancelled ? FString::Printf(TEXT("cancelled -- %s"), *FailureDetail) : FailureDetail,
			false);
		return;
	}

	// --- 7. SAMPLER ---------------------------------------------------------
	// GREEDY, on purpose: an accuracy figure taken with a stochastic sampler is
	// not reproducible, and a number that moves between runs is not a number.
	llama_sampler* Chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
	bool bGrammarActive = false;

	if (!Grammar.IsEmpty())
	{
		const FTCHARToUTF8 GrammarUtf8(*Grammar);
		llama_sampler* GrammarSampler = llama_sampler_init_grammar(Vocab, GrammarUtf8.Get(), "root");
		if (GrammarSampler == nullptr)
		{
			// NOT A WARNING BURIED IN A LINE: an unparsed GBNF means generation
			// is UNCONSTRAINED, and an unconstrained result must never be read as
			// a constrained one. TASK-413 lost six bench runs to exactly this.
			UE_LOG(LogSiegeLlama, Error,
				TEXT("%s: llama_sampler_init_grammar returned NULL -- THE GBNF FAILED TO PARSE (%d chars). This request is being answered UNCONSTRAINED; nothing about its output may be reported as a constrained result."),
				TagGen, Grammar.Len());
		}
		else
		{
			llama_sampler_chain_add(Chain, GrammarSampler);
			bGrammarActive = true;
		}
	}
	llama_sampler_chain_add(Chain, llama_sampler_init_greedy());

	// --- 8. DECODE ----------------------------------------------------------
	// Bytes are accumulated RAW and converted once at the end: a single UTF-8
	// code point can straddle two tokens, and converting per token would turn it
	// into replacement characters. Irrelevant for ASCII JSON, wrong the moment it
	// is not.
	TArray<char> OutputBytes;
	OutputBytes.Reserve(512);

	const double DecodeStart = FPlatformTime::Seconds();
	int32 OutputTokens = 0;
	double TtftMs = 0.0;
	int32 CurrentPosition = PromptTokens.Num();

	for (int32 TokenIndex = 0; TokenIndex < USiegeLlamaSubsystem::MaxOutputTokens; ++TokenIndex)
	{
		if (bCancelRequested || bStopRequested)
		{
			bCancelled = true;
			FailureDetail = FString::Printf(
				TEXT("CANCELLED during DECODE after %d output token(s) (%s)"), OutputTokens, AbortRegime(Tier));
			break;
		}

		const llama_token Token = llama_sampler_sample(Chain, Context, -1);

		if (TokenIndex == 0)
		{
			TtftMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		}

		if (llama_vocab_is_eog(Vocab, Token))
		{
			break;
		}

		char PieceBuffer[256];
		const int32 PieceLength = llama_token_to_piece(Vocab, Token, PieceBuffer,
			static_cast<int32>(sizeof(PieceBuffer)), 0, false);
		if (PieceLength > 0)
		{
			OutputBytes.Append(PieceBuffer, PieceLength);
		}

		++OutputTokens;

		if (CurrentPosition + 1 >= ActualCtx)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s: context full at %d tokens (n_ctx=%d) -- generation truncated."),
				TagGen, CurrentPosition, ActualCtx);
			break;
		}

		llama_token NextToken = Token;
		llama_batch Batch = llama_batch_get_one(&NextToken, 1);
		const int32 DecodeResult = llama_decode(Context, Batch);
		if (DecodeResult != 0)
		{
			bDecodeFailed = true;
			bAborted = (DecodeResult == 2);
			bCancelled = bAborted && bCancelRequested;
			FailureDetail = FString::Printf(
				TEXT("DECODE llama_decode=%d at output token %d of a %d-token budget%s"),
				DecodeResult, TokenIndex, USiegeLlamaSubsystem::MaxOutputTokens,
				(DecodeResult == 2) ? TEXT(" -- code 2 is ABORTED, so the JSON is TRUNCATED, not wrong") : TEXT(""));
			break;
		}
		++CurrentPosition;

		CheckSoftTimeout(SoftDeadlineSeconds, OutputTokens, bSoftTimeoutExceeded);

		const double Now = FPlatformTime::Seconds();
		if (Now > HardDeadlineSeconds)
		{
			bAborted = true;
			// SAME FIX AS THE PREFILL TIMEOUT ABOVE, AND THIS IS THE COPY THAT
			// ACTUALLY FIRES (qa/TASK-446.md WARN-3 named only the prefill one).
			// Measured prefill is 0.4-2.9 s worst-case across all three tiers
			// (TASK-413), so the prefill ceiling is barely reachable -- whereas the
			// partial tier's worst DECODE wall was 8926 ms against this 10 s
			// ceiling, i.e. this branch is roughly one slow token away on a tier
			// where abort_callback enforces nothing. This is the line a deadline
			// report would quote.
			//
			// Now, not a fresh clock read: Now is the exact value that tripped the
			// test above, and it shares StartSeconds' origin with the deadline, so
			// (printed - 10.0) IS the overshoot. Not DecodeStart -- that base
			// excludes the prefill and cannot be compared with the ceiling.
			FailureDetail = FString::Printf(
				TEXT("HARD TIMEOUT: %.1fs elapsed after %d output token(s) of a %d-token budget. llama_decode never returned an error -- the LOOP stopped, so this is the ceiling cutting a slow run, not a generation failure. Regime: %s"),
				Now - StartSeconds, OutputTokens,
				USiegeLlamaSubsystem::MaxOutputTokens, AbortRegime(Tier));
			break;
		}
	}

	llama_sampler_free(Chain);
	FPlatformAtomics::InterlockedExchange(&AbortDeadlineUsec, 0);

	const double DecodeMs = (FPlatformTime::Seconds() - DecodeStart) * 1000.0;
	const double WallMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

	OutputBytes.Add('\0');
	const FString Output = FString(UTF8_TO_TCHAR(OutputBytes.GetData()));

	const bool bSuccess = !bCancelled && !bDecodeFailed && !bAborted && !Output.IsEmpty();

	if (bSuccess)
	{
		LastPromptTokens = MoveTemp(PromptTokens);
	}
	else
	{
		LastPromptTokens.Reset();
	}

	UE_LOG(LogSiegeLlama, Display,
		TEXT("%s tier=%s prompt_tok=%d prefix_reused=%d prefill_tok=%d out_tok=%d grammar=%s ttft=%.0fms prefill=%.0fms decode=%.0fms wall=%.0fms result=%s"),
		TagGen, TierName(Tier), PromptTokenCount, CommonPrefix,
		PromptTokenCount - CommonPrefix, OutputTokens,
		bGrammarActive ? TEXT("CONSTRAINED") : TEXT("UNCONSTRAINED"),
		TtftMs, PrefillMs, DecodeMs, WallMs,
		bSuccess ? TEXT("ok") : TEXT("FAILED"));

	if (!bSuccess)
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s: %s"), TagGen,
			FailureDetail.IsEmpty() ? TEXT("the generation produced no output") : *FailureDetail);
	}

	PostCompletion(bSuccess, Output,
		bSuccess ? FString() : (FailureDetail.IsEmpty() ? FString(TEXT("the generation produced no output")) : FailureDetail),
		bSoftTimeoutExceeded);
}

// ===========================================================================
// 8. COMPLETION -- ALWAYS ON THE GAME THREAD
// ===========================================================================

void FSiegeLlamaWorker::PostCompletion(bool bSuccess, const FString& Output, const FString& Error, bool bSoftTimeoutExceeded)
{
	// EVERYTHING IS CAPTURED BY VALUE. The lambda must not reach back into this
	// worker: Deinitialize can delete it between the post and the run, and the
	// completion must still fire rather than crash or silently vanish.
	TWeakObjectPtr<USiegeLlamaSubsystem> WeakOwner = Owner;
	FString OutputCopy = Output;
	FString ErrorCopy = Error;

	AsyncTask(ENamedThreads::GameThread,
		[WeakOwner, bSuccess, OutputCopy, ErrorCopy, bSoftTimeoutExceeded]()
		{
			if (USiegeLlamaSubsystem* Subsystem = WeakOwner.Get())
			{
				Subsystem->HandleWorkerCompletion(bSuccess, OutputCopy, ErrorCopy, bSoftTimeoutExceeded);
			}
			// A destroyed subsystem drops the completion, which is correct: the
			// only thing that could have been waiting on it died with the game
			// instance. No dangling delegate into a destroyed UObject.
		});
}

void FSiegeLlamaWorker::CheckSoftTimeout(double SoftDeadlineSeconds, int32 OutputTokensSoFar, bool& bInOutFired)
{
	if (bInOutFired || FPlatformTime::Seconds() <= SoftDeadlineSeconds)
	{
		return;
	}

	bInOutFired = true;

	// UX ONLY. NOTHING IS ABORTED and the run continues to the hard ceiling.
	UE_LOG(LogSiegeLlama, Log,
		TEXT("%s: SOFT timeout of %.1fs passed (%d output token(s) so far). NOTHING IS ABORTED -- the request continues to the %.1fs hard ceiling. OnRequestSoftTimeout is broadcasting NOW, on the game thread, so the FSM can say something while the player waits."),
		SiegeLlamaPrivate::TagGen, USiegeLlamaSubsystem::SoftTimeoutSeconds,
		OutputTokensSoFar, USiegeLlamaSubsystem::HardTimeoutSeconds);

	NotifySoftTimeout();
}

void FSiegeLlamaWorker::NotifySoftTimeout()
{
	TWeakObjectPtr<USiegeLlamaSubsystem> WeakOwner = Owner;
	AsyncTask(ENamedThreads::GameThread, [WeakOwner]()
	{
		if (USiegeLlamaSubsystem* Subsystem = WeakOwner.Get())
		{
			Subsystem->HandleSoftTimeout();
		}
	});
}

FString FSiegeLlamaWorker::DescribeState() const
{
	const TCHAR* StateName = TEXT("?");
	switch (GetState())
	{
		case EState::Loading: StateName = TEXT("LOADING"); break;
		case EState::Idle:    StateName = TEXT("IDLE");    break;
		case EState::Busy:    StateName = TEXT("BUSY");    break;
		case EState::Faulted: StateName = TEXT("FAULTED"); break;
		case EState::Stopped: StateName = TEXT("STOPPED"); break;
		default: break;
	}

	return FString::Printf(
		TEXT("state=%s tier=%s n_ctx_actual=%d (requested %d) static_prefix_tok=%d model='%s'"),
		StateName, SiegeLlamaPrivate::TierName(GetTier()),
		ActualContextSize.GetValue(), USiegeLlamaSubsystem::ContextTokens,
		StaticPrefixTokens.GetValue(), *ModelPath);
}

// ===========================================================================
// 9. THE SUBSYSTEM -- GAME THREAD ONLY, EVERY LINE
// ===========================================================================

void USiegeLlamaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	using namespace SiegeLlamaPrivate;

	FSiegeLlamaModule* Module = FSiegeLlamaModule::GetPtr();
	if (Module == nullptr || !Module->IsLlamaAvailable())
	{
		// ONE LINE, AND THEN INERT. The vendored DLLs are delay-loaded, so
		// calling into them here would raise a structured exception rather than
		// return an error.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: llama.cpp is unavailable (%s). The in-match assistant is DISABLED for this session. This is a DEGRADED state, not a crash -- match start is unaffected and every keyboard command still works."),
			TagLoad, (Module != nullptr) ? *Module->GetLoadError() : TEXT("the SiegeLlama module is not loaded"));
		return;
	}

	// RESOLVED HERE, ON THE GAME THREAD. USiegeLlamaSettings is a UObject CDO and
	// the override chain must not be walked from a worker.
	const FString ModelPath = USiegeLlamaSettings::ResolveModelPath();

	// THE ABSENT-MODEL PATH, AND IT IS EXACTLY ONE LOG LINE.
	// Checked BEFORE the backends are registered so a machine with no weights
	// never pays the Vulkan device enumeration, and never starts a thread.
	if (!FPaths::FileExists(ModelPath))
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: no model file at '%s' -- the in-match assistant is unavailable this session. Run Tools/fetch_llm_model.py, set Siege Llama > Model Path, or pass -siegellm.model=<path>. THE MATCH IS FULLY PLAYABLE: nothing is blocked, nothing is retried, and every keyboard command is byte-identical."),
			TagLoad, *ModelPath);
		return;
	}

	// Backend registration on the GAME THREAD, deliberately: FSiegeLlamaModule's
	// bBackendsLoaded latch is a plain bool and Siege.Llama.Info can call
	// EnsureBackendsLoaded from the console, so doing it on the worker would be a
	// data race on module state this file does not own. It is the CHEAP half of
	// the load (DLL registration, not weights); the 2.5 GB read stays on the
	// worker, which is what "the model load never blocks match start" is about.
	const double BackendStart = FPlatformTime::Seconds();
	const bool bBackends = Module->EnsureBackendsLoaded();
	const double BackendMs = (FPlatformTime::Seconds() - BackendStart) * 1000.0;

	if (!bBackends)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: no ggml backends registered (%.0f ms) -- the assistant is unavailable this session. Match start is unaffected."),
			TagLoad, BackendMs);
		return;
	}

	UE_LOG(LogSiegeLlama, Log,
		TEXT("%s: ggml backends ready in %.0f ms. Starting the ASYNC model load on a TPri_BelowNormal worker -- Initialize() returns now and match start never waits on it."),
		TagLoad, BackendMs);

	// ===================================================================
	// DELETE THIS BLOCK IN THE SAME COMMIT AS SiegeLlamaSpike.cpp.
	//
	// TWO MODEL-LOAD PATHS EXIST IN THIS BUILD and the ruling is that they
	// never should. The spike harness was NOT deleted at this task, and the
	// reason is recorded in handoffs/TASK-450-programmer.md: the sealed
	// generation-2 holdout corpus is authored against the t0 fixture INSIDE
	// SiegeLlamaSpike.cpp and is scored by Siege.Llama.SpikeEval, which is
	// also inside it -- deleting the file would spend a sealed, deliberately
	// UNSPENT artifact by destroying the only instrument that can ever open
	// it.
	//
	// So the hazard is real and it is named here rather than left implicit:
	// the spike loads its own llama_model and llama_context into the same
	// process. Driving BOTH in one session means two ~2.5 GB models resident
	// on a card whose whole bar #4 question is whether ONE fits 8 GB.
	// ===================================================================
	UE_LOG(LogSiegeLlama, Warning,
		TEXT("%s: THE SPIKE HARNESS IS STILL PRESENT IN THIS BUILD, so this process has TWO independent model-load paths. DO NOT drive Siege.Llama.Spike* and Siege.Llama.Test / the assistant in the SAME session -- that puts two ~2.5 GB models on one card. Take any spike reading in its own session. This warning and the spike are deleted together."),
		TagLoad);

	Worker = new FSiegeLlamaWorker(ModelPath, this);
}

void USiegeLlamaSubsystem::Deinitialize()
{
	if (Worker != nullptr)
	{
		// Stops the loop, aborts an in-flight decode through the same
		// abort_callback a cancel uses, aborts an in-flight model load through
		// the progress callback, joins the thread, and frees model + context ON
		// THE WORKER before returning.
		Worker->JoinAndDestroy();
		delete Worker;
		Worker = nullptr;
	}

	// Any completion still in the task-graph queue holds only a weak pointer to
	// this object and will find it gone. Clearing the delegate here means nothing
	// can execute into a half-destroyed FSM either.
	ActiveDelegate.Unbind();

	Super::Deinitialize();
}

bool USiegeLlamaSubsystem::IsReady() const
{
	return Worker != nullptr && Worker->IsReadyState();
}

bool USiegeLlamaSubsystem::IsBusy() const
{
	return Worker != nullptr && Worker->IsBusyState();
}

ESiegeLlamaOffloadTier USiegeLlamaSubsystem::GetOffloadTier() const
{
	return Worker != nullptr ? Worker->GetTier() : ESiegeLlamaOffloadTier::None;
}

FString USiegeLlamaSubsystem::DescribeState() const
{
	if (Worker == nullptr)
	{
		return TEXT("state=UNAVAILABLE (no worker: the DLLs, the backends or the model file were missing at Initialize)");
	}
	return Worker->DescribeState();
}

bool USiegeLlamaSubsystem::RequestCompletion(const FString& Prompt, const FString& Grammar, const FSiegeLlamaCompletionSignature& OnComplete)
{
	using namespace SiegeLlamaPrivate;

	check(IsInGameThread());

	if (!IsReady())
	{
		UE_LOG(LogSiegeLlama, Verbose,
			TEXT("%s REJECTED: the subsystem is not ready (%s). Nothing was started and OnComplete will NOT fire."),
			TagRequest, *DescribeState());
		return false;
	}

	// QUEUE DEPTH 1, ENFORCED BY THIS EARLY RETURN AND BY NOTHING ELSE.
	// There is deliberately no queue. A second concurrent call is REFUSED, and
	// the refusal is the contract working rather than an obstacle -- it is what
	// deletes the out-of-order / stale-context bug class outright.
	if (IsBusy())
	{
		UE_LOG(LogSiegeLlama, Log,
			TEXT("%s REJECTED: a request is already in flight. QUEUE DEPTH IS 1 BY DESIGN -- nothing was started, nothing was queued, and OnComplete will NOT fire for this call."),
			TagRequest);
		return false;
	}

	if (Prompt.IsEmpty())
	{
		UE_LOG(LogSiegeLlama, Warning, TEXT("%s REJECTED: the prompt is empty."), TagRequest);
		return false;
	}

	// --- THE PRE-FILTER CUT SITE -------------------------------------------
	// CHEAP, FINITE, NEVER BINDING IN NORMAL PLAY -- and on the GAME THREAD,
	// before a single token is counted, which is what makes it cheap.
	//
	// It measures the SNAPSHOT REGION, not the whole prompt: the static prefix is
	// Zone A and section 8's budget governs Zone B + Zone C only. Without a
	// registered prefix the region cannot be identified, so the pre-filter is
	// skipped -- the worker says so, once, at Warning.
	//
	// It rejects rather than truncates, for the reason in RunRequestUnguarded:
	// the last line of the snapshot is the player's own sentence.
	const FString Prefix = Worker->GetStaticPrefixCopy();
	if (!Prefix.IsEmpty() && Prompt.StartsWith(Prefix, ESearchCase::CaseSensitive))
	{
		const int32 SnapshotChars = Prompt.Len() - Prefix.Len();
		if (SnapshotChars > SnapshotPreFilterMaxChars)
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("%s PRE-FILTER CUT SITE FIRED -- REQUEST REJECTED, NOT TRUNCATED. The snapshot region is %d chars against SnapshotPreFilterMaxChars=%d (prompt %d chars, static prefix %d chars). A pre-filter firing in a real match is a BUG REPORT ABOUT ZONE C'S SHAPE, not a signal to retune the pre-filter -- 3000 is already 3.1x as-built B+C and about 2x the highest chars/token ratio ever measured here."),
				TagBudget, SnapshotChars, SnapshotPreFilterMaxChars, Prompt.Len(), Prefix.Len());
			return false;
		}
	}

	// --- ACCEPT ------------------------------------------------------------
	// The delegate lives on the GAME THREAD and nowhere else: the worker never
	// sees it, never copies it and never executes it. That is what makes "the
	// completion ALWAYS fires on the game thread" structural rather than a
	// convention.
	ActiveDelegate = OnComplete;
	Worker->SubmitRequest(Prompt, Grammar);
	return true;
}

void USiegeLlamaSubsystem::CancelActiveRequest()
{
	check(IsInGameThread());

	if (Worker == nullptr)
	{
		return;
	}

	if (!Worker->IsBusyState())
	{
		// Idempotent and harmless when idle. Deliberately not a warning: the FSM
		// cancelling on console-close should not have to check first.
		return;
	}

	UE_LOG(LogSiegeLlama, Log,
		TEXT("%s: CancelActiveRequest -- routed into llama's abort_callback. %s"),
		SiegeLlamaPrivate::TagRequest, SiegeLlamaPrivate::AbortRegime(GetOffloadTier()));

	Worker->RequestCancel();
}

bool USiegeLlamaSubsystem::SetStaticPrefix(const FString& InStaticPrefix)
{
	check(IsInGameThread());

	if (Worker == nullptr || InStaticPrefix.IsEmpty())
	{
		return false;
	}

	if (IsBusy())
	{
		// The prefix may not move under a live KV cache: LastPromptTokens was
		// built against the old one and the reuse point would be attributed to a
		// prefix that is no longer there.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("%s: SetStaticPrefix REFUSED -- a request is in flight. Register the prefix once, before the first request."),
			SiegeLlamaPrivate::TagBudget);
		return false;
	}

	Worker->SubmitStaticPrefix(InStaticPrefix);
	return true;
}

void USiegeLlamaSubsystem::HandleWorkerCompletion(bool bSuccess, const FString& Output, const FString& Error, bool bSoftTimeoutExceeded)
{
	check(IsInGameThread());

	// CLEARED BEFORE THE DELEGATE FIRES, ON PURPOSE. A handler that immediately
	// issues the next turn must SUCCEED -- that call is sequential, not
	// concurrent, and queue depth 1 is about concurrency. Clearing afterwards
	// would refuse a perfectly legal next request for one frame.
	if (Worker != nullptr && Worker->GetState() == FSiegeLlamaWorker::EState::Busy)
	{
		Worker->ClearBusy();
	}

	UE_LOG(LogSiegeLlama, Verbose,
		TEXT("%s: completion landed on the game thread. success=%d soft_timeout_was_exceeded=%d output=%d chars"),
		SiegeLlamaPrivate::TagRequest, bSuccess ? 1 : 0, bSoftTimeoutExceeded ? 1 : 0, Output.Len());

	// MOVED OUT BEFORE EXECUTION so a handler that re-enters RequestCompletion
	// overwrites a slot nobody is reading.
	FSiegeLlamaCompletionSignature Delegate = ActiveDelegate;
	ActiveDelegate.Unbind();

	// ExecuteIfBound rather than Execute: a UObject-bound delegate whose object
	// died between the request and this frame is unbound by construction, so this
	// is also the "no dangling delegate into a destroyed UObject" guarantee.
	Delegate.ExecuteIfBound(bSuccess, Output, Error);
}

void USiegeLlamaSubsystem::HandleSoftTimeout()
{
	check(IsInGameThread());

	// Only meaningful while the request it belongs to is still running. A post
	// that lands after the answer did is dropped rather than broadcast, so the
	// FSM is never told to say "still thinking" about a finished turn.
	if (!IsBusy())
	{
		return;
	}

	OnRequestSoftTimeout.Broadcast();
}

// ===========================================================================
// 10. DEV DRIVER COMMANDS (Siege.Llama.*, FAutoConsoleCommand, section 5)
// ===========================================================================
//
// These exist because there is NO game-lane caller yet -- the FSM is a later
// task -- so without them the integration gate has no way to exercise a single
// one of the properties above. They are the harness for: async load never
// blocking match start, one parseable JSON from a hard-coded prompt + GBNF,
// queue depth 1 refusing a second concurrent call, a cancel landing during
// prefill, and the absent-model path.
//
// Not shipped in a Shipping build, and never a UFUNCTION(exec) on a gameplay
// class -- the whole inference lane stays new-files-only.

#if !UE_BUILD_SHIPPING

namespace SiegeLlamaDev
{
	static USiegeLlamaSubsystem* Find(UWorld* World)
	{
		if (World == nullptr)
		{
			UE_LOG(LogSiegeLlama, Warning, TEXT("Siege.Llama.*: no world. Run this from PIE."));
			return nullptr;
		}
		UGameInstance* GameInstance = World->GetGameInstance();
		if (GameInstance == nullptr)
		{
			UE_LOG(LogSiegeLlama, Warning, TEXT("Siege.Llama.*: no game instance."));
			return nullptr;
		}
		return GameInstance->GetSubsystem<USiegeLlamaSubsystem>();
	}

	/**
	 *  A GBNF that is trivially legal. Its job is to prove that constrained
	 *  decoding produces ONE PARSEABLE JSON, not to test the real grammar.
	 *
	 *  Rule names are letters and hyphens only -- llama.cpp's parser reads a rule
	 *  name as letters/digits/hyphens, and TASK-413 lost six bench runs to a
	 *  single underscore in `at_least`. No underscores here, deliberately.
	 */
	static FString BuildProbeGrammar()
	{
		return FString(
			TEXT("root ::= \"{\\\"ok\\\":\" flag \",\\\"n\\\":\" digit \"}\"\n")
			TEXT("flag ::= \"true\" | \"false\"\n")
			TEXT("digit ::= [1-9]\n"));
	}

	/** Short probe prompt -- fast, for the JSON and the queue-depth checks. */
	static FString BuildProbePrompt()
	{
		return FString(TEXT("Reply with one JSON object and nothing else. Set ok to true and n to 7.\n"));
	}

	/**
	 *  A LONG prompt, so the prefill takes real time and a cancel has something
	 *  to land inside. Built by repetition rather than from any game content --
	 *  this file may not know what a snapshot looks like.
	 */
	static FString BuildLongProbePrompt()
	{
		FString Prompt;
		Prompt.Reserve(8192);
		Prompt += TEXT("The following is filler used only to make the prefill long enough to cancel inside.\n");
		for (int32 Index = 0; Index < 200; ++Index)
		{
			Prompt += FString::Printf(
				TEXT("line %d: the quick brown fox jumps over the lazy dog while counting to ten.\n"), Index);
		}
		Prompt += BuildProbePrompt();
		return Prompt;
	}

	static void LogResult(const TCHAR* Label, bool bSuccess, const FString& Output, const FString& Error)
	{
		if (bSuccess)
		{
			UE_LOG(LogSiegeLlama, Display,
				TEXT("Siege.Llama.Test [%s] COMPLETED on the GAME THREAD (IsInGameThread=%d). Output (%d chars): %s"),
				Label, IsInGameThread() ? 1 : 0, Output.Len(), *Output);
		}
		else
		{
			UE_LOG(LogSiegeLlama, Warning,
				TEXT("Siege.Llama.Test [%s] FAILED on the GAME THREAD (IsInGameThread=%d). Reason: %s"),
				Label, IsInGameThread() ? 1 : 0, *Error);
		}
	}

	static void ExecStatus(const TArray<FString>& /*Args*/, UWorld* World, FOutputDevice& /*Ar*/)
	{
		if (USiegeLlamaSubsystem* Subsystem = Find(World))
		{
			UE_LOG(LogSiegeLlama, Display,
				TEXT("Siege.Llama.Status: IsReady=%d IsBusy=%d %s"),
				Subsystem->IsReady() ? 1 : 0, Subsystem->IsBusy() ? 1 : 0, *Subsystem->DescribeState());
		}
	}

	static void ExecTest(const TArray<FString>& Args, UWorld* World, FOutputDevice& /*Ar*/)
	{
		USiegeLlamaSubsystem* Subsystem = Find(World);
		if (Subsystem == nullptr)
		{
			return;
		}

		bool bLong = false;
		bool bTwice = false;
		int32 CancelAfterMs = -1;
		for (const FString& Arg : Args)
		{
			if (Arg.Equals(TEXT("long"), ESearchCase::IgnoreCase)) { bLong = true; }
			else if (Arg.Equals(TEXT("twice"), ESearchCase::IgnoreCase)) { bTwice = true; }
			else if (Arg.StartsWith(TEXT("cancelms="), ESearchCase::IgnoreCase))
			{
				CancelAfterMs = FCString::Atoi(*Arg.Mid(9));
			}
		}

		const FString Prompt = bLong ? BuildLongProbePrompt() : BuildProbePrompt();
		const FString Grammar = BuildProbeGrammar();

		UE_LOG(LogSiegeLlama, Display,
			TEXT("Siege.Llama.Test: prompt=%d chars grammar=%d chars long=%d twice=%d cancelms=%d. %s"),
			Prompt.Len(), Grammar.Len(), bLong ? 1 : 0, bTwice ? 1 : 0, CancelAfterMs, *Subsystem->DescribeState());

		FSiegeLlamaCompletionSignature First;
		First.BindLambda([](bool bSuccess, const FString& Output, const FString& Error)
		{
			LogResult(TEXT("first"), bSuccess, Output, Error);
		});

		const bool bFirstAccepted = Subsystem->RequestCompletion(Prompt, Grammar, First);
		UE_LOG(LogSiegeLlama, Display,
			TEXT("Siege.Llama.Test: call 1 RequestCompletion returned %s."),
			bFirstAccepted ? TEXT("TRUE (accepted)") : TEXT("FALSE (refused)"));

		if (bTwice)
		{
			// QUEUE DEPTH 1, PROVEN IN ONE COMMAND: the second call happens in the
			// same function, on the same frame, while the first is in flight.
			FSiegeLlamaCompletionSignature Second;
			Second.BindLambda([](bool bSuccess, const FString& Output, const FString& Error)
			{
				LogResult(TEXT("second"), bSuccess, Output, Error);
			});

			const bool bSecondAccepted = Subsystem->RequestCompletion(Prompt, Grammar, Second);
			UE_LOG(LogSiegeLlama, Display,
				TEXT("Siege.Llama.Test: call 2 RequestCompletion returned %s. QUEUE DEPTH 1 REQUIRES call 1 TRUE and call 2 FALSE -- anything else is a FAIL, and two TRUEs with the model absent means the test proved nothing."),
				bSecondAccepted ? TEXT("TRUE (accepted -- THIS IS A FAIL if call 1 was also TRUE)") : TEXT("FALSE (refused, correct)"));
		}

		if (CancelAfterMs >= 0 && bFirstAccepted)
		{
			TWeakObjectPtr<USiegeLlamaSubsystem> WeakSubsystem(Subsystem);
			FTSTicker::GetCoreTicker().AddTicker(TEXT("SiegeLlamaTestCancel"),
				static_cast<float>(CancelAfterMs) / 1000.0f,
				[WeakSubsystem](float /*Delta*/)
				{
					if (USiegeLlamaSubsystem* Live = WeakSubsystem.Get())
					{
						UE_LOG(LogSiegeLlama, Display,
							TEXT("Siege.Llama.Test: firing CancelActiveRequest (IsBusy=%d)."), Live->IsBusy() ? 1 : 0);
						Live->CancelActiveRequest();
					}
					return false;
				});
		}
	}

	static void ExecCancel(const TArray<FString>& /*Args*/, UWorld* World, FOutputDevice& /*Ar*/)
	{
		if (USiegeLlamaSubsystem* Subsystem = Find(World))
		{
			UE_LOG(LogSiegeLlama, Display, TEXT("Siege.Llama.Cancel: IsBusy=%d"), Subsystem->IsBusy() ? 1 : 0);
			Subsystem->CancelActiveRequest();
		}
	}

	static FAutoConsoleCommandWithWorldArgsAndOutputDevice GStatusCommand(
		TEXT("Siege.Llama.Status"),
		TEXT("Prints the inference subsystem's state, tier, actual n_ctx and static-prefix token count."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ExecStatus));

	static FAutoConsoleCommandWithWorldArgsAndOutputDevice GTestCommand(
		TEXT("Siege.Llama.Test"),
		TEXT("Drives one constrained completion. Args: long | twice | cancelms=<n>. 'twice' proves queue depth 1; 'long cancelms=200' proves a cancel lands during prefill."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ExecTest));

	static FAutoConsoleCommandWithWorldArgsAndOutputDevice GCancelCommand(
		TEXT("Siege.Llama.Cancel"),
		TEXT("Cancels the in-flight completion through llama's abort_callback."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ExecCancel));
}

#endif // !UE_BUILD_SHIPPING
