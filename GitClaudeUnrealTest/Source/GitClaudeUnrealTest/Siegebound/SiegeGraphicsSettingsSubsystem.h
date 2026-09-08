// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SiegeGraphicsSettingsSubsystem.generated.h"

class UGameUserSettings;

/**
 *  The graphics log category (CONVENTIONS GFX-§10 pins the name character-for-
 *  character: LogSiegeGraphics, declared here and defined in the .cpp — the
 *  LogSiegeSettings precedent).
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeGraphics, Log, All);

/**
 *  Fired when a graphics setting's value ACTUALLY CHANGES, carrying WHICH
 *  setting changed (GFX-§10: FOnSiegeGraphicsSettingsChanged /
 *  OnGraphicsSettingsChanged, FName SettingName).
 *
 *  ⛔ NEVER BROADCAST ON A NO-OP. That is a STRUCTURAL property here, not a
 *  promise repeated at call sites: OnGraphicsSettingsChanged.Broadcast appears
 *  exactly ONCE in the .cpp — inside BroadcastGraphicsSettingChanged — and every
 *  mutation in the file READS THE CURRENT VALUE AND RETURNS EARLY when the new
 *  one equals it, BEFORE any apply, any save and any broadcast. The ten quality
 *  groups additionally funnel through a single ApplyQualityGroupLevelInternal,
 *  so the compare cannot be re-derived ten slightly different ways.
 *
 *  The payload for a quality group is the CANONICAL GROUP NAME (see the
 *  GroupName_* constants below) — the same token the ini section, the sg.* CVar
 *  and the GFX-§10 widget triple all use, so a log line, an ini readback and a
 *  delegate payload read alike and stay greppable.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeGraphicsSettingsChanged, FName, SettingName);

/**
 *  THE ONE TESTABLE SEAM OVER UGameUserSettings (TASK-1113; CONVENTIONS GFX-§3,
 *  GFX-§4, GFX-§5, GFX-§8, GFX-§10).
 *
 *  ⛔ NO UGameUserSettings SUBCLASS AND NO GameUserSettingsClassName EDIT TO
 *  Config/DefaultEngine.ini (GFX-§3). Everything below is stock engine API
 *  wrapped in a facade, which is what deletes the silent-fallback failure mode a
 *  mistyped subclass name would have shipped.
 *
 *  ⛔ THE WIDGET NEVER CALLS AN ENGINE API DIRECTLY (GFX-§3, "the seam"). One
 *  facade means one place the automation suite can drive, one place the clamps
 *  live, and one place the apply/save discipline of GFX-§4 is enforced.
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  WHAT TASK-1112 MEASURED THAT SHAPED THIS FILE (handoffs/TASK-1112-programmer.md)
 *  ─────────────────────────────────────────────────────────────────────────
 *
 *  (1) ⚠️ RevertVideoMode() APPLIES NOTHING. Its complete body
 *      (Engine/Private/GameUserSettings.cpp:276-285) restores five member fields
 *      and broadcasts. Without a following ApplyResolutionSettings(false) the
 *      screen KEEPS THE BROKEN MODE while every property read-back reports the
 *      good one — an SC-§94 cl. A lie (the instrument echoes the request, not
 *      the result). RevertVideoModeChange() below does the apply. ⛔ Do not
 *      "simplify" it back to a bare RevertVideoMode() call.
 *
 *  (2) THE ENGINE HAS ELEVEN int32 QUALITY GROUPS; THIS FACADE SHIPS TEN.
 *      The eleventh is LandscapeQuality (GameUserSettings.h:291/:295). It is
 *      DROPPED, and the grounds are measured rather than stylistic:
 *        - Engine/Config/BaseScalability.ini contains NO [LandscapeQuality@N]
 *          section at ANY level (all ten others have @0 @1 @2 @3 @Cine).
 *        - [ScalabilitySettings] carries no PerfIndexThresholds_LandscapeQuality,
 *          so auto-detect cannot even derive it.
 *        - This project has NO Landscape; the arena floor is a static mesh.
 *      ⇒ A Landscape slider would move, write sg.LandscapeQuality, and change
 *      NOTHING — precisely the "control that lies" GFX-§9 forbids. Recorded here
 *      so nobody re-adds it from a UE docs page later.
 *
 *  (3) THE CANONICAL GROUP-NAME PIN (GFX-§10 owed one; this is where it is paid).
 *      Two concepts carry FOUR spellings each and the engine wrappers cross them
 *      silently:
 *          UGameUserSettings::SetVisualEffectQuality  -> FQualityLevels::SetEffectsQuality
 *              -> member EffectsQuality      -> ini [EffectsQuality@N] / sg.EffectsQuality
 *          UGameUserSettings::SetPostProcessingQuality -> FQualityLevels::SetPostProcessQuality
 *              -> member PostProcessQuality  -> ini [PostProcessQuality@N] / sg.PostProcessQuality
 *      ⛔ CANONICAL = THE INI / CVar / sg.* SPELLING, everywhere: "EffectsQuality"
 *      and "PostProcessQuality". Grounds: that is what a LEVEL actually means and
 *      what a log line or an ini readback will show, so the delegate payload, the
 *      GFX-§10 widget triple (EffectsQualitySlider / EffectsQualityLabelText /
 *      EffectsQualityValueText) and the file on disk all read alike. The engine's
 *      other two spellings appear in this codebase at FOUR SITES TOTAL, ⛔ ALL
 *      FOUR INSIDE THE TWO FUNNELS — the READ forwarding lines in
 *      ReadQualityGroupLevel and the WRITE forwarding lines in
 *      ApplyQualityGroupLevelInternal, i.e. one site per SYMBOL per funnel, two
 *      per CONCEPT — and all four are commented in place as
 *      "NAME CROSSING #1/#2 OF 2 (read side)" and "… (write side)".
 *      (Corrected from "exactly one site each" by TASK-1114 NIT-1, which measured
 *      four; this header, the cpp banner and the handoff each said it a different
 *      way.) ⛔ Nothing else in the project may use them.
 *      GFX-§10's own worked example (ShadowQualitySlider) already uses this
 *      spelling, so <Group> == the ini section name minus "@N".
 *
 *  (4) ⚠️ THE RESOLUTION-SCALE FLOOR IS 0.0, NOT 50 (Scalability.h:242), AND
 *      THIS MACHINE'S LIVE INI HOLDS sg.ResolutionQuality=0 — which
 *      BaseScalability.ini:39-42 defines as a SENTINEL meaning "use the
 *      project's default screen percentage", NOT "render at 0%".
 *      ⇒ THIS FACADE CLAMPS ON WRITE AND NEVER ON READ. Clamping on load would
 *      silently rewrite that sentinel to a hard 50% the first time the player
 *      OPENS the menu without touching anything — changing their picture as a
 *      side effect of looking at it. See IsResolutionScaleProjectDefault().
 *
 *  (5) GetSupportedFullscreenResolutions is NOT on UGameUserSettings. It is a
 *      static returning bool on UKismetSystemLibrary (KismetSystemLibrary.h:1786),
 *      with a windowed sibling. Both failure returns are handled.
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  NULL-SAFETY CONTRACT (TASK-1113 cl. 1)
 *  ─────────────────────────────────────────────────────────────────────────
 *  Every public entry point resolves through ResolveSettings(), which returns
 *  nullptr when GEngine is null, when GEngine->GetGameUserSettings() is null, or
 *  when a test has forced the null path. On null:
 *      - every GETTER returns the documented fallback named on its declaration,
 *      - every SETTER / apply is a no-op that returns false and broadcasts NOTHING,
 *      - the failure logs ONCE per subsystem instance (latched), never per frame,
 *      - nothing crashes and nothing dereferences.
 *  ⚠️ THE FALLBACKS DELIBERATELY PRESERVE THE AUTHORED BASELINE (Epic / full
 *  density / fog on). A lookup failure must never silently DOWNGRADE the game —
 *  a thinner scatter caused by a failed subsystem lookup would also be a
 *  server-vs-client determinism hazard (see GetFoliageDensityScale).
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  HOW CONSUMERS RESOLVE IT (the USiegeSettingsSubsystem snippet, unchanged)
 *  ─────────────────────────────────────────────────────────────────────────
 *      UGameInstance* GI = GetGameInstance();          // or Actor->GetGameInstance()
 *      USiegeGraphicsSettingsSubsystem* Graphics =
 *          GI ? GI->GetSubsystem<USiegeGraphicsSettingsSubsystem>() : nullptr;
 *      const float Density = Graphics ? Graphics->GetFoliageDensityScale() : 1.0f;
 *
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no new
 *  relevancy tier. Every value here is per-machine by construction (GFX-§3) and
 *  governs LOCAL presentation only. ⚠️ THAT IS ALSO A CONSTRAINT ON CONSUMERS:
 *  see the determinism warning on GetFoliageDensityScale().
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeGraphicsSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// ─────────────────────────── PINNED CONSTANTS ───────────────────────────

	/** Lowest quality level (Low). Every group's band is [0, 4] — TASK-1112: sg.<Group>.NumLevels defaults to 5 for every group. */
	static constexpr int32 MinQualityLevel = 0;

	/** Highest quality level (Cinematic). ⛔ 4, not 3 — the engine's fifth rung is spelled "@Cine" in the ini, not "@4". */
	static constexpr int32 MaxQualityLevel = 4;

	/** 5 detents (GFX-§5: StepSize 0.25 over a USlider). Measured, not assumed: Scalability.cpp:1267-1283. */
	static constexpr int32 NumQualityLevels = 5;

	/**
	 *  The engine-native "the groups disagree" state (FQualityLevels::GetSingleQualityLevel,
	 *  documented "-1:custom", Scalability.h:106). ⛔ GFX-§5's Custom is ENGINE-NATIVE —
	 *  do not invent a parallel bCustom flag that can drift from it.
	 */
	static constexpr int32 CustomQualityLevel = -1;

	/**
	 *  The fallback level every group getter returns when UGameUserSettings does
	 *  not resolve: 3 (Epic). It is the MEASURED live state of this machine's
	 *  GameUserSettings.ini (TASK-1112 §1.6 — all ten groups at 3), and it is the
	 *  authored baseline the content was built against.
	 */
	static constexpr int32 FallbackQualityLevel = 3;

	/** GFX-§5's honest band for the one genuinely continuous control. ⛔ Enforced on WRITE only — see IsResolutionScaleProjectDefault(). */
	static constexpr float MinResolutionScalePercent = 50.0f;
	static constexpr float MaxResolutionScalePercent = 100.0f;

	/**
	 *  ⚠️ THE SENTINEL, NOT A VALUE. sg.ResolutionQuality == 0 means "use the
	 *  project's default screen percentage" (BaseScalability.ini:39-42
	 *  +ResolutionPresets=(Name="Default",ResolutionQuality=0.0)), NOT "render at
	 *  0%". Compared with a small epsilon because the underlying store is a float.
	 */
	static constexpr float ResolutionScaleSentinelEpsilon = 0.001f;

	/** What a slider shows while the sentinel is live: 100%. The project default screen percentage is 100. */
	static constexpr float ResolutionScaleSentinelDisplayPercent = 100.0f;

	// ──────────────────── THE CANONICAL GROUP NAMES (the GFX-§10 pin) ────────────────────
	//
	// ⛔ THESE TEN STRINGS ARE THE PIN. They are the ini section name minus "@N",
	// the sg.* CVar suffix, the delegate payload, and the <Group> half of every
	// GFX-§10 widget triple. An automation test asserts each one character-for-
	// character, because a drifted name here silently disconnects a slider from
	// the setting it claims to drive — with no error anywhere.

	static const FName GroupName_ViewDistanceQuality;
	static const FName GroupName_AntiAliasingQuality;
	static const FName GroupName_ShadowQuality;
	static const FName GroupName_GlobalIlluminationQuality;
	static const FName GroupName_ReflectionQuality;
	static const FName GroupName_PostProcessQuality;   // ⚠️ NOT "PostProcessingQuality" — see the class comment, note (3).
	static const FName GroupName_TextureQuality;
	static const FName GroupName_EffectsQuality;       // ⚠️ NOT "VisualEffectQuality" — see the class comment, note (3).
	static const FName GroupName_FoliageQuality;
	static const FName GroupName_ShadingQuality;

	/** Delegate payload names for the non-group controls. Same greppability rule as the groups. */
	static const FName SettingName_OverallQuality;
	static const FName SettingName_ResolutionScale;
	static const FName SettingName_ScreenResolution;
	static const FName SettingName_WindowMode;
	static const FName SettingName_VSync;
	static const FName SettingName_FrameRateLimit;
	static const FName SettingName_AutoDetect;

	/**
	 *  The ten canonical group names IN DISPLAY ORDER (GFX-§8's Tier-B order:
	 *  View Distance, Anti-Aliasing, Shadows, Global Illumination, Reflections,
	 *  Post Processing, Textures, Effects, Foliage, Shading).
	 *
	 *  ⛔ THIS IS THE LIST TASK-1115 LOOPS OVER to build its ten rows, and the
	 *  list TASK-1122 reads Tier-D levels from. Adding a group means adding it
	 *  here, in ApplyQualityGroupLevelInternal and in GetQualityGroupLevel — a
	 *  test asserts the three agree, so a half-added group fails rather than
	 *  ships a slider wired to nothing.
	 *
	 *  ⚠️ RETURNED BY VALUE, not by const reference: UnrealHeaderTool rejects a
	 *  const-reference RETURN type on a Blueprint-exposed function. Ten FNames.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static TArray<FName> GetQualityGroupNames();

	/** Player-facing label for a canonical group name ("View Distance", "Anti-Aliasing", …). NAME_None-safe; an unknown name returns the name itself. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static FText GetQualityGroupDisplayName(FName GroupName);

	/**
	 *  "Low" / "Medium" / "High" / "Epic" / "Cinematic" for a 0-4 level, from
	 *  Scalability::GetScalabilityNameFromQualityLevel (Scalability.h:251).
	 *  ⛔ THE ENGINE ALREADY LOCALISES THESE — GFX-§5's "live level name beside
	 *  it" uses this, never a hand-written array that would drift from the ini.
	 *  CustomQualityLevel (-1) returns "Custom".
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static FText GetQualityLevelDisplayName(int32 QualityLevel);

	// ──────────────────────── TIER B: THE TEN QUALITY GROUPS ────────────────────────
	//
	// One BlueprintPure getter + one BlueprintCallable setter per group
	// (TASK-1113 cl. 2). Every getter falls back to FallbackQualityLevel (3);
	// every setter forwards to the ONE mutation path, which clamps to [0, 4],
	// refuses no-ops, applies and saves.

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetViewDistanceQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetViewDistanceQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetAntiAliasingQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetAntiAliasingQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetShadowQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetShadowQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetGlobalIlluminationQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetGlobalIlluminationQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetReflectionQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetReflectionQuality(int32 NewLevel);

	/** ⚠️ Canonical spelling. The engine's own wrapper is SetPostProcessingQuality — crossed at exactly one site. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetPostProcessQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetPostProcessQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetTextureQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetTextureQuality(int32 NewLevel);

	/** ⚠️ Canonical spelling. The engine's own wrapper is SetVisualEffectQuality — crossed at exactly one site. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetEffectsQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetEffectsQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetFoliageQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetFoliageQuality(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics") int32 GetShadingQuality() const;
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics") void SetShadingQuality(int32 NewLevel);

	/**
	 *  GENERIC, FName-KEYED ACCESS — what TASK-1115 loops over so it builds ten
	 *  rows without ten copies of the same code, and without ANY enum crossing a
	 *  Blueprint boundary (GFX-§2(d) forbids enums and structs in a
	 *  BlueprintImplementableEvent parameter; there is deliberately no UENUM in
	 *  this class at all, so that rule cannot be broken downstream).
	 *
	 *  An unknown GroupName returns FallbackQualityLevel and logs a Warning — it
	 *  is a programming error, not a player action.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	int32 GetQualityGroupLevel(FName GroupName) const;

	/** Generic setter. Clamps to [0, 4], refuses no-ops, applies + saves. An unknown GroupName is refused and logged. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetQualityGroupLevel(FName GroupName, int32 NewLevel);

	// ──────────────────────── TIER A: THE OVERALL PRESET ────────────────────────

	/**
	 *  0-4, or CustomQualityLevel (-1) when the ten groups DISAGREE. Straight
	 *  through to UGameUserSettings::GetOverallScalabilityLevel
	 *  (GameUserSettings.cpp:971-974), which is FQualityLevels::GetSingleQualityLevel.
	 *  Fallback when unresolvable: FallbackQualityLevel (3).
	 *
	 *  ⚠️ THE ENGINE ANSWERS THIS OVER ALL ELEVEN GROUPS, INCLUDING THE
	 *  LandscapeQuality WE DO NOT EXPOSE. SetOverallScalabilityLevel writes all
	 *  eleven to the same value, so a preset press always agrees.
	 *
	 *  ⛔ BUT "IT CAN ONLY DIFFER IF SOMETHING OUTSIDE THIS FACADE MOVES IT" WAS
	 *  TOO STRONG, AND AutoDetectQuality() — IN THIS VERY CLASS — IS SUCH A MOVER
	 *  (TASK-1114 WARN-6). RunHardwareBenchmark → Scalability::BenchmarkQualityLevels
	 *  → ComputeQualityLevelsFromPerfIndex DOES compute LandscapeQuality
	 *  (Scalability.cpp:727), and with no PerfIndexThresholds_LandscapeQuality in
	 *  this project's ini, ComputeOptionFromPerfIndex falls back to its HARD-CODED
	 *  {20, 50, 70} thresholds (:210-214) while the other ten use their own table.
	 *  ⇒ The invisible eleventh can land on a DIFFERENT level from the visible ten,
	 *  and this getter then reports Custom with all ten sliders in agreement —
	 *  permanently, until a preset is pressed. (The benchmark's ResolutionQuality is
	 *  a second, independent route to the same reading; see the paragraph below.)
	 *  ⛔ THEREFORE TASK-1115 MUST LABEL "Custom" FROM
	 *  IsOverallQualityCustomAcrossVisibleGroups(), not from this getter. That is
	 *  now MANDATORY rather than stylistic.
	 *
	 *  ⚠️ AND A SECOND, LOUDER SURPRISE THAT TASK-1115 MUST NOT READ AS A BUG:
	 *  FQualityLevels::GetSingleQualityLevel (Scalability.cpp:1083-1097) requires
	 *  ResolutionQuality to equal the preset's canonical render scale AS WELL AS
	 *  all eleven groups agreeing. ⇒ DRAGGING THE RESOLUTION SCALE SLIDER ALONE
	 *  PUTS THE OVERALL PRESET INTO Custom while every group slider still agrees.
	 *  That is defensible — the picture really is no longer stock Epic — but a
	 *  panel that showed "Custom" with ten matching sliders and no explanation
	 *  would look broken. Both readings are pinned by an automation test; the
	 *  widget should decide deliberately which one it labels.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	int32 GetOverallScalabilityLevel() const;

	/**
	 *  Sets all groups to one level (clamped to [0, 4]), then applies + saves. A
	 *  no-op write broadcasts nothing.
	 *
	 *  ⛔ A PRESET PRESS ALSO OVERWRITES THE PLAYER'S RESOLUTION SCALE, AND ON A
	 *  FIRST-EVER RUN IT DESTROYS THE sg.ResolutionQuality=0 SENTINEL
	 *  (TASK-1114 WARN-5). The engine's FQualityLevels::SetFromSingleQualityLevel
	 *  writes ResolutionQuality from PerfIndexValues_ResolutionQuality
	 *  = 50 71 87 100 100 (Scalability.cpp:1047). That is engine-native and this
	 *  facade does not fight it — but ⇒ ONE WRITE MOVES TWO PLAYER-VISIBLE VALUES,
	 *  so this function broadcasts SettingName_OverallQuality AND, only if the
	 *  scale really moved, SettingName_ResolutionScale. ⚠️ TASK-1115: expect a
	 *  preset press to move the Resolution-Scale row, and do not treat that second
	 *  broadcast as a spurious one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetOverallScalabilityLevel(int32 NewLevel);

	/** GFX-§5's read-only Custom state: true when GetOverallScalabilityLevel() == -1. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsOverallQualityCustom() const;

	/**
	 *  The same question asked over the TEN GROUPS THIS FACADE SHIPS, ignoring
	 *  the dropped LandscapeQuality. This is what the panel's "Custom" label
	 *  should read, because it is the only version of the question the player can
	 *  actually see the inputs to.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsOverallQualityCustomAcrossVisibleGroups() const;

	/**
	 *  GFX-§6's Auto-Detect button, behind ONE call: RunHardwareBenchmark() then
	 *  ApplyHardwareBenchmarkResults(). Returns false when unresolvable.
	 *
	 *  ⚠️ IT STALLS FOR A SECOND OR TWO — GFX-§6 rules it a BUTTON and ⛔ NEVER a
	 *  first-boot action, for exactly that reason.
	 *  ⛔ REFUSED (returns false, nothing runs) WHILE IsVideoModeChangePending().
	 *  ⚠️ RunHardwareBenchmark POPULATES ONLY and applies nothing
	 *  (GameUserSettings.cpp:1122-1130). ApplyHardwareBenchmarkResults applies and
	 *  then saves TWICE — its body is :1132-1143 in full:
	 *      :1139  Scalability::SaveState()  → [ScalabilityGroups] only
	 *      :1142  SaveSettings()            → the FULL config save (SaveConfig at
	 *             :683), which writes ResolutionSizeX/Y (GameUserSettings.h:465)
	 *             and FullscreenMode (:500).
	 *  ⇒ Auto-Detect CAN persist an unconfirmed video mode, entirely around
	 *  RequestSaveSettings' guard, so this function carries its own GFX-§4 refusal
	 *  (TASK-1114 BLOCKER-1). ⛔ An earlier revision of this comment stopped the
	 *  trace at :1140 and concluded the opposite; do not shorten the range back.
	 *  ⚠️ IT CAN ALSO LEAVE THE PANEL READING "Custom" WITH ALL TEN SLIDERS
	 *  AGREEING (TASK-1114 WARN-6). BenchmarkQualityLevels →
	 *  ComputeQualityLevelsFromPerfIndex computes the INVISIBLE eleventh group too
	 *  (LandscapeQuality, Scalability.cpp:727), and with no
	 *  PerfIndexThresholds_LandscapeQuality in this project's ini it falls back to
	 *  ComputeOptionFromPerfIndex's HARD-CODED {20, 50, 70} (:210-214) while the
	 *  other ten use their own table ⇒ the eleventh can land on a different level
	 *  and GetOverallScalabilityLevel() then reports Custom permanently, until a
	 *  preset is pressed. The benchmark's ResolutionQuality is a second,
	 *  independent route to the same reading. ⇒ TASK-1115 labels from
	 *  IsOverallQualityCustomAcrossVisibleGroups(), NEVER from
	 *  GetOverallScalabilityLevel().
	 *  ⚠️ This machine has NEVER run it (LastCPUBenchmarkResult=-1,
	 *  LastGPUBenchmarkResult=-1, TASK-1112 §1.6) — the first press is a cold path.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	bool AutoDetectQuality();

	// ──────────────────────── TIER C: RESOLUTION SCALE ────────────────────────

	/**
	 *  THE RAW, HONEST READ: normalized 0..1, exactly as the engine holds it, with
	 *  ⛔ NO CLAMP. Returns 0.0 when the sg.ResolutionQuality=0 SENTINEL is live —
	 *  which is the state this machine is in right now.
	 *
	 *  ⛔ DO NOT CLAMP THIS. Clamping on read is the TASK-1112 §1.3(b) trap: the
	 *  first time a player OPENED the graphics panel, the sentinel would be
	 *  rewritten to a hard 50% and their picture would change as a side effect of
	 *  looking at the menu. Clamping happens on WRITE, in SetResolutionScaleNormalized.
	 *
	 *  Measured: normalized == percent / 100 EXACTLY (min 0, max 100, a plain
	 *  lerp — GameUserSettings.cpp:976-981 / Scalability.h:242,245).
	 *  Fallback when unresolvable: 1.0 (100%, the authored baseline).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetResolutionScaleNormalized() const;

	/** The same raw value as a percentage 0..100. ⛔ Also unclamped, for the same reason. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetResolutionScalePercent() const;

	/**
	 *  TRUE when the underlying value is the "use the project default screen
	 *  percentage" SENTINEL (0), rather than a real player-chosen percentage.
	 *  ⛔ A UI THAT CANNOT TELL THESE APART WILL EITHER DISPLAY "0%" (a lie about
	 *  what is rendering) OR SILENTLY OVERWRITE THE SENTINEL (a change the player
	 *  did not make). This getter is how TASK-1115 avoids both.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsResolutionScaleProjectDefault() const;

	/**
	 *  What a 50-100% slider should be SEEDED with: the raw percent clamped into
	 *  the GFX-§5 band, or ResolutionScaleSentinelDisplayPercent (100) while the
	 *  sentinel is live. ⛔ THIS IS A DISPLAY TRANSFORM AND WRITES NOTHING — the
	 *  underlying sentinel survives until the player actually moves the slider.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetResolutionScalePercentForSlider() const;

	/** Clamps to [0.5, 1.0] ON WRITE, then applies + saves. A no-op write broadcasts nothing. ⛔ Can never write the sentinel back. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetResolutionScaleNormalized(float NewScaleNormalized);

	/** Percent form of the above (GFX-§5: 1% steps). Clamps to [50, 100] on write. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetResolutionScalePercent(float NewScalePercent);

	// ──────────────────────── TIER C: DISPLAY MODE ────────────────────────
	//
	// ⚠️ THE ONLY CONTROLS IN THIS FILE THAT CAN LOCK A PLAYER OUT OF THE GAME.
	// GFX-§4: apply, then a 10-second "Keep these settings?" that AUTO-REVERTS.
	// The countdown is the widget's (TASK-1118); the apply/confirm/revert
	// discipline is here, because that is where it can be tested.

	/** Current staged resolution. Fallback when unresolvable: 1280x720 (the engine's own SetToDefaults DesiredScreen value). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	FIntPoint GetScreenResolution() const;

	/**
	 *  STAGES a resolution. ⛔ APPLIES NOTHING and SAVES NOTHING — call
	 *  ApplyVideoModeProvisional() next, then Confirm/Revert. Splitting stage
	 *  from apply is what lets the widget change resolution AND window mode in
	 *  one provisional step instead of two screen flashes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetScreenResolution(FIntPoint NewResolution);

	/**
	 *  The supported modes for the CURRENT window mode: the windowed list when
	 *  windowed, the fullscreen list otherwise. TASK-1112 §1.5 — the fullscreen
	 *  list is derived from the monitor's mode list and is the WRONG SET for a
	 *  window.
	 *
	 *  ⚠️ UKismetSystemLibrary::GetSupportedFullscreenResolutions RETURNS bool
	 *  (KismetSystemLibrary.h:1786). On failure — or on an empty list — this
	 *  returns a single-entry array holding the CURRENT resolution, so a stepper
	 *  is never empty and never divides by zero. The failure logs once.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	TArray<FIntPoint> GetSupportedScreenResolutions() const;

	/** Stepper support for TASK-1115: how many entries GetSupportedScreenResolutions() has. Always >= 1. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	int32 GetSupportedScreenResolutionCount() const;

	/** Stepper support: "1920 x 1080" for an index. Out-of-range returns an empty string rather than asserting. ⛔ FString, not a struct — BIE-safe (GFX-§2(d)). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	FString GetSupportedScreenResolutionLabel(int32 Index) const;

	/** Stepper support: index of the current resolution in the supported list, or 0 when it is not in the list. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	int32 FindCurrentScreenResolutionIndex() const;

	/** Stepper support: stages the resolution at an index. Out-of-range is refused and logged. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetScreenResolutionByIndex(int32 Index);

	/**
	 *  Window mode AS int32 (0 Fullscreen, 1 Borderless/WindowedFullscreen,
	 *  2 Windowed). ⛔ MARSHALLED AS int32 ON PURPOSE: EWindowMode::Type is a
	 *  plain C++ enum and GFX-§2(d) forbids an enum in a
	 *  BlueprintImplementableEvent parameter. Keeping the facade's own type
	 *  int32 means the widget CANNOT get this wrong.
	 *  Fallback when unresolvable: 1 (WindowedFullscreen — the measured live
	 *  value in this machine's ini, TASK-1112 §1.6).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	int32 GetWindowMode() const;

	/** STAGES a window mode (clamped to [0, 2]). ⛔ Applies nothing, saves nothing — see SetScreenResolution. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetWindowMode(int32 NewWindowMode);

	/** 3 — EWindowMode::NumWindowModes. Asserted against the engine enum in the .cpp with a static_assert. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static int32 GetWindowModeCount();

	/** "Fullscreen" / "Borderless Window" / "Windowed". Out-of-range returns an empty string. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static FString GetWindowModeLabel(int32 WindowMode);

	/**
	 *  GFX-§4's provisional apply: pushes the STAGED resolution + window mode to
	 *  the display and ⛔ DOES NOT SAVE. Returns false when unresolvable.
	 *
	 *  ⛔ IT USES ApplyResolutionSettings(false), ⛔ NEVER ApplySettings(). That is
	 *  not a style choice: UGameUserSettings::ApplySettings CALLS SaveSettings()
	 *  ON ITS LAST LINE (GameUserSettings.cpp:590-601), so routing the provisional
	 *  path through it would persist an unconfirmed video mode — the exact
	 *  lockout GFX-§4 exists to prevent.
	 *
	 *  After this returns true, IsVideoModeChangePending() is true and every save
	 *  in this facade is REFUSED until Confirm or Revert (see RequestSaveSettings).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	bool ApplyVideoModeProvisional();

	/**
	 *  TRUE while there is a video-mode change the player has not confirmed —
	 *  either a provisional apply is live, or the staged resolution / window mode
	 *  simply DIFFERS from the last confirmed one. Every save in this facade is
	 *  refused while it is true.
	 *
	 *  ⚠️ IT DELIBERATELY DOES NOT USE THE ENGINE'S IsScreenResolutionDirty() /
	 *  IsFullscreenModeDirty(). Those compare the staged value against the LIVE
	 *  VIEWPORT (GSystemResolution / Viewport->GetWindowMode()) and BOTH RETURN
	 *  false WHEN THERE IS NO GameViewport (GameUserSettings.cpp:219-239) — so in
	 *  a headless run, a commandlet or an automation test they read "clean" no
	 *  matter what is staged. A guard built on them would be untestable AND
	 *  silently absent exactly where it matters least visibly. This compares
	 *  staged-vs-LastConfirmed instead, which is the question GFX-§4 is actually
	 *  asking and which holds with no viewport at all.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsVideoModeChangePending() const;

	/** The player pressed "Keep": ConfirmVideoMode() stamps the LastConfirmed* fields, then the save is released and flushed. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	bool ConfirmVideoModeChange();

	/**
	 *  The player pressed "Revert", or the countdown expired.
	 *
	 *  🚨 RevertVideoMode() ALONE IS NOT A REVERT. Its complete engine body
	 *  (GameUserSettings.cpp:276-285) restores five member fields and broadcasts —
	 *  it pushes NOTHING to the display. This function therefore calls
	 *  RevertVideoMode() AND THEN ApplyResolutionSettings(false), which is what
	 *  actually puts the screen back.
	 *
	 *  ⛔ Deleting that second call ships a screen the player cannot read while
	 *  every property read-back reports the good mode (SC-§94 cl. A). An
	 *  automation test asserts the resolution apply is reached on this path, and
	 *  the SC-§83 mutation for that test is "delete the ApplyResolutionSettings
	 *  call from RevertVideoModeChange".
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	bool RevertVideoModeChange();

	// ──────────────────────── TIER C: VSYNC + FRAME RATE ────────────────────────

	/** ⚠️ The engine spells its getter IsVSyncEnabled, not GetVSync. Fallback when unresolvable: false (the measured live value). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsVSyncEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetVSyncEnabled(bool bEnable);

	/** ⚠️ float, not int32, and 0 means UNLIMITED. Fallback when unresolvable: 0. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetFrameRateLimit() const;

	/** Snaps to the GFX-§5 ladder before writing — an off-ladder value would make the stepper unable to represent its own state. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetFrameRateLimit(float NewLimit);

	/** GFX-§5's fixed ladder, in order: 30, 60, 90, 120, 144, 165, 240, 0 (Unlimited LAST). ⚠️ By value — UHT rejects a const-ref return on a BP function. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static TArray<float> GetFrameRateLimitLadder();

	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static int32 GetFrameRateLimitOptionCount();

	/** "Unlimited" for 0, otherwise "144 FPS". */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	static FString GetFrameRateLimitLabel(float Limit);

	/** Index of the current limit on the ladder, or the Unlimited index when the live value is off-ladder. Never -1. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	int32 FindCurrentFrameRateLimitIndex() const;

	/** Stepper support: sets the limit at a ladder index. Out-of-range is refused and logged. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void SetFrameRateLimitByIndex(int32 Index);

	// ──────────────────────── TIER D: THIS PROJECT'S OWN LEVERS ────────────────────────
	//
	// TASK-1113 cl. 5 ships these HERE, upstream of both consumers, so TASK-1115
	// (the widgets) and TASK-1122 (BattlefieldScatter + the fog site) never touch
	// the same file. That is what makes those two rows parallel-safe.
	//
	// ⛔ ALL THREE ARE PURE AND SIDE-EFFECT-FREE: they read a level and return a
	// derived value. No apply, no save, no broadcast, no CVar write, no logging
	// on the success path.
	//
	// ⛔ GFX-§9: READ AT SPAWN / BUILD TIME, NEVER PER FRAME. Measured build
	// trigger (TASK-1112 §3.2): ASiegeBattlefieldScatter::RunScatterPasses,
	// BattlefieldScatter.cpp:363 — the single funnel both the authority path
	// (:322) and the client mirror (:360) pass through, before any placement.

	/**
	 *  Foliage/scatter density multiplier derived from the Foliage group:
	 *      Low 0.25 · Medium 0.50 · High 0.75 · Epic 1.00 · Cinematic 1.00
	 *
	 *  Epic is 1.00 because DA_BattlefieldScatter's AUTHORED values ARE the Epic
	 *  baseline (GFX-§9) and that asset is never edited by this lane. Cinematic
	 *  is also 1.00 — it never EXCEEDS the authored baseline, because "the
	 *  authored values are the maximum" is what makes them a baseline at all.
	 *  Fallback when unresolvable: 1.0 (full density — a failed lookup must never
	 *  silently thin the field).
	 *
	 *  🚨 A CONSTRAINT ON THE CONSUMER, NOT A SUGGESTION (TASK-1112 §3.4).
	 *  THIS IS A PER-MACHINE VALUE (GFX-§3) AND THE SCATTER IS UNDER AN M8/D9
	 *  SERVER-CLIENT DETERMINISM CONTRACT. A client does not self-generate: it
	 *  mirrors the authority's seed through OnRep_GenerationIndex
	 *  (BattlefieldScatter.cpp:335) and both machines must produce the SAME field
	 *  from it. Multiplying a layer's InstanceCount by this value changes the
	 *  ITERATION COUNT of a loop that draws from a SHARED FRandomStream (:637,
	 *  :647, :648, :660) ⇒ a server at Foliage=Epic and a client at Foliage=Low
	 *  produce two DIFFERENT battlefields from one seed, and every layer
	 *  processed AFTER the scaled one is displaced too.
	 *  ⇒ ⛔ A CONSUMER MAY NOT LET THIS VALUE CHANGE THE RNG DRAW SEQUENCE.
	 *  The safe shapes are render-side only (cull band; place-then-skip-the-
	 *  AddInstance for a deterministically chosen subset). TASK-1122 owes the
	 *  ruling; this getter owes the warning, and here it is.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetFoliageDensityScale() const;

	/**
	 *  THE PROJECT-SIDE cull-band multiplier derived from the View Distance group.
	 *  ⛔ IT IS 1.0 AT EVERY LEVEL IN V1, AND THAT IS A MEASUREMENT, NOT A STUB.
	 *
	 *  TASK-1112 §3.5(b): [ViewDistanceQuality@N] already sets r.ViewDistanceScale
	 *  (0.4 at @0 BaseScalability.ini:112, 1.0 at @3 :124), and that CVar ALREADY
	 *  scales primitive draw distances INCLUDING HISM cull distances. So the View
	 *  Distance slider ALREADY moves this scatter's effective band with zero
	 *  project code. Layering a second multiplier on CullStartDistance /
	 *  CullEndDistance (BattlefieldScatter.cpp:1003, :1092) would COMPOUND it —
	 *  Low would become 0.4 x <our factor>, not 0.4.
	 *
	 *  ⇒ The honest project-side factor today is 1.0, and the control does NOT
	 *  lie (GFX-§9) because the ENGINE lever behind it is real and measured.
	 *  ⚠️ If TASK-1122 rules a project lever in, it must ship a COMPENSATING
	 *  table (target / engine factor), and it must re-measure the band rather
	 *  than inherit TASK-1084's "~72 m at r.ViewDistanceScale 0.8" — that 0.8 is
	 *  NOT reproducible from config (r.ViewDistanceScale appears nowhere in
	 *  Config/ or Source/, and @3 sets it to 1.0). SC-§91: a relayed number is a
	 *  lower bound. An automation test pins 1.0 across all five levels so a
	 *  silent change to this table cannot ship unnoticed.
	 *  Fallback when unresolvable: 1.0.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetViewDistanceScale() const;

	/**
	 *  Whether volumetric fog should be on, derived from the SHADOW group:
	 *  true at High/Epic/Cinematic (>= 2), false at Low/Medium.
	 *
	 *  🚨 GFX-§9 BINDS FOG TO THE EFFECTS GROUP AND THE ENGINE DISAGREES —
	 *  THE MEASUREMENT IS FOLLOWED HERE AND THE CONFLICT IS FLAGGED FOR THE
	 *  MANAGER (handoffs/TASK-1113-programmer.md, flagged decision F-3).
	 *  r.VolumetricFog is owned by [ShadowQuality@N]: 0 at @0 (:143) and @1
	 *  (:178); 1 at @2 (:213), @3 (:251) and @Cine (:289), with a graduated
	 *  froxel grid. [EffectsQuality@*] NEVER mentions r.VolumetricFog.
	 *
	 *  ⛔ DERIVING THIS FROM EFFECTS WOULD SHIP A LIVE CONTRADICTION: a player at
	 *  Shadows=Low + Effects=Epic gets r.VolumetricFog=0 from stock engine
	 *  scalability while our named switch insists the fog is on — the fog would
	 *  be GONE with the control saying otherwise, which is the failure mode
	 *  GFX-§9 was written to prevent, arriving through the ruling itself. The
	 *  threshold (>= 2) is the engine's own boundary, so the two cannot disagree.
	 *  Fallback when unresolvable: true (fog on — the shipped look).
	 *
	 *  ⚠️ ALSO FOR TASK-1122: GFX-§9 names the symbol "bEnableVolumetricFog
	 *  (shipped 12b8707)", and that symbol occurs ZERO times in Source/, Config/
	 *  and Tools/. It is a UExponentialHeightFogComponent ENGINE property, not a
	 *  project symbol. Under GFX-§11 (L_Arena.umap is never saved, zero .uasset
	 *  writes) any consumer must act at RUNTIME — e.g. set the r.VolumetricFog
	 *  CVar at build time — never by authoring an asset.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool ShouldEnableVolumetricFog() const;

	// ──────────────────────── THE APPLY MODEL (GFX-§4) ────────────────────────

	/**
	 *  GFX-§4's SAFE half: apply the quality state AND save it, immediately.
	 *  Quality groups and resolution scale cannot soft-lock anyone — worst case
	 *  it looks bad and the player drags the slider back — and immediate apply is
	 *  what makes the perf/quality tradeoff LEARNABLE.
	 *
	 *  ⛔ ApplyNonResolutionSettings(), ⛔ NOT ApplySettings(): the latter also
	 *  re-applies the (possibly unconfirmed) resolution AND calls SaveSettings()
	 *  itself, defeating both halves of GFX-§4.
	 *
	 *  Every quality setter calls this for you. It is public only so a caller
	 *  that batched several writes can apply once.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	bool ApplyQualitySettings();

	/**
	 *  Broadcast on every ACTUAL change, never on a no-op. UI consumers SEED FROM
	 *  THE GETTER FIRST, THEN BIND (qa/TASK-005 major-2: a bind-only widget
	 *  created at a pinned value stays stale).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Graphics")
	FOnSiegeGraphicsSettingsChanged OnGraphicsSettingsChanged;

	//~ Begin USubsystem interface
	/** Logs the resolved baseline ONCE. ⛔ Applies nothing and saves nothing — the engine already loaded GameUserSettings.ini during FEngineLoop::PreInit. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End USubsystem interface

	// ──────────────────────── DIAGNOSTICS / TEST OBSERVATION POINTS ────────────────────────
	//
	// The USiegeSettingsSubsystem idiom: a dynamic multicast needs a bound
	// UFUNCTION and a UCLASS cannot be declared in a test .cpp, so the counters
	// are how automation observes "did it broadcast / apply / save?" without one.
	// Each is incremented at the EXACT site of the thing it counts, so it cannot
	// diverge from it.

	/** How many times OnGraphicsSettingsChanged has broadcast this session. */
	int32 GraphicsSettingsChangeBroadcastCount = 0;

	/** The setting name carried by the most recent broadcast (NAME_None before the first). */
	FName LastBroadcastSettingName = NAME_None;

	/** How many times the facade decided to call UGameUserSettings::ApplyNonResolutionSettings. */
	int32 ApplyNonResolutionSettingsCallCount = 0;

	/** How many times the facade decided to call UGameUserSettings::ApplyResolutionSettings(false). */
	int32 ApplyResolutionSettingsCallCount = 0;

	/** How many times the facade decided to call UGameUserSettings::SaveSettings. ⛔ Must stay 0 across a provisional video-mode change. */
	int32 SaveSettingsCallCount = 0;

	/** How many saves were REFUSED because a video-mode change was still unconfirmed (GFX-§4). */
	int32 RefusedSaveWhileVideoModePendingCount = 0;

	// ──────────────────────── AUTOMATION-ONLY SEAMS ────────────────────────

	/**
	 *  ⛔ AUTOMATION TESTS ONLY — nothing in the game may call this.
	 *
	 *  Points THIS INSTANCE at a scratch UGameUserSettings (a bare
	 *  NewObject<UGameUserSettings>(), which the engine's own constructor brings
	 *  up at SetToDefaults with no disk access) instead of
	 *  GEngine->GetGameUserSettings(), so a test run can never read, write or
	 *  apply the machine's real Saved/Config/.../GameUserSettings.ini.
	 *
	 *  ⚠️ IT ALSO TURNS ON ENGINE-APPLY SUPPRESSION, IN THE SAME CALL, ON PURPOSE.
	 *  ApplyNonResolutionSettings pushes global CVars through
	 *  Scalability::SetQualityLevels and ApplyResolutionSettings requests a real
	 *  display-mode change; running either against a scratch object would still
	 *  hit the live editor. Two switches would let a test forget one. Passing
	 *  nullptr restores normal resolution AND clears suppression.
	 *
	 *  ⚠️ DECLARED LIMIT OF WHAT THE SUITE CAN THEN SEE (SC-§79): with suppression
	 *  on, the Apply-and-Save counters record the facade's DECISION — that it
	 *  reached the call site — not the engine's effect. The decision is exactly
	 *  what GFX-§4 constrains ("SaveSettings is unreachable from the provisional
	 *  path"), so it is the right instrument for THAT failure class. It says
	 *  nothing about whether the display actually changed; that rung is a pixel /
	 *  human check owed by TASK-1118 and TASK-1119.
	 */
	void SetGameUserSettingsForAutomationTests(UGameUserSettings* InSettings);

	/**
	 *  ⛔ AUTOMATION TESTS ONLY. Forces ResolveSettings() to return nullptr so the
	 *  null-degradation contract can be exercised in-process — the one path that
	 *  is impossible to reach any other way, and the one where a missing null
	 *  check is a crash rather than a wrong pixel.
	 */
	void SetForceNullGameUserSettingsForAutomationTests(bool bInForceNull);

	/** ⛔ AUTOMATION TESTS ONLY. Zeroes every counter above so each test starts from a known state. */
	void ResetDiagnosticCountersForAutomationTests();

