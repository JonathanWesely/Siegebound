// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeGraphicsSettingsSubsystem.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GenericPlatform/GenericWindow.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Scalability.h"

#define LOCTEXT_NAMESPACE "SiegeGraphics"

// The ONE definition of the graphics log category (GFX-§10 pins the name).
DEFINE_LOG_CATEGORY(LogSiegeGraphics);

// ─────────────────────────────────────────────────────────────────────────────
// THE CANONICAL GROUP-NAME PIN (GFX-§10; the ruling and its grounds are written
// in full at the top of SiegeGraphicsSettingsSubsystem.h, note (3)).
//
// ⛔ THESE ARE THE INI / sg.* CVar SPELLINGS, character-for-character, so a
// delegate payload, a log line, a widget name and the file on disk all read
// alike. The two engine spellings that DIFFER from these appear in this project
// at FOUR SITES TOTAL — a read and a write for each of the two symbols, all four
// inside the two funnels (ReadQualityGroupLevel and ApplyQualityGroupLevelInternal
// below) and all four commented in place as "NAME CROSSING #1/#2 OF 2 (read side)"
// and "… (write side)". (Measured and corrected by TASK-1114 NIT-1; this banner
// previously said "one site each" while naming two functions.)
// ─────────────────────────────────────────────────────────────────────────────
const FName USiegeGraphicsSettingsSubsystem::GroupName_ViewDistanceQuality(TEXT("ViewDistanceQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_AntiAliasingQuality(TEXT("AntiAliasingQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality(TEXT("ShadowQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_GlobalIlluminationQuality(TEXT("GlobalIlluminationQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_ReflectionQuality(TEXT("ReflectionQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_PostProcessQuality(TEXT("PostProcessQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_TextureQuality(TEXT("TextureQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_EffectsQuality(TEXT("EffectsQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality(TEXT("FoliageQuality"));
const FName USiegeGraphicsSettingsSubsystem::GroupName_ShadingQuality(TEXT("ShadingQuality"));

const FName USiegeGraphicsSettingsSubsystem::SettingName_OverallQuality(TEXT("OverallQuality"));
const FName USiegeGraphicsSettingsSubsystem::SettingName_ResolutionScale(TEXT("ResolutionScale"));
const FName USiegeGraphicsSettingsSubsystem::SettingName_ScreenResolution(TEXT("ScreenResolution"));
const FName USiegeGraphicsSettingsSubsystem::SettingName_WindowMode(TEXT("WindowMode"));
const FName USiegeGraphicsSettingsSubsystem::SettingName_VSync(TEXT("VSync"));
const FName USiegeGraphicsSettingsSubsystem::SettingName_FrameRateLimit(TEXT("FrameRateLimit"));
const FName USiegeGraphicsSettingsSubsystem::SettingName_AutoDetect(TEXT("AutoDetect"));

namespace SiegeGraphicsInternal
{
	/** The fallback screen resolution: the engine's own SetToDefaults DesiredScreen value (GameUserSettings.cpp:302-303). */
	static const FIntPoint FallbackScreenResolution(1280, 720);

	/** WindowedFullscreen — the measured live value in this machine's ini (TASK-1112 §1.6). */
	static constexpr int32 FallbackWindowMode = 1;

	/**
	 *  ⛔ THE UNCONFIRMED-VIDEO-MODE PREDICATE, and it is deliberately NOT the
	 *  engine's IsScreenResolutionDirty()/IsFullscreenModeDirty(). Those compare
	 *  the staged value against the LIVE VIEWPORT and both return false when
	 *  there is no GameViewport (GameUserSettings.cpp:219-239) ⇒ they read
	 *  "clean" in every headless context, including every automation run. This
	 *  compares staged-vs-LastConfirmed, which is the question GFX-§4 asks and
	 *  which is true with no viewport at all.
	 */
	static bool HasUnconfirmedVideoModeDifference(const UGameUserSettings* Settings)
	{
		if (!Settings)
		{
			return false;
		}

		return Settings->GetScreenResolution() != Settings->GetLastConfirmedScreenResolution()
			|| Settings->GetFullscreenMode() != Settings->GetLastConfirmedFullscreenMode();
	}

	/** GFX-§5's fixed ladder. ⛔ Unlimited (0) is LAST, so a stepper walks 30 → … → 240 → Unlimited. */
	static const TArray<float>& FrameRateLadder()
	{
		static const TArray<float> Ladder = { 30.0f, 60.0f, 90.0f, 120.0f, 144.0f, 165.0f, 240.0f, 0.0f };
		return Ladder;
	}

	/** The ten canonical group names in GFX-§8 Tier-B display order. */
	static const TArray<FName>& GroupNames()
	{
		static const TArray<FName> Names = {
			USiegeGraphicsSettingsSubsystem::GroupName_ViewDistanceQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_AntiAliasingQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_GlobalIlluminationQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_ReflectionQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_PostProcessQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_TextureQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_EffectsQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality,
			USiegeGraphicsSettingsSubsystem::GroupName_ShadingQuality
		};
		return Names;
	}
}

// ═════════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// ⛔ NO LoadSettings AND NO ApplySettings HERE, ON PURPOSE. The engine already
	// loads GameUserSettings.ini during FEngineLoop::PreInit and applies
	// scalability from it before any subsystem exists; re-loading would be
	// redundant and re-applying would stall startup for a state that is already
	// live. GFX-§6 makes the same point about the benchmark: never on first boot.
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		// ResolveSettings has already logged the failure exactly once.
		return;
	}

	// ⚠️ HEAL A STALE UNCONFIRMED VIDEO MODE AT BOOT. RequestSaveSettings refuses
	// every save while the staged mode differs from the last confirmed one, and
	// NOTHING in a normal session closes that window on its own — CloseVideoModeWindow
	// is reachable only from Confirm/Revert. The symptom would be "my graphics
	// settings don't stick", with only a Log line to explain it.
	//
	// ⛔ THE CASE THIS SURVIVES ON IS THE ABANDONED / CRASHED COUNTDOWN — NOT A
	// FRESH INSTALL. An earlier revision of this comment justified the heal on a
	// first launch; that justification is FALSE for a booted game and was corrected
	// by TASK-1114 WARN-7. The engine heals the first-launch case itself, at TWO
	// sites, both gated on a ZERO resolution:
	//   GameUserSettings.cpp:628-632  LoadSettings: bDetectingResolution =
	//                                 (ResolutionSizeX == 0 || …) ⇒ ConfirmVideoMode()
	//   GameUserSettings.cpp:470-480  ValidateSettings: ResolutionSizeX <= 0 ⇒ stamps
	//                                 all five LastConfirmed/LastUserConfirmed fields
	//                                 (reached from ApplyNonResolutionSettings :510
	//                                 and ApplyResolutionSettings :573)
	// Both run during engine init, long before any UGameInstanceSubsystem exists.
	//
	// The uncovered case is the one with a PERFECTLY NON-ZERO staged resolution and a
	// confirmed triple that disagrees with it — a countdown that was abandoned, or a
	// session that died mid-countdown after some full UGameUserSettings::SaveSettings
	// outside this facade's one guarded call site persisted the staged fields. Both
	// engine heals test for zero and both decline; nothing else in the engine looks.
	//
	// ⚠️ WHAT MAKES THE HEAL SAFE IS THAT NOTHING CAN WRONGLY PERSIST A MODE IN THE
	// FIRST PLACE — this heal turns a persisted staged mode into a PERMANENTLY
	// confirmed one on the following launch. That is why AutoDetectQuality's
	// IsVideoModeChangePending() refusal (TASK-1114 BLOCKER-1) is this function's
	// precondition and not an unrelated fix. ⛔ Do not keep one without the other.
	//
	// Confirming the CURRENT mode — rather than reverting to LastConfirmed — is the
	// truthful repair: the engine boots the game from the ini's STAGED fields
	// (PreloadResolutionSettings reads FullscreenMode and ResolutionSizeX/Y at
	// GameUserSettings.cpp:771-775 and hands them to RequestResolutionChange at :816),
	// so the player is demonstrably looking at the staged mode — the entire question
	// GFX-§4's confirmation asks. Reverting instead would set the object to a mode the
	// screen is NOT in, and the next ApplyResolutionSettings would then change the
	// player's display without being asked. The heal reaches no disk: ConfirmVideoMode
	// is five field assignments (:267-274), no apply and no save.
	if (SiegeGraphicsInternal::HasUnconfirmedVideoModeDifference(Settings))
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[SiegeGraphics] Boot found an UNCONFIRMED video mode left over from a previous session (staged %dx%d/mode %d vs confirmed %dx%d/mode %d). The game is running in the staged mode, so it is being confirmed — otherwise every future save would be refused."),
			Settings->GetScreenResolution().X, Settings->GetScreenResolution().Y,
			static_cast<int32>(Settings->GetFullscreenMode()),
			Settings->GetLastConfirmedScreenResolution().X, Settings->GetLastConfirmedScreenResolution().Y,
			static_cast<int32>(Settings->GetLastConfirmedFullscreenMode()));

		Settings->ConfirmVideoMode();
	}

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[SiegeGraphics] Subsystem initialized — Overall=%d (%s), ResolutionScale=%.1f%%%s, %dx%d, WindowMode=%d, VSync=%s, FrameRateLimit=%.0f."),
		GetOverallScalabilityLevel(),
		*GetQualityLevelDisplayName(GetOverallScalabilityLevel()).ToString(),
		GetResolutionScalePercent(),
		IsResolutionScaleProjectDefault() ? TEXT(" (PROJECT-DEFAULT SENTINEL)") : TEXT(""),
		GetScreenResolution().X, GetScreenResolution().Y,
		GetWindowMode(),
		IsVSyncEnabled() ? TEXT("on") : TEXT("off"),
		GetFrameRateLimit());
}

UGameUserSettings* USiegeGraphicsSettingsSubsystem::ResolveSettings() const
{
	// ⛔ Automation only. Non-null means a scratch object AND suppressed engine
	// applies — the two are set together so a test cannot enable one and forget
	// the other.
	if (SettingsOverrideForAutomationTests)
	{
		return SettingsOverrideForAutomationTests;
	}

	// ⚠️ THE FORCED-NULL PATH FALLS THROUGH TO THE SAME LOG BELOW RATHER THAN
	// RETURNING EARLY, ON PURPOSE: a test-only shortcut that skipped the logging
	// would exercise a DIFFERENT function than the one that ships, and the
	// latched-log behaviour is part of the contract being tested.
	UGameUserSettings* Settings = nullptr;
	if (!bForceNullSettingsForAutomationTests)
	{
		Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	}

	if (!Settings && !bResolveFailureLogged)
	{
		// LATCHED TO ONE LINE PER INSTANCE. Several of the getters above are
		// BlueprintPure and a widget may read them every frame; an unlatched
		// warning would bury the log it is meant to be findable in.
		bResolveFailureLogged = true;
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[SiegeGraphics] UGameUserSettings did not resolve (GEngine=%s). Every getter now returns its documented fallback and every setter is a no-op; this line is logged ONCE."),
			GEngine ? TEXT("valid") : TEXT("null"));
	}

	return Settings;
}

// ═════════════════════════════════════════════════════════════════════════════
// NAMES + LABELS
// ═════════════════════════════════════════════════════════════════════════════

TArray<FName> USiegeGraphicsSettingsSubsystem::GetQualityGroupNames()
{
	return SiegeGraphicsInternal::GroupNames();
}

FText USiegeGraphicsSettingsSubsystem::GetQualityGroupDisplayName(FName GroupName)
{
	if (GroupName == GroupName_ViewDistanceQuality)        { return LOCTEXT("Group_ViewDistance", "View Distance"); }
	if (GroupName == GroupName_AntiAliasingQuality)        { return LOCTEXT("Group_AntiAliasing", "Anti-Aliasing"); }
	if (GroupName == GroupName_ShadowQuality)              { return LOCTEXT("Group_Shadows", "Shadows"); }
	if (GroupName == GroupName_GlobalIlluminationQuality)  { return LOCTEXT("Group_GlobalIllumination", "Global Illumination"); }
	if (GroupName == GroupName_ReflectionQuality)          { return LOCTEXT("Group_Reflections", "Reflections"); }
	if (GroupName == GroupName_PostProcessQuality)         { return LOCTEXT("Group_PostProcessing", "Post Processing"); }
	if (GroupName == GroupName_TextureQuality)             { return LOCTEXT("Group_Textures", "Textures"); }
	if (GroupName == GroupName_EffectsQuality)             { return LOCTEXT("Group_Effects", "Effects"); }
	if (GroupName == GroupName_FoliageQuality)             { return LOCTEXT("Group_Foliage", "Foliage"); }
	if (GroupName == GroupName_ShadingQuality)             { return LOCTEXT("Group_Shading", "Shading"); }

	// An unknown name is a programming error. Returning the raw name is more
	// useful to whoever has to find it than an empty label would be.
	return FText::FromName(GroupName);
}

FText USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(int32 QualityLevel)
{
	if (QualityLevel == CustomQualityLevel)
	{
		return LOCTEXT("Quality_Custom", "Custom");
	}

	// ⛔ THE ENGINE'S OWN LOCALISED NAMES (Scalability.h:251). A hand-written
	// {"Low","Medium","High","Epic","Cinematic"} array would be one more thing
	// that can silently drift from the ini it is describing.
	return Scalability::GetScalabilityNameFromQualityLevel(
		FMath::Clamp(QualityLevel, MinQualityLevel, MaxQualityLevel));
}

// ═════════════════════════════════════════════════════════════════════════════
// TIER B — THE TEN QUALITY GROUPS
// ═════════════════════════════════════════════════════════════════════════════

int32 USiegeGraphicsSettingsSubsystem::ReadQualityGroupLevel(const UGameUserSettings* Settings, FName GroupName)
{
	if (!Settings)
	{
		return FallbackQualityLevel;
	}

	if (GroupName == GroupName_ViewDistanceQuality)        { return Settings->GetViewDistanceQuality(); }
	if (GroupName == GroupName_AntiAliasingQuality)        { return Settings->GetAntiAliasingQuality(); }
	if (GroupName == GroupName_ShadowQuality)              { return Settings->GetShadowQuality(); }
	if (GroupName == GroupName_GlobalIlluminationQuality)  { return Settings->GetGlobalIlluminationQuality(); }
	if (GroupName == GroupName_ReflectionQuality)          { return Settings->GetReflectionQuality(); }

	// ⚠️ NAME CROSSING #1 OF 2 (read side). Canonical "PostProcessQuality" ->
	// the engine's GetPostProcessingQuality, whose body returns
	// ScalabilityQuality.PostProcessQuality — i.e. the canonical spelling is the
	// one the value actually has; only this wrapper is spelled differently.
	if (GroupName == GroupName_PostProcessQuality)         { return Settings->GetPostProcessingQuality(); }

	if (GroupName == GroupName_TextureQuality)             { return Settings->GetTextureQuality(); }

	// ⚠️ NAME CROSSING #2 OF 2 (read side). Canonical "EffectsQuality" -> the
	// engine's GetVisualEffectQuality, whose body returns
	// ScalabilityQuality.EffectsQuality (GameUserSettings.cpp:1072-1075).
	if (GroupName == GroupName_EffectsQuality)             { return Settings->GetVisualEffectQuality(); }

	if (GroupName == GroupName_FoliageQuality)             { return Settings->GetFoliageQuality(); }
	if (GroupName == GroupName_ShadingQuality)             { return Settings->GetShadingQuality(); }

	// ⛔ LandscapeQuality IS NOT HERE, AND ITS ABSENCE IS THE POINT. The engine
	// has an eleventh int32 group (GameUserSettings.h:291/:295), but
	// BaseScalability.ini has NO [LandscapeQuality@N] section at any level, there
	// is no PerfIndexThresholds_LandscapeQuality for auto-detect to use, and this
	// project has no Landscape. A slider for it would move and change nothing —
	// the "control that lies" GFX-§9 forbids. Measured by TASK-1112 §1.1.
	UE_LOG(LogSiegeGraphics, Warning,
		TEXT("[SiegeGraphics] ReadQualityGroupLevel: '%s' is not one of the ten canonical group names — returning the fallback (%d). This is a programming error, not a player action."),
		*GroupName.ToString(), FallbackQualityLevel);
	return FallbackQualityLevel;
}

int32 USiegeGraphicsSettingsSubsystem::GetQualityGroupLevel(FName GroupName) const
{
	return ReadQualityGroupLevel(ResolveSettings(), GroupName);
}

void USiegeGraphicsSettingsSubsystem::SetQualityGroupLevel(FName GroupName, int32 NewLevel)
{
	ApplyQualityGroupLevelInternal(GroupName, NewLevel);
}

bool USiegeGraphicsSettingsSubsystem::ApplyQualityGroupLevelInternal(FName GroupName, int32 NewLevel)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// ⛔ CLAMP, NEVER ASSERT. A slider that is one float-rounding step out of
	// range must land on the nearest legal detent, not take the game down. The
	// engine's own FQualityLevels setters clamp too (Scalability.cpp:1117-1125),
	// but this facade owns its band explicitly so the guard is OURS and testable.
	const int32 ClampedLevel = FMath::Clamp(NewLevel, MinQualityLevel, MaxQualityLevel);

	const int32 CurrentLevel = ReadQualityGroupLevel(Settings, GroupName);
	if (CurrentLevel == ClampedLevel)
	{
		// ⛔ NO-OP: no engine write, no apply, no save, NO BROADCAST (the delegate
		// law). Note this also covers "the caller asked for 9 and we are already
		// at 4" — the clamp happens BEFORE the compare, so a repeated
		// out-of-range write is silent rather than a broadcast storm.
		UE_LOG(LogSiegeGraphics, Verbose,
			TEXT("[SiegeGraphics] '%s' is already %d — no-op (no apply, no save, no broadcast)."),
			*GroupName.ToString(), ClampedLevel);
		return false;
	}

	// ── THE ONLY PLACE THE ENGINE'S QUALITY-GROUP SETTERS ARE CALLED ──
	if (GroupName == GroupName_ViewDistanceQuality)             { Settings->SetViewDistanceQuality(ClampedLevel); }
	else if (GroupName == GroupName_AntiAliasingQuality)        { Settings->SetAntiAliasingQuality(ClampedLevel); }
	else if (GroupName == GroupName_ShadowQuality)              { Settings->SetShadowQuality(ClampedLevel); }
	else if (GroupName == GroupName_GlobalIlluminationQuality)  { Settings->SetGlobalIlluminationQuality(ClampedLevel); }
	else if (GroupName == GroupName_ReflectionQuality)          { Settings->SetReflectionQuality(ClampedLevel); }
	// ⚠️ NAME CROSSING #1 OF 2 (write side) — canonical "PostProcessQuality".
	else if (GroupName == GroupName_PostProcessQuality)         { Settings->SetPostProcessingQuality(ClampedLevel); }
	else if (GroupName == GroupName_TextureQuality)             { Settings->SetTextureQuality(ClampedLevel); }
	// ⚠️ NAME CROSSING #2 OF 2 (write side) — canonical "EffectsQuality".
	else if (GroupName == GroupName_EffectsQuality)             { Settings->SetVisualEffectQuality(ClampedLevel); }
	else if (GroupName == GroupName_FoliageQuality)             { Settings->SetFoliageQuality(ClampedLevel); }
	else if (GroupName == GroupName_ShadingQuality)             { Settings->SetShadingQuality(ClampedLevel); }
	else
	{
		// ReadQualityGroupLevel already logged the unknown name; refuse the write
		// rather than silently doing nothing while returning success.
		return false;
	}

	// GFX-§4: quality applies IMMEDIATELY and saves. It cannot soft-lock anyone.
	ApplyQualitySettings();
	BroadcastGraphicsSettingChanged(GroupName);
	return true;
}

int32 USiegeGraphicsSettingsSubsystem::GetViewDistanceQuality() const       { return GetQualityGroupLevel(GroupName_ViewDistanceQuality); }
void  USiegeGraphicsSettingsSubsystem::SetViewDistanceQuality(int32 L)      { ApplyQualityGroupLevelInternal(GroupName_ViewDistanceQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetAntiAliasingQuality() const       { return GetQualityGroupLevel(GroupName_AntiAliasingQuality); }
void  USiegeGraphicsSettingsSubsystem::SetAntiAliasingQuality(int32 L)      { ApplyQualityGroupLevelInternal(GroupName_AntiAliasingQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetShadowQuality() const             { return GetQualityGroupLevel(GroupName_ShadowQuality); }
void  USiegeGraphicsSettingsSubsystem::SetShadowQuality(int32 L)            { ApplyQualityGroupLevelInternal(GroupName_ShadowQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetGlobalIlluminationQuality() const { return GetQualityGroupLevel(GroupName_GlobalIlluminationQuality); }
void  USiegeGraphicsSettingsSubsystem::SetGlobalIlluminationQuality(int32 L){ ApplyQualityGroupLevelInternal(GroupName_GlobalIlluminationQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetReflectionQuality() const         { return GetQualityGroupLevel(GroupName_ReflectionQuality); }
void  USiegeGraphicsSettingsSubsystem::SetReflectionQuality(int32 L)        { ApplyQualityGroupLevelInternal(GroupName_ReflectionQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetPostProcessQuality() const        { return GetQualityGroupLevel(GroupName_PostProcessQuality); }
void  USiegeGraphicsSettingsSubsystem::SetPostProcessQuality(int32 L)       { ApplyQualityGroupLevelInternal(GroupName_PostProcessQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetTextureQuality() const            { return GetQualityGroupLevel(GroupName_TextureQuality); }
void  USiegeGraphicsSettingsSubsystem::SetTextureQuality(int32 L)           { ApplyQualityGroupLevelInternal(GroupName_TextureQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetEffectsQuality() const            { return GetQualityGroupLevel(GroupName_EffectsQuality); }
void  USiegeGraphicsSettingsSubsystem::SetEffectsQuality(int32 L)           { ApplyQualityGroupLevelInternal(GroupName_EffectsQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetFoliageQuality() const            { return GetQualityGroupLevel(GroupName_FoliageQuality); }
void  USiegeGraphicsSettingsSubsystem::SetFoliageQuality(int32 L)           { ApplyQualityGroupLevelInternal(GroupName_FoliageQuality, L); }

int32 USiegeGraphicsSettingsSubsystem::GetShadingQuality() const            { return GetQualityGroupLevel(GroupName_ShadingQuality); }
void  USiegeGraphicsSettingsSubsystem::SetShadingQuality(int32 L)           { ApplyQualityGroupLevelInternal(GroupName_ShadingQuality, L); }

// ═════════════════════════════════════════════════════════════════════════════
// TIER A — THE OVERALL PRESET
// ═════════════════════════════════════════════════════════════════════════════

int32 USiegeGraphicsSettingsSubsystem::GetOverallScalabilityLevel() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	return Settings ? Settings->GetOverallScalabilityLevel() : FallbackQualityLevel;
}

void USiegeGraphicsSettingsSubsystem::SetOverallScalabilityLevel(int32 NewLevel)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	const int32 ClampedLevel = FMath::Clamp(NewLevel, MinQualityLevel, MaxQualityLevel);
	if (Settings->GetOverallScalabilityLevel() == ClampedLevel)
	{
		// Already uniformly at this level ⇒ nothing to write, nothing to
		// broadcast. ⚠️ NOT the same as "the sliders look right": if the state is
		// Custom the engine returns -1, which never equals a clamped 0..4, so a
		// preset press out of Custom always writes.
		UE_LOG(LogSiegeGraphics, Verbose,
			TEXT("[SiegeGraphics] Overall quality is already %d — no-op (no apply, no save, no broadcast)."), ClampedLevel);
		return;
	}

	// ⚠️ A PRESET PRESS MOVES THE RESOLUTION SCALE TOO, AND THE PLAYER DID NOT ASK
	// IT TO (TASK-1114 WARN-5). SetOverallScalabilityLevel forwards to
	// FQualityLevels::SetFromSingleQualityLevel, which ALSO writes ResolutionQuality
	// from PerfIndexValues_ResolutionQuality = 50 71 87 100 100
	// (Scalability.cpp:1047). ⇒ Two player-visible values move on one write, and on
	// a first-ever run this is what DESTROYS the sg.ResolutionQuality=0 sentinel the
	// whole T3 design exists to protect. That is engine-native and we do not fight
	// it — but the delegate contract is "carry WHICH setting changed", so a widget
	// row bound to SettingName_ResolutionScale has to be told, or it displays a
	// stale number until something else happens to refresh it.
	const float ScaleBefore = Settings->GetResolutionScaleNormalized();

	// Writes all ELEVEN engine groups (including the LandscapeQuality this facade
	// does not expose) to the same level — which is exactly why the dropped
	// eleventh can never be the reason GetOverallScalabilityLevel() reports Custom.
	Settings->SetOverallScalabilityLevel(ClampedLevel);

	ApplyQualitySettings();
	BroadcastGraphicsSettingChanged(SettingName_OverallQuality);

	// ⚠️ SECOND BROADCAST, AND ONLY IF IT REALLY MOVED — the no-op law applies to
	// this one exactly as it does to the first. The snapshot-and-compare idiom is
	// AutoDetectQuality's (see its LevelsBefore/ScaleBefore pair), for the same
	// reason: this is a value we did not choose.
	if (!FMath::IsNearlyEqual(ScaleBefore, Settings->GetResolutionScaleNormalized(), UE_KINDA_SMALL_NUMBER))
	{
		UE_LOG(LogSiegeGraphics, Log,
			TEXT("[SiegeGraphics] The overall preset also moved the resolution scale (%.3f -> %.3f) — the engine writes ResolutionQuality from the preset (Scalability.cpp:1047)."),
			ScaleBefore, Settings->GetResolutionScaleNormalized());
		BroadcastGraphicsSettingChanged(SettingName_ResolutionScale);
	}
}

bool USiegeGraphicsSettingsSubsystem::IsOverallQualityCustom() const
{
	return GetOverallScalabilityLevel() == CustomQualityLevel;
}

bool USiegeGraphicsSettingsSubsystem::IsOverallQualityCustomAcrossVisibleGroups() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	const TArray<FName>& Names = SiegeGraphicsInternal::GroupNames();
	if (Names.Num() == 0)
	{
		return false;
	}

	const int32 First = ReadQualityGroupLevel(Settings, Names[0]);
	for (int32 Index = 1; Index < Names.Num(); ++Index)
	{
		if (ReadQualityGroupLevel(Settings, Names[Index]) != First)
		{
			return true;
		}
	}
	return false;
}

bool USiegeGraphicsSettingsSubsystem::AutoDetectQuality()
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// ⛔⛔ THE SECOND GFX-§4 GUARD, AND IT GUARDS A SAVE THAT IS NOT OURS.
	// ApplyHardwareBenchmarkResults' body is GameUserSettings.cpp:1132-1143 and its
	// SECOND-TO-LAST LINE IS SaveSettings() (:1142) — a FULL UGameUserSettings save
	// ending in SaveConfig(CPF_Config, GGameUserSettingsIni) (:683), which writes
	// EVERY UPROPERTY(config) on the class including ResolutionSizeX/Y
	// (GameUserSettings.h:465) and FullscreenMode (:500).
	// Scalability::SaveState (:1139) is only the FIRST of that function's TWO saves,
	// and it alone writes just [ScalabilityGroups]. Stopping the trace there — which
	// is exactly what this file's previous comment did — makes the wrong call look
	// safe (TASK-1114 BLOCKER-1).
	//
	// ⇒ Pressing Auto-Detect during a live confirmation countdown would persist a
	// video mode the player may not be able to see, entirely AROUND
	// RequestSaveSettings and its guard — the F-A lockout ("the lockout arriving
	// through a control that is not the resolution control") arriving through the
	// button next to the sliders. Refuse instead.
	//
	// ⚠️ THIS GUARD SITS ABOVE THE AUTOMATION SUPPRESSION BRANCH ON PURPOSE, so a
	// test can reach it. Moving it below makes it untestable and silently unproven.
	// ⛔ DO NOT "fix" this by hand-rolling RunHardwareBenchmark + SetQualityLevels in
	// place of ApplyHardwareBenchmarkResults: that re-implements engine code this
	// facade exists to wrap, and it would silently drop Scalability::SaveState.
	if (IsVideoModeChangePending())
	{
		++RefusedSaveWhileVideoModePendingCount;
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[SiegeGraphics] AutoDetectQuality REFUSED — a video-mode change is unconfirmed and the benchmark's own save would persist it (refusal #%d). Confirm or revert first."),
			RefusedSaveWhileVideoModePendingCount);
		return false;
	}

	if (bSuppressEngineApplyForAutomationTests)
	{
		// ⛔ Automation only. The benchmark spins the GPU for a second or two and
		// then writes the ini; neither belongs in a unit test.
		// ⚠️ Distinguishable from the refusal above by the counter: this branch
		// leaves RefusedSaveWhileVideoModePendingCount UNCHANGED, which is how
		// AutoDetectRefusedDuringUnconfirmedVideoMode tells the two falses apart.
		UE_LOG(LogSiegeGraphics, Verbose, TEXT("[SiegeGraphics] AutoDetectQuality suppressed (automation)."));
		return false;
	}

	UE_LOG(LogSiegeGraphics, Log, TEXT("[SiegeGraphics] AutoDetectQuality: running the hardware benchmark (this stalls briefly)."));

	// ⚠️ SNAPSHOT FIRST. The benchmark may legitimately land on exactly the levels
	// we already had, and the delegate law does not care that the MUTATION was
	// expensive — it cares whether the VALUE changed. Broadcasting unconditionally
	// here would be the "fires on a no-op" defect wearing a benchmark costume.
	// (This is the one mutation whose new value we do not choose, so the compare
	// has to happen after the write instead of before it.)
	const TArray<FName>& GroupNameList = SiegeGraphicsInternal::GroupNames();
	TArray<int32> LevelsBefore;
	LevelsBefore.Reserve(GroupNameList.Num());
	for (const FName& GroupName : GroupNameList)
	{
		LevelsBefore.Add(ReadQualityGroupLevel(Settings, GroupName));
	}
	const float ScaleBefore = Settings->GetResolutionScaleNormalized();

	// RunHardwareBenchmark POPULATES ONLY (GameUserSettings.cpp:1122-1130).
	// ApplyHardwareBenchmarkResults (:1132-1143 — the WHOLE body, all twelve lines)
	// applies and then saves TWICE:
	//   :1139  Scalability::SaveState(GGameUserSettingsIni)  — [ScalabilityGroups] only
	//   :1142  SaveSettings()                                — the FULL config save,
	//          SaveConfig(CPF_Config, GGameUserSettingsIni) at :683, which writes
	//          ResolutionSizeX/Y (GameUserSettings.h:465) and FullscreenMode (:500).
	// ⛔ The second one is why the IsVideoModeChangePending() refusal at the top of
	// this function exists. Reading only as far as :1139 is precisely the mistake
	// TASK-1114 BLOCKER-1 caught, and the range in this comment is the artefact that
	// would repeat it — it is quoted to :1143 deliberately (SC-§97).
	Settings->RunHardwareBenchmark();
	Settings->ApplyHardwareBenchmarkResults();

	bool bAnythingChanged = !FMath::IsNearlyEqual(ScaleBefore, Settings->GetResolutionScaleNormalized(), UE_KINDA_SMALL_NUMBER);
	for (int32 Index = 0; !bAnythingChanged && Index < GroupNameList.Num(); ++Index)
	{
		bAnythingChanged = ReadQualityGroupLevel(Settings, GroupNameList[Index]) != LevelsBefore[Index];
	}

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[SiegeGraphics] AutoDetectQuality complete — Overall=%d (%s), changed=%s."),
		GetOverallScalabilityLevel(), *GetQualityLevelDisplayName(GetOverallScalabilityLevel()).ToString(),
		bAnythingChanged ? TEXT("yes") : TEXT("no"));

	if (bAnythingChanged)
	{
		BroadcastGraphicsSettingChanged(SettingName_AutoDetect);
	}

	// ⚠️ RETURNS true FOR "THE BENCHMARK RAN", NOT "SOMETHING MOVED". A button that
	// reported failure because the machine was already correctly tuned would be
	// lying about the one thing it was asked to do.
	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// TIER C — RESOLUTION SCALE
// ═════════════════════════════════════════════════════════════════════════════

float USiegeGraphicsSettingsSubsystem::GetResolutionScaleNormalized() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return 1.0f;
	}

	// ⛔ RAW. NO CLAMP. See IsResolutionScaleProjectDefault().
	return Settings->GetResolutionScaleNormalized();
}

float USiegeGraphicsSettingsSubsystem::GetResolutionScalePercent() const
{
	// Measured (GameUserSettings.cpp:976-981 with Min 0 / Max 100): normalized is
	// a plain lerp over [0, 100], so percent == normalized * 100 EXACTLY. No
	// magic number is being invented here.
	return GetResolutionScaleNormalized() * 100.0f;
}

bool USiegeGraphicsSettingsSubsystem::IsResolutionScaleProjectDefault() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// sg.ResolutionQuality == 0 is a SENTINEL — BaseScalability.ini:39-42
	// +ResolutionPresets=(Name="Default",ResolutionQuality=0.0), "use the
	// project's default screen percentage" — and NOT a request to render at 0%.
	// This machine's live ini is in exactly this state right now (TASK-1112 §1.6).
	return Settings->GetResolutionScaleNormalized() <= ResolutionScaleSentinelEpsilon;
}

float USiegeGraphicsSettingsSubsystem::GetResolutionScalePercentForSlider() const
{
	if (IsResolutionScaleProjectDefault())
	{
		// ⛔ A DISPLAY TRANSFORM THAT WRITES NOTHING. The sentinel survives until
		// the player actually drags the slider — opening the menu must not change
		// their picture.
		return ResolutionScaleSentinelDisplayPercent;
	}

	return FMath::Clamp(GetResolutionScalePercent(), MinResolutionScalePercent, MaxResolutionScalePercent);
}

void USiegeGraphicsSettingsSubsystem::SetResolutionScaleNormalized(float NewScaleNormalized)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	// ⛔ THE CLAMP LIVES HERE — ON THE WRITE — AND NOWHERE ELSE. GFX-§5's 50-100%
	// band is load-bearing rather than cosmetic: the engine floor is 0.0
	// (Scalability.h:242), so an unclamped SetResolutionScaleNormalized(0) is a
	// literal request for a 0% render resolution.
	const float ClampedNormalized = FMath::Clamp(NewScaleNormalized,
		MinResolutionScalePercent / 100.0f, MaxResolutionScalePercent / 100.0f);

	const float CurrentNormalized = Settings->GetResolutionScaleNormalized();
	if (FMath::IsNearlyEqual(CurrentNormalized, ClampedNormalized, UE_KINDA_SMALL_NUMBER))
	{
		UE_LOG(LogSiegeGraphics, Verbose,
			TEXT("[SiegeGraphics] Resolution scale is already %.3f — no-op (no apply, no save, no broadcast)."), ClampedNormalized);
		return;
	}

	Settings->SetResolutionScaleNormalized(ClampedNormalized);

	// GFX-§4: resolution SCALE is a quality control, not a video mode — it cannot
	// make the screen unreadable, so it applies immediately like the groups do.
	// ⛔ It is NOT part of the confirm/revert dance and must never be dragged into it.
	ApplyQualitySettings();
	BroadcastGraphicsSettingChanged(SettingName_ResolutionScale);
}

void USiegeGraphicsSettingsSubsystem::SetResolutionScalePercent(float NewScalePercent)
{
	SetResolutionScaleNormalized(NewScalePercent / 100.0f);
}

// ═════════════════════════════════════════════════════════════════════════════
// TIER C — DISPLAY MODE (the only controls here that can lock a player out)
// ═════════════════════════════════════════════════════════════════════════════

FIntPoint USiegeGraphicsSettingsSubsystem::GetScreenResolution() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	return Settings ? Settings->GetScreenResolution() : SiegeGraphicsInternal::FallbackScreenResolution;
}

void USiegeGraphicsSettingsSubsystem::SetScreenResolution(FIntPoint NewResolution)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	if (NewResolution.X <= 0 || NewResolution.Y <= 0)
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[SiegeGraphics] SetScreenResolution refused: %dx%d is not a usable resolution."),
			NewResolution.X, NewResolution.Y);
		return;
	}

	if (Settings->GetScreenResolution() == NewResolution)
	{
		UE_LOG(LogSiegeGraphics, Verbose, TEXT("[SiegeGraphics] Screen resolution is already %dx%d — no-op."),
			NewResolution.X, NewResolution.Y);
		return;
	}

	// ⛔ STAGE ONLY. No apply, no save. The staged value now differs from the last
	// confirmed one, so IsVideoModeChangePending() is true and RequestSaveSettings
	// will refuse until Confirm or Revert closes the window.
	Settings->SetScreenResolution(NewResolution);
	BroadcastGraphicsSettingChanged(SettingName_ScreenResolution);
}

TArray<FIntPoint> USiegeGraphicsSettingsSubsystem::GetSupportedScreenResolutions() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	const FIntPoint Current = Settings ? Settings->GetScreenResolution() : SiegeGraphicsInternal::FallbackScreenResolution;

	TArray<FIntPoint> Resolutions;

	// ⚠️ NOT A UGameUserSettings MEMBER. Both are statics on UKismetSystemLibrary
	// (KismetSystemLibrary.h:1786 / :1793) and both RETURN bool.
	// ⚠️ AND THE WINDOWED LIST IS A DIFFERENT SET: the fullscreen list is derived
	// from the monitor's mode list and is the wrong answer for a window.
	const bool bWindowed = Settings && Settings->GetFullscreenMode() == EWindowMode::Windowed;
	const bool bQuerySucceeded = bWindowed
		? UKismetSystemLibrary::GetConvenientWindowedResolutions(Resolutions)
		: UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);

	if (!bQuerySucceeded || Resolutions.Num() == 0)
	{
		if (!bResolutionQueryFailureLogged)
		{
			bResolutionQueryFailureLogged = true;
			UE_LOG(LogSiegeGraphics, Warning,
				TEXT("[SiegeGraphics] The %s resolution query returned %s — falling back to a one-entry list holding the current mode (%dx%d). Logged ONCE."),
				bWindowed ? TEXT("windowed") : TEXT("fullscreen"),
				bQuerySucceeded ? TEXT("an EMPTY list") : TEXT("false"),
				Current.X, Current.Y);
		}

		// ⛔ NEVER RETURN AN EMPTY LIST. A stepper built on an empty array either
		// shows nothing or divides by zero; a one-entry list showing the mode the
		// player is already in is honest and inert.
		Resolutions.Reset();
		Resolutions.Add(Current);
	}

	return Resolutions;
}

int32 USiegeGraphicsSettingsSubsystem::GetSupportedScreenResolutionCount() const
{
	return GetSupportedScreenResolutions().Num();
}

FString USiegeGraphicsSettingsSubsystem::GetSupportedScreenResolutionLabel(int32 Index) const
{
	const TArray<FIntPoint> Resolutions = GetSupportedScreenResolutions();
	if (!Resolutions.IsValidIndex(Index))
	{
		// Empty string, never an assert: a stepper that overruns its own list is a
		// bug to see in the label, not a crash to see in a player's log.
		return FString();
	}

	return FString::Printf(TEXT("%d x %d"), Resolutions[Index].X, Resolutions[Index].Y);
}

int32 USiegeGraphicsSettingsSubsystem::FindCurrentScreenResolutionIndex() const
{
	const TArray<FIntPoint> Resolutions = GetSupportedScreenResolutions();
	const int32 Found = Resolutions.IndexOfByKey(GetScreenResolution());

	// A current mode that is not in the supported list is real (a hand-edited ini,
	// a monitor swap). Index 0 keeps the stepper usable instead of stranding it.
	return Found != INDEX_NONE ? Found : 0;
}

void USiegeGraphicsSettingsSubsystem::SetScreenResolutionByIndex(int32 Index)
{
	const TArray<FIntPoint> Resolutions = GetSupportedScreenResolutions();
	if (!Resolutions.IsValidIndex(Index))
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[SiegeGraphics] SetScreenResolutionByIndex(%d) refused — the supported list holds %d entries."),
			Index, Resolutions.Num());
		return;
	}

	SetScreenResolution(Resolutions[Index]);
}

