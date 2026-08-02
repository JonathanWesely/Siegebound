// External module wrapping the vendored llama.cpp binaries.
//
// LAW (CONVENTIONS "In-match LLM command assistant" section 6, ruling B):
//   - This lives in the PLUGIN's Source/ThirdParty/, never the game module's.
//     The game module is one UBT unit under per-file single-owner locks, so
//     third-party include paths and /MD-vs-/MT link settings must not leak into
//     it. The plugin boundary is what lets the go/no-go spike run with zero
//     edits to any locked file.
//   - THE C API ONLY: llama.h + ggml.h and their transitive headers. We do NOT
//     link llama.cpp's common/ helpers (common.cpp, sampling.cpp, arg.cpp).
//     Those are C++ with STL in their signatures and are exactly where the
//     /MD-vs-/MT, _ITERATOR_DEBUG_LEVEL and MSVC-toolset-mismatch link failures
//     come from. Verified empirically: llama.dll's import table does not
//     reference llama-common.dll, so the C API is cleanly separable.
//   - Backend is Vulkan + CPU, NEVER CUDA. One binary covers NVIDIA / AMD /
//     Intel; CUDA would add 1 GB+ of DLLs and be NVIDIA-only.

using System.IO;
using UnrealBuildTool;

public class LlamaCpp : ModuleRules
{
	public LlamaCpp(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		// Vendored binaries are Win64-only. On any other platform this module
		// contributes nothing and the plugin degrades to "assistant unavailable".
		if (Target.Platform != UnrealTargetPlatform.Win64)
		{
			return;
		}

		string IncludeDir = Path.Combine(ModuleDirectory, "include");
		string LibDir     = Path.Combine(ModuleDirectory, "lib", "Win64");
		string BinDir     = Path.Combine(ModuleDirectory, "bin", "Win64");

		// System (not Public) include path: these are third-party headers and
		// must not be subjected to our warnings-as-errors settings.
		PublicSystemIncludePaths.Add(IncludeDir);

		// Consumer side of llama.cpp's dllimport/dllexport switches. Without
		// these the API macros expand to bare `extern` and we lose __declspec
		// (dllimport) -- it still links, but delay-loading is cleaner with it.
		PublicDefinitions.Add("GGML_SHARED=1");
		PublicDefinitions.Add("GGML_BACKEND_SHARED=1");
		PublicDefinitions.Add("LLAMA_SHARED=1");

		// Import libraries. These were generated from the upstream release DLLs
		// with `lib /def:` (upstream ships no .lib) -- see VERSION.md for the
		// exact, reproducible recipe.
		string[] ImportLibs = { "llama.lib", "ggml.lib", "ggml-base.lib" };
		foreach (string ImportLib in ImportLibs)
		{
			PublicAdditionalLibraries.Add(Path.Combine(LibDir, ImportLib));
		}

		// THE DLL TRAP, named so it is not rediscovered:
		// Delay-load ONLY the three DLLs we actually import symbols from. Delay
		// loading is what lets a missing/unloadable DLL degrade to "assistant
		// unavailable" instead of failing the whole editor at process load.
		// The module's StartupModule does an explicit FPlatformProcess::
		// GetDllHandle so the delay-load helper finds them already resident.
		// WARNING: because these are delay-loaded, calling ANY llama_*/ggml_*
		// function while FSiegeLlamaModule::IsLlamaAvailable() is false raises a
		// structured exception. Every call site must gate on it.
		string[] DelayLoad = { "llama.dll", "ggml.dll", "ggml-base.dll" };
		foreach (string Dll in DelayLoad)
		{
			PublicDelayLoadDLLs.Add(Dll);
		}

		// Stage EVERY vendored DLL, not just the delay-loaded three: the ggml
		// backends (ggml-vulkan, ggml-cpu-*) are opened by ggml itself at
		// runtime by filename, so they are never link-time imports and would
		// otherwise be missing from a packaged build.
		foreach (string DllPath in Directory.GetFiles(BinDir, "*.dll"))
		{
			RuntimeDependencies.Add("$(BinaryOutputDir)/" + Path.GetFileName(DllPath), DllPath);
		}
	}
}
