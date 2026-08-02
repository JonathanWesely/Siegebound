using UnrealBuildTool;

public class SiegeLlama : ModuleRules
{
	public SiegeLlama(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// REQUIRED BY THE SPEC ITSELF, and both are flagged in the handoff
			// against TASK-409's "Core, CoreUObject, Engine only" wording:
			//   Projects          -> IPluginManager, which the spec mandates for
			//                        locating the plugin base dir at runtime.
			//   DeveloperSettings -> UDeveloperSettings, the base class of
			//                        USiegeLlamaSettings (the model override chain).
			// Neither is HTTP nor Sockets -- the actual prohibition, whose stated
			// rationale is the Windows Defender firewall prompt from a sidecar
			// llama-server.exe. That decision stays closed: this plugin opens no
			// socket and makes no network call.
			"Projects",
			"DeveloperSettings",

			// The vendored llama.cpp C API. PRIVATE on purpose: llama.h must not
			// reach the game module's include path. The contract across the
			// plugin boundary is (prompt, gbnf) -> string, nothing more.
			"LlamaCpp",
		});
	}
}