int32 USiegeGraphicsSettingsSubsystem::GetWindowMode() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	return Settings ? static_cast<int32>(Settings->GetFullscreenMode()) : SiegeGraphicsInternal::FallbackWindowMode;
}

void USiegeGraphicsSettingsSubsystem::SetWindowMode(int32 NewWindowMode)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	const int32 ClampedMode = FMath::Clamp(NewWindowMode, 0, GetWindowModeCount() - 1);
	if (static_cast<int32>(Settings->GetFullscreenMode()) == ClampedMode)
	{
		UE_LOG(LogSiegeGraphics, Verbose, TEXT("[SiegeGraphics] Window mode is already %d — no-op."), ClampedMode);
		return;
	}

	// ⛔ STAGE ONLY — see SetScreenResolution.
	Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(ClampedMode));
	BroadcastGraphicsSettingChanged(SettingName_WindowMode);
}

int32 USiegeGraphicsSettingsSubsystem::GetWindowModeCount()
{
	// If the engine ever grows a fourth window mode this fails at COMPILE time
	// rather than shipping a stepper that cannot reach it.
	static_assert(static_cast<int32>(EWindowMode::NumWindowModes) == 3,
		"EWindowMode gained or lost a mode — update GetWindowModeLabel and the GFX-§5 stepper.");
	return static_cast<int32>(EWindowMode::NumWindowModes);
}

