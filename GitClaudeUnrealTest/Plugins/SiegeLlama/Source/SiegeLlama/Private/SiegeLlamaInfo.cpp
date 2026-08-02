// Siege.Llama.Info -- the LINK PROOF for the vendored llama.cpp.
//
// Registered via FAutoConsoleCommand in a NEW FILE, never as a UFUNCTION(exec)
// on a shipped class (CONVENTIONS section 5). USiegeCheatManager and
// ASiegePlayerController are untouched by this batch, which is what keeps the
// whole inference lane genuinely new-files-only. Namespace: Siege.Llama.*
//
// This command carries no gameplay logic and takes no measurements -- latency,
// accuracy and the go/no-go bars belong to TASK-410 / TASK-413.
//
// M8 DECLARATION DUTY: adds no replicated property, no new replicated class,
// no new relevancy tier.

#include "SiegeLlamaLog.h"
#include "SiegeLlamaModule.h"
#include "SiegeLlamaSettings.h"

#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"

THIRD_PARTY_INCLUDES_START
#include "ggml-backend.h"
#include "llama.h"
THIRD_PARTY_INCLUDES_END

namespace SiegeLlamaInfo
{
	// NOTE the mandatory `enum` keyword: ggml_backend_dev_type is BOTH a type
	// and a function (ggml-backend.h:182). In C++ the function name hides the
	// type name, so the elaborated-type-specifier is required -- which is why
	// upstream's own declaration reads `GGML_API enum ggml_backend_dev_type
	// ggml_backend_dev_type(...)`. Dropping `enum` here does not compile.
	static const TCHAR* DeviceTypeToString(const enum ggml_backend_dev_type Type)
	{
		switch (Type)
		{
			case GGML_BACKEND_DEVICE_TYPE_CPU:   return TEXT("CPU");
			case GGML_BACKEND_DEVICE_TYPE_GPU:   return TEXT("GPU");
			case GGML_BACKEND_DEVICE_TYPE_IGPU:  return TEXT("iGPU");
			case GGML_BACKEND_DEVICE_TYPE_ACCEL: return TEXT("ACCEL");
			case GGML_BACKEND_DEVICE_TYPE_META:  return TEXT("META");
			default:                             return TEXT("UNKNOWN");
		}
	}

	static void Execute()
	{
		UE_LOG(LogSiegeLlama, Display, TEXT("===== Siege.Llama.Info ====="));

		FSiegeLlamaModule* Module = FSiegeLlamaModule::GetPtr();
		if (Module == nullptr)
		{
			UE_LOG(LogSiegeLlama, Warning, TEXT("SiegeLlama module is not loaded."));
			return;
		}

		// --- DLL handle status -------------------------------------------------
		UE_LOG(LogSiegeLlama, Display, TEXT("DLLs available : %s"),
			Module->IsLlamaAvailable() ? TEXT("YES") : TEXT("NO"));

		if (!Module->IsLlamaAvailable())
		{
			// Never call into a delay-loaded DLL that failed to resolve: it
			// raises a structured exception rather than returning an error.
			UE_LOG(LogSiegeLlama, Warning, TEXT("Load error     : %s"), *Module->GetLoadError());
			UE_LOG(LogSiegeLlama, Display,
				TEXT("Assistant is unavailable. This is a DEGRADED state, not a crash -- "
					 "every keyboard command still works and match start is unaffected."));
			return;
		}

		UE_LOG(LogSiegeLlama, Display, TEXT("Vendored bin   : %s"), *Module->GetResolvedBinDir());

		// --- Model resolution (chain: -siegellm.model= > settings > default) ---
		const FString ResolvedModel = USiegeLlamaSettings::ResolveModelPath();
		UE_LOG(LogSiegeLlama, Display, TEXT("Model path     : %s"), *ResolvedModel);
		UE_LOG(LogSiegeLlama, Display, TEXT("Model present  : %s"),
			FPaths::FileExists(ResolvedModel) ? TEXT("YES") : TEXT("NO (run Tools/fetch_llm_model.py)"));

		// --- Backends ----------------------------------------------------------
		if (!Module->EnsureBackendsLoaded())
		{
			UE_LOG(LogSiegeLlama, Warning, TEXT("No ggml backends registered."));
			return;
		}

		UE_LOG(LogSiegeLlama, Display, TEXT("--- llama_print_system_info() ---"));
		UE_LOG(LogSiegeLlama, Display, TEXT("%s"), *FString(UTF8_TO_TCHAR(llama_print_system_info())));

		const size_t BackendCount = ggml_backend_reg_count();
		UE_LOG(LogSiegeLlama, Display, TEXT("--- registered backends: %d ---"),
			static_cast<int32>(BackendCount));
		for (size_t Index = 0; Index < BackendCount; ++Index)
		{
			ggml_backend_reg_t Reg = ggml_backend_reg_get(Index);
			UE_LOG(LogSiegeLlama, Display, TEXT("  [%d] %s"),
				static_cast<int32>(Index),
				*FString(UTF8_TO_TCHAR(ggml_backend_reg_name(Reg))));
		}

		const size_t DeviceCount = ggml_backend_dev_count();
		UE_LOG(LogSiegeLlama, Display, TEXT("--- devices: %d ---"),
			static_cast<int32>(DeviceCount));
		for (size_t Index = 0; Index < DeviceCount; ++Index)
		{
			ggml_backend_dev_t Device = ggml_backend_dev_get(Index);

			size_t FreeBytes = 0;
			size_t TotalBytes = 0;
			ggml_backend_dev_memory(Device, &FreeBytes, &TotalBytes);

			UE_LOG(LogSiegeLlama, Display, TEXT("  [%d] %-10s type=%-5s mem=%.1f/%.1f MiB | %s"),
				static_cast<int32>(Index),
				*FString(UTF8_TO_TCHAR(ggml_backend_dev_name(Device))),
				DeviceTypeToString(ggml_backend_dev_type(Device)),
				static_cast<double>(FreeBytes) / 1048576.0,
				static_cast<double>(TotalBytes) / 1048576.0,
				*FString(UTF8_TO_TCHAR(ggml_backend_dev_description(Device))));
		}

		UE_LOG(LogSiegeLlama, Display, TEXT("===== end Siege.Llama.Info ====="));
	}
}

static FAutoConsoleCommand GSiegeLlamaInfoCommand(
	TEXT("Siege.Llama.Info"),
	TEXT("Prints vendored llama.cpp status: DLL handles, ggml backends, devices, "
		 "llama_print_system_info() and the resolved model path."),
	FConsoleCommandDelegate::CreateStatic(&SiegeLlamaInfo::Execute));
