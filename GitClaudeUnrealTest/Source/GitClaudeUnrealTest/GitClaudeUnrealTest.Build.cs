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
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"Niagara"
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