FString USiegeGraphicsSettingsSubsystem::GetWindowModeLabel(int32 WindowMode)
{
	switch (WindowMode)
	{
	case static_cast<int32>(EWindowMode::Fullscreen):         return TEXT("Fullscreen");
	// The engine calls it WindowedFullscreen; players call it borderless.
	case static_cast<int32>(EWindowMode::WindowedFullscreen): return TEXT("Borderless Window");
	case static_cast<int32>(EWindowMode::Windowed):           return TEXT("Windowed");
	default:                                                  return FString();
	}
}

bool USiegeGraphicsSettingsSubsystem::ApplyVideoModeProvisional()
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// ⛔ ApplyResolutionSettings(false), ⛔ NEVER ApplySettings(). ApplySettings'
	// last line is SaveSettings() (GameUserSettings.cpp:590-601), which would
	// persist an unconfirmed video mode and ship the exact lockout GFX-§4 exists
	// to prevent. This counter is bumped at the call site so a test can assert
	// which of the two was reached.
	++ApplyResolutionSettingsCallCount;
	if (!bSuppressEngineApplyForAutomationTests)
	{
		Settings->ApplyResolutionSettings(/*bCheckForCommandLineOverrides*/ false);
	}

	bVideoModeChangePending = true;

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[SiegeGraphics] Provisional video mode applied (%dx%d, mode %d) — NOT saved. Awaiting ConfirmVideoModeChange() or RevertVideoModeChange()."),
		Settings->GetScreenResolution().X, Settings->GetScreenResolution().Y,
		static_cast<int32>(Settings->GetFullscreenMode()));
	return true;
}