private:

	/**
	 *  THE ONE RESOLVE. Automation override > forced-null > GEngine->GetGameUserSettings().
	 *  Returns nullptr rather than crashing, and latches its failure log to ONE
	 *  line per subsystem instance (a per-frame getter must not spam).
	 */
	UGameUserSettings* ResolveSettings() const;

	/**
	 *  THE ONE MUTATION PATH FOR A QUALITY GROUP: resolve, clamp to [0, 4], read
	 *  current, bail on a no-op, write through the engine setter, apply, save,
	 *  broadcast. Every named setter and the generic setter funnel through here,
	 *  which is what makes "never broadcasts on a no-op" a STRUCTURAL property
	 *  rather than a promise repeated at ten call sites.
	 *  Returns true only when a real change was written.
	 */
	bool ApplyQualityGroupLevelInternal(FName GroupName, int32 NewLevel);

	/** Reads one group off a resolved settings object. Unknown names return FallbackQualityLevel. */
	static int32 ReadQualityGroupLevel(const UGameUserSettings* Settings, FName GroupName);

	/**
	 *  THE ONLY UGameUserSettings::SaveSettings CALL SITE IN THIS FILE, and the
	 *  place GFX-§4's "never save an unconfirmed video mode" is ENFORCED rather
	 *  than promised: while bVideoModeChangePending is true this REFUSES, counts
	 *  the refusal, and latches bSaveDeferredByVideoModePending so the save is
	 *  flushed by whichever of Confirm/Revert closes the window.
	 *  ⛔ QA trace (TASK-1114 cl. c): grep this file for "SaveSettings(" — there
	 *  is one call, and it is below the guard.
	 */
	bool RequestSaveSettings();

	/** The ONLY OnGraphicsSettingsChanged.Broadcast caller — also bumps the counter and logs. */
	void BroadcastGraphicsSettingChanged(FName SettingName);

	/** Shared tail for Confirm/Revert: clears the pending flag and flushes a save that was refused during the window. */
	bool CloseVideoModeWindow(const TCHAR* Reason);

	/** ⛔ Automation only. Non-null redirects ResolveSettings and implies bSuppressEngineApplyForAutomationTests. */
	UPROPERTY(Transient)
	TObjectPtr<UGameUserSettings> SettingsOverrideForAutomationTests = nullptr;

	/** ⛔ Automation only. Makes ResolveSettings return nullptr. */
	bool bForceNullSettingsForAutomationTests = false;

	/** ⛔ Automation only. Skips the three engine calls while still running (and counting) the facade's own decisions. */
	bool bSuppressEngineApplyForAutomationTests = false;

	/** True between a successful ApplyVideoModeProvisional() and the Confirm/Revert that closes it. */
	bool bVideoModeChangePending = false;

	/** Set when RequestSaveSettings refused during a pending video-mode window; flushed by CloseVideoModeWindow. */
	bool bSaveDeferredByVideoModePending = false;

	/** Latches the "UGameUserSettings did not resolve" line to ONE emission per subsystem instance. */
	mutable bool bResolveFailureLogged = false;

	/** Latches the "supported-resolution query failed" line to ONE emission per subsystem instance. */
	mutable bool bResolutionQueryFailureLogged = false;
};
