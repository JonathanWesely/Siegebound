#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SiegeLlamaSettings.generated.h"

/**
 * Project settings for the local LLM.
 *
 * THE OVERRIDE CHAIN EXISTS SO SWAPPING MODELS NEVER NEEDS A REBUILD -- the
 * spike evaluates several quants and a rebuild per model would be absurd:
 *
 *   1. -siegellm.model=<path>          command line (highest; per-run, no config edit)
 *   2. USiegeLlamaSettings::ModelPath  project settings / DefaultGame.ini
 *   3. <ProjectDir>/Models/<DefaultModelFileName>   plugin default
 *
 * THE WEIGHTS ARE NOT IN Content/ AND NOT IN GIT. A ~2.5 GB GGUF is not a
 * .uasset, so Content/ would cook-ignore it; and it never enters version
 * control (CONVENTIONS section 7 -- *.gguf and /Models/ are both ignored).
 * Tools/fetch_llm_model.py populates <ProjectDir>/Models/.
 *
 * Shipping/staging the GGUF is deliberately UNSOLVED here: it is a Wave-2
 * problem, because RuntimeDependencies and staging errors surface only in a
 * packaged build. Do not bolt a shipping path in early.
 *
 * M8 DECLARATION DUTY: adds no replicated property, no new replicated class,
 * no new relevancy tier.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Siege Llama"))
class SIEGELLAMA_API USiegeLlamaSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Explicit path to a .gguf. Absolute, or relative to the project directory.
	 * Empty falls through to <ProjectDir>/Models/<DefaultModelFileName>.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Model",
		meta = (DisplayName = "Model Path (.gguf)"))
	FString ModelPath;

	/**
	 * File looked for inside <ProjectDir>/Models/ when ModelPath is empty.
	 *
	 * LICENCE: CLEARED. Qwen/Qwen3-4B-GGUF :: Qwen3-4B-Q4_K_M.gguf, Apache-2.0,
	 * ungated, published by Qwen themselves (not a third-party re-upload) and
	 * shipping its own verbatim Apache-2.0 LICENSE file. Verified against that
	 * exact quant repo's card by TASK-413 and recorded in
	 * Docs/ThirdPartyNotices.md section 2. Note that every purpose-built small
	 * function-calling model (xLAM-2, Hammer 2.1, Arch-Function) is
	 * non-commercial and BANNED.
	 *
	 * The PREVIOUS default, "Qwen3.5-4B-Instruct-Q4_K_M.gguf", was a filename
	 * that CANNOT EXIST: Qwen publishes no -Instruct variant of Qwen3.5-4B and
	 * no official GGUF for it at all. It was never downloadable.
	 *
	 * WHY THE DENSE Qwen3-4B AND NOT Qwen3.5-4B: Qwen3.5-4B is a hybrid
	 * multimodal model whose 32 layers are only 8 full_attention to 24
	 * linear_attention (Gated DeltaNet). Linear-attention layers carry a rolling
	 * recurrent state rather than a per-token KV cache, so the prefix-reuse
	 * behaviour CONVENTIONS section 8 is written around does not apply to them.
	 * Qwen3-4B is Qwen3ForCausalLM, 36 uniform full-attention layers, GQA 32Q/8KV
	 * -- the architecture section 8 was actually designed against.
	 *
	 * This is a SPIKE MEASUREMENT choice, not a shipping decision. Jonathan
	 * rules on the shipping model at TASK-415. Known open risk: Qwen3 is a
	 * hybrid-reasoning model that can emit <think> blocks; the GBNF constrains
	 * generation from token 0, so any residual cost lands visibly in spike bar 5.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Model")
	FString DefaultModelFileName = TEXT("Qwen3-4B-Q4_K_M.gguf");

	/** Directory the fetch tool populates: <ProjectDir>/Models/. */
	static FString GetModelSearchDir();

	/**
	 * Applies the override chain above.
	 * @return absolute path to the intended .gguf. NOT guaranteed to exist --
	 *         callers must check, and a missing model degrades to "assistant
	 *         unavailable", never a hard failure.
	 */
	static FString ResolveModelPath();

	//~ UDeveloperSettings
	virtual FName GetCategoryName() const override;
	//~ End UDeveloperSettings
};