bool USiegeGraphicsSettingsSubsystem::IsVideoModeChangePending() const
{
	return bVideoModeChangePending
		|| SiegeGraphicsInternal::HasUnconfirmedVideoModeDifference(ResolveSettings());
}

bool USiegeGraphicsSettingsSubsystem::ConfirmVideoModeChange()
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// Stamps LastConfirmed* == current (GameUserSettings.cpp:267-274). It does NOT
	// save — GFX-§4's sequence is confirm THEN save, and the save is the tail below.
	Settings->ConfirmVideoMode();

	return CloseVideoModeWindow(TEXT("confirmed"));
}

bool USiegeGraphicsSettingsSubsystem::RevertVideoModeChange()
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// 🚨 STEP 1 OF 2. RevertVideoMode()'s COMPLETE body restores five member
	// fields and broadcasts (GameUserSettings.cpp:276-285). It pushes NOTHING to
	// the display.
	Settings->RevertVideoMode();

	// 🚨 STEP 2 OF 2 — ⛔ THE LINE THAT MAKES THE REVERT REAL. Without it the
	// screen keeps the broken mode while every property read-back reports the
	// good one: an SC-§94 cl. A lie, and the one failure GFX-§4's whole
	// confirm/revert dance exists to prevent. ⛔ DO NOT DELETE THIS.
	++ApplyResolutionSettingsCallCount;
	if (!bSuppressEngineApplyForAutomationTests)
	{
		Settings->ApplyResolutionSettings(/*bCheckForCommandLineOverrides*/ false);
	}

	return CloseVideoModeWindow(TEXT("reverted"));
}

