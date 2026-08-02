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
	 * PROVISIONAL. The spike defaults to Qwen3.5-4B-Instruct Q4_K_M from the
	 * cleared permissive list; TASK-410/413 confirm the exact repo id, filename
	 * and quant, and TASK-413 records the licence line verbatim. Note that
	 * every purpose-built small function-calling model (xLAM-2, Hammer 2.1,
	 * Arch-Function) is non-commercial and BANNED.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Model")
	FString DefaultModelFileName = TEXT("Qwen3.5-4B-Instruct-Q4_K_M.gguf");

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
