// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GitClaudeUnrealTest : ModuleRules
{
	public GitClaudeUnrealTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",

			// ⚠️ SlateCore is NOT optional and is NOT implied by "Slate".
			// Slate PUBLICLY DEPENDS on SlateCore, so it propagates SlateCore's
			// INCLUDE PATHS — which is why everything here compiled clean for a
			// long time without it — but a dependency does not propagate the
			// IMPORT LIBRARY, so nothing could LINK against a SlateCore symbol.
			// That gap is invisible until some file references an actual symbol
			// rather than just a type, and it fails at LINK rather than at
			// compile: 16 unresolved externals led by
			// Z_Construct_UEnum_SlateCore_ETextCommit.
			//
			// ⚠️ THE EXAMPLE THIS COMMENT ONCE NAMED — SiegeAssistantInputProbe.cpp —
			// WAS DELETED BY TASK-444 (the throwaway-probe ruling). ⛔ THAT IS NOT A
			// REASON TO DROP SlateCore: only the EXAMPLE went stale, never the rule
			// (CONVENTIONS "Settings screen…" §12 — when a justifying comment names
			// an artifact that is later deleted, RE-POINT THE COMMENT; never delete
			// the thing it justifies). The example is re-pointed here, by TASK-443,
			// which owns this file.
			//
			// THE LIVE REASON TODAY: SiegeAssistantConsoleWidget.{h,cpp} is now the
			// module's ONLY ETextCommit-in-a-UFUNCTION signature (HandleTextCommitted)
			// and its ONLY FSlateApplication user
			// (FSlateApplication::SetAllUserFocusToGameViewport). Removing SlateCore
			// reproduces the exact link failure above. Epic's own commented
			// boilerplate below pairs the two modules for exactly this reason.
			"SlateCore",

			"Niagara",

			// LLM-ASSISTANT batch (TASK-417): ParseSiegeAssistantCommand parses the
			// model's constrained-decoding output in the game lane.
			// ⚠️ NOT "HTTP" and NOT "Sockets" — llama.cpp runs IN-PROCESS. The
			// sidecar llama-server.exe was considered and rejected, because a
			// Windows Defender firewall prompt on first launch of a shipped game is
			// unacceptable. Adding either module for this feature re-opens a closed
			// decision (CONVENTIONS "In-match LLM command assistant" §6).
			"Json",
			"JsonUtilities",

			// SETTINGS+CONFIRM batch (TASK-443): the game lane's FIRST call into the
			// inference plugin. USiegeAssistantComponent::DispatchTurnToModel resolves
			// USiegeLlamaSubsystem off the GameInstance and calls RequestCompletion /
			// IsReady / IsBusy / CancelActiveRequest. TASK-417 deliberately deferred
			// this line to the commit where that call first exists, and this is it.
			//
			// ⛔ STILL NOT "HTTP" AND STILL NOT "Sockets", for the unchanged reason
			// stated directly above: llama.cpp runs IN-PROCESS and the sidecar
			// llama-server.exe was considered and REJECTED. Adding either module
			// re-opens a closed decision (CONVENTIONS "In-match LLM command
			// assistant" §6) — this plugin opens no socket and makes no network call.
			//
			// ⚠️ THIS DOES NOT LEAK llama.h INTO THIS MODULE. LlamaCpp is a PRIVATE
			// dependency of SiegeLlama, so the native header stays off this module's
			// include path and the whole cross-lane surface remains
			// (prompt, gbnf) -> string.
			"SiegeLlama"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GitClaudeUnrealTest",
			"GitClaudeUnrealTest/ThirdPerson",
			"GitClaudeUnrealTest/Variant_Platforming",
			"GitClaudeUnrealTest/Variant_Platforming/Animation",
			"GitClaudeUnrealTest/Variant_Combat",
			"GitClaudeUnrealTest/Variant_Combat/AI",
			"GitClaudeUnrealTest/Variant_Combat/Animation",
			"GitClaudeUnrealTest/Variant_Combat/Gameplay",
			"GitClaudeUnrealTest/Variant_Combat/Interfaces",
			"GitClaudeUnrealTest/Variant_Combat/UI",
			"GitClaudeUnrealTest/Variant_SideScrolling",
			"GitClaudeUnrealTest/Variant_SideScrolling/AI",
			"GitClaudeUnrealTest/Variant_SideScrolling/Gameplay",
			"GitClaudeUnrealTest/Variant_SideScrolling/Interfaces",
			"GitClaudeUnrealTest/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