bool USiegeGraphicsSettingsSubsystem::CloseVideoModeWindow(const TCHAR* Reason)
{
	bVideoModeChangePending = false;

	// Now that the window is closed, a save is legal again. Flush one if a quality
	// change was made (and refused) while the confirmation was outstanding —
	// otherwise that legitimate change would silently fail to persist.
	const bool bHadDeferredSave = bSaveDeferredByVideoModePending;
	bSaveDeferredByVideoModePending = false;

	const bool bSaved = RequestSaveSettings();

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[SiegeGraphics] Video mode %s — saved=%s%s."),
		Reason, bSaved ? TEXT("yes") : TEXT("no"),
		bHadDeferredSave ? TEXT(" (flushed a save deferred during the confirmation window)") : TEXT(""));

	// ⚠️ THIS BROADCAST IS NOT A NO-OP EVEN WHEN NEITHER VALUE MOVED: a confirm or
	// a revert always changes IsVideoModeChangePending(), which is the state the
	// countdown UI is bound to, so a listener that did not hear this would leave
	// the prompt on screen forever. The payload is SettingName_ScreenResolution
	// rather than a third name because "the video mode settled" is one event to a
	// consumer; a revert that was triggered by a window-mode change reports the
	// same token on purpose.
	BroadcastGraphicsSettingChanged(SettingName_ScreenResolution);
	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// TIER C — VSYNC + FRAME RATE
