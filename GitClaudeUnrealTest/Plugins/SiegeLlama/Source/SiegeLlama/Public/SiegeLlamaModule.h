#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

/**
 * SiegeLlama runtime module.
 *
 * Owns the lifetime of the vendored llama.cpp DLLs and nothing else. It holds
 * no model, no context and no worker thread -- those belong to
 * USiegeLlamaSubsystem (TASK-423).
 *
 * THE DEGRADATION POSTURE IS THE POINT: the vendored DLLs are delay-loaded, and
 * this module resolves them explicitly at startup. If any of them is missing or
 * unloadable, the module logs ONCE on LogSiegeLlama, latches
 * bLlamaAvailable = false, and stays inert. A missing DLL must degrade to
 * "assistant unavailable", never to a load-time crash or a blocked editor
 * launch -- the same posture as every soft-loaded material in this codebase.
 *
 * M8 DECLARATION DUTY: adds no replicated property, no new replicated class,
 * no new relevancy tier.
 */
class SIEGELLAMA_API FSiegeLlamaModule : public IModuleInterface
{
public:
	//~ IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface

	/** Null when the module is not loaded. Never dereference without checking. */
	static FSiegeLlamaModule* GetPtr();

	/**
	 * True only when every vendored llama.cpp DLL resolved.
	 *
	 * !! GATE EVERY llama_ AND ggml_ CALL ON THIS !!
	 * (Written without glob asterisks on purpose: an asterisk immediately
	 *  followed by a slash inside this block would close the comment early.)
	 * The DLLs are delay-loaded, so calling into them while this is false
	 * raises a structured exception rather than returning an error.
	 */
	bool IsLlamaAvailable() const { return bLlamaAvailable; }

	/** Human-readable reason IsLlamaAvailable() is false. Empty when available. */
	const FString& GetLoadError() const { return LoadError; }

	/**
	 * Idempotently registers the ggml backends (Vulkan, CPU) from the vendored
	 * bin directory and performs llama's one-time global init.
	 *
	 * Deferred rather than done in StartupModule on purpose: enumerating Vulkan
	 * devices creates driver objects and costs editor-startup time we should not
	 * pay until something actually wants inference.
	 *
	 * @return false when llama is unavailable or registration produced no backends.
	 */
	bool EnsureBackendsLoaded();

	/** Absolute directory the vendored DLLs were loaded from. Empty if unresolved. */
	const FString& GetResolvedBinDir() const { return ResolvedBinDir; }

	/**
	 * Candidate directories for the vendored DLLs, in probe order:
	 *   1. <PluginBaseDir>/Source/ThirdParty/LlamaCpp/bin/Win64  (editor / dev)
	 *   2. FPlatformProcess::BaseDir()                            (packaged, via
	 *      RuntimeDependencies -> $(BinaryOutputDir))
	 */
	static TArray<FString> GetCandidateBinDirs();

private:
	bool LoadLlamaDlls();
	void FreeLlamaDlls();

	/** Freed in reverse order at shutdown; dependencies load first. */
	TArray<void*> DllHandles;

	FString ResolvedBinDir;
	FString LoadError;

	bool bLlamaAvailable = false;
	bool bBackendsLoaded = false;
};
