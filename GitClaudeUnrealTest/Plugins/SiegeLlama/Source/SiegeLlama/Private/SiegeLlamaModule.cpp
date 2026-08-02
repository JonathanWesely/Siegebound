#include "SiegeLlamaModule.h"

#include "SiegeLlamaLog.h"

#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

// Third-party C API. Wrapped so llama.cpp's headers are not held to this
// project's warnings-as-errors settings.
THIRD_PARTY_INCLUDES_START
#include "ggml-backend.h"
#include "llama.h"
THIRD_PARTY_INCLUDES_END

#define LOCTEXT_NAMESPACE "FSiegeLlamaModule"

// The ONE definition of the plugin log category. See SiegeLlamaLog.h -- TASK-423
// must not add a second one (duplicate symbol at link).
DEFINE_LOG_CATEGORY(LogSiegeLlama);

namespace SiegeLlamaPrivate
{
	/**
	 * Load order matters: each entry's load-time dependencies precede it.
	 * ggml-base imports libomp140; ggml imports ggml-base; llama imports both.
	 *
	 * The ggml BACKENDS (ggml-vulkan.dll, ggml-cpu-*.dll) are deliberately
	 * absent: ggml opens those itself, by filename, at backend-registration
	 * time. They are runtime dependencies, never link-time imports.
	 */
	static const TCHAR* const RequiredDlls[] =
	{
		TEXT("libomp140.x86_64.dll"),
		TEXT("ggml-base.dll"),
		TEXT("ggml.dll"),
		TEXT("llama.dll"),
	};
}

TArray<FString> FSiegeLlamaModule::GetCandidateBinDirs()
{
	TArray<FString> Candidates;

	// 1. The vendored tree, via the plugin base dir (editor + any dev build).
	//    Non-.uasset blobs must not live in Content/ -- they would be
	//    cook-ignored -- so the vendored tree is the source of truth here.
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SiegeLlama")))
	{
		Candidates.Add(FPaths::ConvertRelativePathToFull(
			FPaths::Combine(Plugin->GetBaseDir(),
				TEXT("Source"), TEXT("ThirdParty"), TEXT("LlamaCpp"),
				TEXT("bin"), TEXT("Win64"))));
	}

	// 2. Next to the executable -- where RuntimeDependencies staged them via
	//    $(BinaryOutputDir) in a packaged build.
	Candidates.Add(FPaths::ConvertRelativePathToFull(FPlatformProcess::BaseDir()));

	return Candidates;
}

bool FSiegeLlamaModule::LoadLlamaDlls()
{
	TArray<FString> Attempted;

	for (const FString& CandidateDir : GetCandidateBinDirs())
	{
		Attempted.Add(CandidateDir);

		if (!FPaths::DirectoryExists(CandidateDir))
		{
			continue;
		}

		// Require the full set before loading anything, so a partial vendored
		// tree fails cleanly instead of half-loading.
		bool bAllPresent = true;
		for (const TCHAR* const DllName : SiegeLlamaPrivate::RequiredDlls)
		{
			if (!FPaths::FileExists(FPaths::Combine(CandidateDir, DllName)))
			{
				bAllPresent = false;
				break;
			}
		}
		if (!bAllPresent)
		{
			continue;
		}

		// Pushing the directory lets the loader resolve each DLL's own
		// dependencies out of the vendored folder rather than the system path.
		FPlatformProcess::PushDllDirectory(*CandidateDir);

		bool bLoadedAll = true;
		for (const TCHAR* const DllName : SiegeLlamaPrivate::RequiredDlls)
		{
			const FString FullPath = FPaths::Combine(CandidateDir, DllName);
			void* Handle = FPlatformProcess::GetDllHandle(*FullPath);
			if (Handle == nullptr)
			{
				LoadError = FString::Printf(TEXT("failed to load '%s'"), *FullPath);
				bLoadedAll = false;
				break;
			}
			DllHandles.Add(Handle);
		}

		FPlatformProcess::PopDllDirectory(*CandidateDir);

		if (bLoadedAll)
		{
			ResolvedBinDir = CandidateDir;
			LoadError.Empty();
			return true;
		}

		// Partial load: release what we took before trying the next candidate.
		FreeLlamaDlls();
	}

	if (LoadError.IsEmpty())
	{
		LoadError = FString::Printf(
			TEXT("vendored llama.cpp binaries not found. Searched: %s"),
			*FString::Join(Attempted, TEXT(" | ")));
	}
	return false;
}

void FSiegeLlamaModule::FreeLlamaDlls()
{
	for (int32 Index = DllHandles.Num() - 1; Index >= 0; --Index)
	{
		if (DllHandles[Index] != nullptr)
		{
			FPlatformProcess::FreeDllHandle(DllHandles[Index]);
		}
	}
	DllHandles.Reset();
	ResolvedBinDir.Empty();
}

void FSiegeLlamaModule::StartupModule()
{
	bLlamaAvailable = LoadLlamaDlls();

	if (bLlamaAvailable)
	{
		UE_LOG(LogSiegeLlama, Log,
			TEXT("SiegeLlama: vendored llama.cpp loaded from '%s' (%d modules)."),
			*ResolvedBinDir, DllHandles.Num());
	}
	else
	{
		// ONE line, at Warning, and then the module stays inert. This is the
		// whole point of the delay-load posture: no crash, no blocked editor
		// launch, and match start is never gated on inference being present.
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("SiegeLlama: llama.cpp unavailable -- the in-match assistant will be disabled. Reason: %s"),
			*LoadError);
	}
}

void FSiegeLlamaModule::ShutdownModule()
{
	if (bLlamaAvailable && bBackendsLoaded)
	{
		llama_backend_free();
	}
	bBackendsLoaded = false;
	bLlamaAvailable = false;

	FreeLlamaDlls();
}

bool FSiegeLlamaModule::EnsureBackendsLoaded()
{
	if (!bLlamaAvailable)
	{
		return false;
	}
	if (bBackendsLoaded)
	{
		return ggml_backend_reg_count() > 0;
	}

	// Load backends explicitly from the directory we resolved, rather than
	// relying on ggml's default search. In the editor the vendored tree is not
	// next to the executable, so the default search would find nothing.
	ggml_backend_load_all_from_path(TCHAR_TO_UTF8(*ResolvedBinDir));
	llama_backend_init();
	bBackendsLoaded = true;

	const size_t RegisteredBackends = ggml_backend_reg_count();
	if (RegisteredBackends == 0)
	{
		UE_LOG(LogSiegeLlama, Warning,
			TEXT("SiegeLlama: no ggml backends registered from '%s' -- inference is unavailable."),
			*ResolvedBinDir);
		return false;
	}

	UE_LOG(LogSiegeLlama, Log, TEXT("SiegeLlama: %d ggml backend(s), %d device(s)."),
		static_cast<int32>(RegisteredBackends),
		static_cast<int32>(ggml_backend_dev_count()));
	return true;
}

FSiegeLlamaModule* FSiegeLlamaModule::GetPtr()
{
	return FModuleManager::GetModulePtr<FSiegeLlamaModule>(TEXT("SiegeLlama"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSiegeLlamaModule, SiegeLlama)