// ═════════════════════════════════════════════════════════════════════════════

bool USiegeGraphicsSettingsSubsystem::IsVSyncEnabled() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	return Settings ? Settings->IsVSyncEnabled() : false;
}

void USiegeGraphicsSettingsSubsystem::SetVSyncEnabled(bool bEnable)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings || Settings->IsVSyncEnabled() == bEnable)
	{
		return;
	}

	Settings->SetVSyncEnabled(bEnable);
	ApplyQualitySettings();
	BroadcastGraphicsSettingChanged(SettingName_VSync);
}

float USiegeGraphicsSettingsSubsystem::GetFrameRateLimit() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	return Settings ? Settings->GetFrameRateLimit() : 0.0f;
}

TArray<float> USiegeGraphicsSettingsSubsystem::GetFrameRateLimitLadder()
{
	return SiegeGraphicsInternal::FrameRateLadder();
}

int32 USiegeGraphicsSettingsSubsystem::GetFrameRateLimitOptionCount()
{
	return SiegeGraphicsInternal::FrameRateLadder().Num();
}

FString USiegeGraphicsSettingsSubsystem::GetFrameRateLimitLabel(float Limit)
{
	// 0 disables the limit (GameUserSettings.h:167). "0 FPS" would be a lie about
	// a control the player is looking straight at.
	if (Limit <= 0.0f)
	{
		return TEXT("Unlimited");
	}

	return FString::Printf(TEXT("%d FPS"), FMath::RoundToInt(Limit));
}

