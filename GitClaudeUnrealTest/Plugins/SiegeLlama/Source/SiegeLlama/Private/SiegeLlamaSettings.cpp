#include "SiegeLlamaSettings.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

FName USiegeLlamaSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FString USiegeLlamaSettings::GetModelSearchDir()
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Models")));
}

FString USiegeLlamaSettings::ResolveModelPath()
{
	auto MakeAbsolute = [](const FString& InPath) -> FString
	{
		return FPaths::ConvertRelativePathToFull(
			FPaths::IsRelative(InPath)
				? FPaths::Combine(FPaths::ProjectDir(), InPath)
				: InPath);
	};

	// 1. Command line wins, so a single run can point at any quant without
	//    touching config. FParse::Value handles a quoted path with spaces.
	FString CommandLineOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("siegellm.model="), CommandLineOverride)
		&& !CommandLineOverride.IsEmpty())
	{
		return MakeAbsolute(CommandLineOverride);
	}

	// 2. Project settings / DefaultGame.ini.
	if (const USiegeLlamaSettings* Settings = GetDefault<USiegeLlamaSettings>())
	{
		if (!Settings->ModelPath.IsEmpty())
		{
			return MakeAbsolute(Settings->ModelPath);
		}

		if (!Settings->DefaultModelFileName.IsEmpty())
		{
			// 3. Plugin default.
			return FPaths::Combine(GetModelSearchDir(), Settings->DefaultModelFileName);
		}
	}

	return GetModelSearchDir();
}