int32 USiegeGraphicsSettingsSubsystem::FindCurrentFrameRateLimitIndex() const
{
	const TArray<float>& Ladder = SiegeGraphicsInternal::FrameRateLadder();
	const float Current = GetFrameRateLimit();

	for (int32 Index = 0; Index < Ladder.Num(); ++Index)
	{
		if (FMath::IsNearlyEqual(Ladder[Index], Current, 0.5f))
		{
			return Index;
		}
	}

	// An off-ladder value can only come from outside this facade (a hand-edited
	// ini). Report Unlimited — the last rung — rather than -1, so the stepper is
	// never in a state it cannot render.
	return Ladder.Num() - 1;
}

void USiegeGraphicsSettingsSubsystem::SetFrameRateLimit(float NewLimit)
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	// SNAP TO THE LADDER BEFORE WRITING. GFX-§5 makes this a stepper precisely
	// because the spacing between 144 and 165 is not the spacing between 30 and
	// 60; an off-ladder value would leave the stepper unable to represent its own
	// state. 0 (Unlimited) is preserved exactly and never snapped up to 30.
	float SnappedLimit = 0.0f;
	if (NewLimit > 0.0f)
	{
		const TArray<float>& Ladder = SiegeGraphicsInternal::FrameRateLadder();
		float BestDelta = TNumericLimits<float>::Max();
		for (const float Rung : Ladder)
		{
			if (Rung <= 0.0f)
			{
				continue; // Unlimited is only reachable by asking for it explicitly.
			}

			const float Delta = FMath::Abs(Rung - NewLimit);
			if (Delta < BestDelta)
			{
				BestDelta = Delta;
				SnappedLimit = Rung;
			}
		}
	}

	if (FMath::IsNearlyEqual(Settings->GetFrameRateLimit(), SnappedLimit, 0.5f))
	{
		UE_LOG(LogSiegeGraphics, Verbose, TEXT("[SiegeGraphics] Frame rate limit is already %.0f — no-op."), SnappedLimit);
		return;
	}

	Settings->SetFrameRateLimit(SnappedLimit);
	ApplyQualitySettings();
	BroadcastGraphicsSettingChanged(SettingName_FrameRateLimit);
}

void USiegeGraphicsSettingsSubsystem::SetFrameRateLimitByIndex(int32 Index)
{
	const TArray<float>& Ladder = SiegeGraphicsInternal::FrameRateLadder();
	if (!Ladder.IsValidIndex(Index))
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[SiegeGraphics] SetFrameRateLimitByIndex(%d) refused — the ladder holds %d entries."), Index, Ladder.Num());
		return;
	}

	SetFrameRateLimit(Ladder[Index]);
}

// ═════════════════════════════════════════════════════════════════════════════
// TIER D — THIS PROJECT'S OWN LEVERS (pure, side-effect-free; GFX-§9)
// ═════════════════════════════════════════════════════════════════════════════

float USiegeGraphicsSettingsSubsystem::GetFoliageDensityScale() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		// ⛔ FULL DENSITY ON FAILURE. A failed subsystem lookup must never silently
		// thin the field — that would also make the field differ between two
		// machines for the same seed, which is the M8/D9 hazard named in the header.
		return 1.0f;
	}

	switch (ReadQualityGroupLevel(Settings, GroupName_FoliageQuality))
	{
	case 0:  return 0.25f;   // Low
	case 1:  return 0.50f;   // Medium
	case 2:  return 0.75f;   // High
	case 3:  return 1.00f;   // Epic — DA_BattlefieldScatter's AUTHORED values (GFX-§9)
	case 4:  return 1.00f;   // Cinematic — ⛔ never ABOVE the authored baseline
	default: return 1.00f;
	}
}

float USiegeGraphicsSettingsSubsystem::GetViewDistanceScale() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return 1.0f;
	}

	// ⛔ 1.0 AT EVERY LEVEL IN V1, AND THAT IS A MEASUREMENT RATHER THAN A STUB.
	// [ViewDistanceQuality@N] already sets r.ViewDistanceScale (0.4 at @0
	// BaseScalability.ini:112 → 1.0 at @3 :124), and that CVar already scales
	// HISM cull distances. A second project-side multiplier on
	// SetCullDistances (BattlefieldScatter.cpp:1003, :1092) would COMPOUND it:
	// Low would become 0.4 × <our factor>, not 0.4. The View Distance control
	// does not lie (GFX-§9) because the ENGINE lever behind it is real.
	// ⚠️ If TASK-1122 rules a project lever in, replace this with a COMPENSATING
	// table and re-measure the band — TASK-1084's "0.8" is not reproducible from
	// config (SC-§91).
	switch (ReadQualityGroupLevel(Settings, GroupName_ViewDistanceQuality))
	{
	case 0:  return 1.0f;
	case 1:  return 1.0f;
	case 2:  return 1.0f;
	case 3:  return 1.0f;
	case 4:  return 1.0f;
	default: return 1.0f;
	}
}

bool USiegeGraphicsSettingsSubsystem::ShouldEnableVolumetricFog() const
{
	const UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return true; // Fog on — the shipped look.
	}

	// 🚨 DERIVED FROM THE SHADOW GROUP, NOT EFFECTS — GFX-§9 says Effects and the
	// ENGINE DISAGREES. r.VolumetricFog is owned by [ShadowQuality@N]: 0 at @0
	// (BaseScalability.ini:143) and @1 (:178); 1 at @2 (:213), @3 (:251) and
	// @Cine (:289). [EffectsQuality@*] never mentions it.
	// Binding this to Effects would let a player at Shadows=Low + Effects=Epic
	// see NO fog while our named switch insisted it was on — the "control that
	// lies" GFX-§9 was written to prevent, arriving through the ruling itself.
	// The threshold below IS the engine's own boundary, so the two cannot drift.
	// ⚠️ Flagged for the manager as F-3 in handoffs/TASK-1113-programmer.md.
	return ReadQualityGroupLevel(Settings, GroupName_ShadowQuality) >= 2;
}

// ═════════════════════════════════════════════════════════════════════════════
// THE APPLY MODEL (GFX-§4)
// ═════════════════════════════════════════════════════════════════════════════

bool USiegeGraphicsSettingsSubsystem::ApplyQualitySettings()
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// ⛔ ApplyNonResolutionSettings(), ⛔ NOT ApplySettings(). ApplySettings also
	// re-applies the (possibly unconfirmed) resolution AND calls SaveSettings()
	// itself (GameUserSettings.cpp:590-601), defeating both halves of GFX-§4.
	++ApplyNonResolutionSettingsCallCount;
	if (!bSuppressEngineApplyForAutomationTests)
	{
		Settings->ApplyNonResolutionSettings();
	}

	// Quality applies IMMEDIATELY and saves — it cannot soft-lock anyone. The
	// save may still be refused if a video-mode confirmation is outstanding; see
	// RequestSaveSettings.
	RequestSaveSettings();
	return true;
}

bool USiegeGraphicsSettingsSubsystem::RequestSaveSettings()
{
	UGameUserSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	// ⛔⛔ THE GFX-§4 GUARD, ENFORCED STRUCTURALLY. SaveSettings writes
	// ResolutionSizeX/Y and FullscreenMode along with everything else, so saving
	// while a video mode is staged-but-unconfirmed PERSISTS A MODE THE PLAYER MAY
	// NOT BE ABLE TO SEE — and it persists across restarts, with no in-game route
	// back. That is the lockout GFX-§4 exists to prevent, and it would arrive
	// through an innocent quality-slider drag rather than through the resolution
	// control itself.
	if (IsVideoModeChangePending())
	{
		++RefusedSaveWhileVideoModePendingCount;
		bSaveDeferredByVideoModePending = true;
		UE_LOG(LogSiegeGraphics, Log,
			TEXT("[SiegeGraphics] Save REFUSED — a video-mode change is unconfirmed. It will be flushed by ConfirmVideoModeChange() or RevertVideoModeChange() (refusal #%d)."),
			RefusedSaveWhileVideoModePendingCount);
		return false;
	}

	++SaveSettingsCallCount;
	if (!bSuppressEngineApplyForAutomationTests)
	{
		// ⛔ THE ONE AND ONLY UGameUserSettings::SaveSettings CALL SITE IN THIS
		// FILE. Grep it: one hit, below the guard above. That is what makes
		// "SaveSettings is unreachable from the provisional video-mode path" a
		// traceable structural fact rather than a comment (TASK-1114 cl. c).
		Settings->SaveSettings();
	}

	return true;
}

void USiegeGraphicsSettingsSubsystem::BroadcastGraphicsSettingChanged(FName SettingName)
{
	// ⚠️ THE ONLY OnGraphicsSettingsChanged.Broadcast IN THIS FILE. The counter is
	// bumped here and nowhere else, which is what lets an automation test observe
	// "did it broadcast?" without a bound UFUNCTION listener — a dynamic
	// multicast needs one, and a UCLASS cannot be declared in a test .cpp
	// (the USiegeSettingsSubsystem idiom, cloned).
	++GraphicsSettingsChangeBroadcastCount;
	LastBroadcastSettingName = SettingName;

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[SiegeGraphics] '%s' changed — broadcasting OnGraphicsSettingsChanged (broadcast #%d)."),
		*SettingName.ToString(), GraphicsSettingsChangeBroadcastCount);

	OnGraphicsSettingsChanged.Broadcast(SettingName);
}

// ═════════════════════════════════════════════════════════════════════════════
// AUTOMATION-ONLY SEAMS
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsSettingsSubsystem::SetGameUserSettingsForAutomationTests(UGameUserSettings* InSettings)
{
	SettingsOverrideForAutomationTests = InSettings;

	// ⚠️ THE TWO SWITCHES MOVE TOGETHER, ON PURPOSE. Applying against a scratch
	// object would still push global CVars through Scalability::SetQualityLevels
	// and still request a real display-mode change in the live editor. Two
	// independent switches would let a test set one and forget the other, and the
	// symptom would be an editor that changed resolution during a unit test.
	bSuppressEngineApplyForAutomationTests = (InSettings != nullptr);
}

void USiegeGraphicsSettingsSubsystem::SetForceNullGameUserSettingsForAutomationTests(bool bInForceNull)
{
	bForceNullSettingsForAutomationTests = bInForceNull;

	// ⛔ SUPPRESSION IS RE-ARMED WITH THE FORCE-NULL FLAG (TASK-1114 NIT-4).
	// The null test clears the settings override first, which also clears
	// suppression — leaving this instance pointed at the LIVE UGameUserSettings
	// with applies and saves enabled. It is inert today only because the force-null
	// check precedes the GEngine lookup in ResolveSettings. ⚠️ Mutation M12 removes
	// exactly that check, at which point the mutated test would write the
	// developer's real GameUserSettings.ini and push live CVars — a mutation that
	// damages the host is not an acceptable cost of proving a guard. Suppression is
	// never cleared here, only set: a test that wants the live path back calls
	// SetGameUserSettingsForAutomationTests(nullptr) with force-null off.
	if (bInForceNull)
	{
		bSuppressEngineApplyForAutomationTests = true;
	}
}

void USiegeGraphicsSettingsSubsystem::ResetDiagnosticCountersForAutomationTests()
{
	GraphicsSettingsChangeBroadcastCount = 0;
	LastBroadcastSettingName = NAME_None;
	ApplyNonResolutionSettingsCallCount = 0;
	ApplyResolutionSettingsCallCount = 0;
	SaveSettingsCallCount = 0;
	RefusedSaveWhileVideoModePendingCount = 0;
}

#undef LOCTEXT_NAMESPACE
