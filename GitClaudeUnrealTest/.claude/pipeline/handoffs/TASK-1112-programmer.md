# TASK-1112 — GFX-MEASURE — handoff (gameplay-programmer)

**Marker:** `TASK-1112-GFX-MEASURE` · **Date:** 2026-09-07 · **Mode:** DIAGNOSE-ONLY, gate waived
**Net source change: ZERO.** No `.cpp`, `.h`, `.ini`, `.uasset` written. No compile, no editor, no MCP, no git.
**Only write:** this file (+ my own `- status:` line on the board).

**Engine measured:** `C:/Program Files/Epic Games/UE_5.8/Engine/Build/Build.version` → `5.8.0`, CL `55116800`, branch `++UE5+Release-5.8`, `IsPromotedBuild: 1`. Every signature below is quoted from **that install**, not from memory or the web.

---

## 0. HEADLINES — the four things that change the lane's shape

| # | Finding | Affects |
|---|---|---|
| **A** | **Human gate: CONFIRMED.** All three sub-clauses verified at source. The lane's no-`.uasset` premise holds. | whole lane — **no re-board needed** |
| **B** | **`ConfirmVideoMode`/`RevertVideoMode` EXIST** (public, `BlueprintCallable`) — **but `RevertVideoMode()` does NOT apply anything.** It writes member fields and broadcasts. Without a following `ApplyResolutionSettings()` the revert is an `SC-§94`-class lie: the object reports the good mode, the screen keeps the broken one. | **`TASK-1118`** |
| **C** | **`r.VolumetricFog` is owned by the SHADOW group, not Effects.** `GFX-§9` binds fog to Effects; the engine disagrees. Also **`bEnableVolumetricFog` appears ZERO times in `Source/`, `Config/`, `Tools/`** — the symbol `GFX-§9` names does not exist in this project. | **`GFX-§9`, `TASK-1122`** |
| **D** | 🚨 **A per-machine `InstanceCount` multiplier BREAKS the M8/D9 server↔client determinism contract**, and not just for the scaled layer — it shifts the shared `FRandomStream` draw sequence, desyncing **every subsequent layer**. | 🚨 **`TASK-1122` — needs a manager ruling before it is written** |

---

## 1. THE REAL UE 5.8 SCALABILITY SURFACE

### 1.1 Quality groups — **the engine has ELEVEN `int32` groups, not ten**

`GFX-§8` lists ten. The measurement finds **eleven**. The extra one is **`LandscapeQuality`**, and the manager's own row anticipated exactly this ("*e.g. an extra `LandscapeQuality`*"). It is real, it is `BlueprintCallable`, and it round-trips to the ini on this machine.

File: `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/GameUserSettings.h`
All eleven pairs are `UFUNCTION(BlueprintCallable, Category=Settings)` — note the getters are **`BlueprintCallable`, NOT `BlueprintPure`**.

| # | Setter (`file:line`) | Getter (`file:line`) | `FQualityLevels` member | ini section / CVar |
|---|---|---|---|---|
| 1 | `void SetViewDistanceQuality(int32)` `:201` | `int32 GetViewDistanceQuality() const` `:205` | `ViewDistanceQuality` | `[ViewDistanceQuality@N]` |
| 2 | `void SetShadowQuality(int32)` `:210` | `int32 GetShadowQuality() const` `:214` | `ShadowQuality` | `[ShadowQuality@N]` |
| 3 | `void SetGlobalIlluminationQuality(int32)` `:219` | `int32 GetGlobalIlluminationQuality() const` `:223` | `GlobalIlluminationQuality` | `[GlobalIlluminationQuality@N]` |
| 4 | `void SetReflectionQuality(int32)` `:228` | `int32 GetReflectionQuality() const` `:232` | `ReflectionQuality` | `[ReflectionQuality@N]` |
| 5 | `void SetAntiAliasingQuality(int32)` `:237` | `int32 GetAntiAliasingQuality() const` `:241` | `AntiAliasingQuality` | `[AntiAliasingQuality@N]` |
| 6 | `void SetTextureQuality(int32)` `:246` | `int32 GetTextureQuality() const` `:250` | `TextureQuality` | `[TextureQuality@N]` |
| 7 | ⚠️ `void SetVisualEffectQuality(int32)` `:255` | ⚠️ `int32 GetVisualEffectQuality() const` `:259` | ⚠️ `EffectsQuality` | ⚠️ `[EffectsQuality@N]` |
| 8 | ⚠️ `void SetPostProcessingQuality(int32)` `:264` | ⚠️ `int32 GetPostProcessingQuality() const` `:268` | ⚠️ `PostProcessQuality` | ⚠️ `[PostProcessQuality@N]` |
| 9 | `void SetFoliageQuality(int32)` `:273` | `int32 GetFoliageQuality() const` `:277` | `FoliageQuality` | `[FoliageQuality@N]` |
| 10 | `void SetShadingQuality(int32)` `:282` | `int32 GetShadingQuality() const` `:286` | `ShadingQuality` | `[ShadingQuality@N]` |
| 11 | 🆕 `void SetLandscapeQuality(int32)` `:291` | 🆕 `int32 GetLandscapeQuality() const` `:295` | `LandscapeQuality` | 🚨 **NO ini section exists** |

**`Scalability::FQualityLevels`** — `.../Engine/Source/Runtime/Engine/Public/Scalability.h:27-159`. Twelve fields: `float ResolutionQuality` (`:29`) + the eleven `int32` above (`:30-40`). `operator==` `:58`, `GetHash()` `:80`.

⚠️ **NAME DRIFT — a real trap, and `GFX-§10` must rule on it.** Two concepts carry **four different spellings each**, and the wrappers cross them silently:
- `UGameUserSettings::SetVisualEffectQuality` → `ScalabilityQuality.SetEffectsQuality()` (`GameUserSettings.cpp:1067-1069`); getter returns `ScalabilityQuality.EffectsQuality` (`:1072-1075`). Player-facing level name is **`EffectsQuality`**.
- `UGameUserSettings::SetPostProcessingQuality` → `ScalabilityQuality.SetPostProcessQuality()` (`GameUserSettings.cpp:1077-1080`). Player-facing level name is **`PostProcessQuality`**.
> `GFX-§10` pins the widget triple as `<Group>Slider`. **Which `<Group>`?** My recommendation: use the **ini/CVar spelling** (`EffectsQuality…`, `PostProcessQuality…`) because that is what a level actually *means* and what a log/ini readback will show — but this is the **manager's** call, not mine. Flagged, not decided.

🚨 **`LandscapeQuality` is an API with no engine behaviour behind it — I recommend NOT shipping the row.** Evidence:
- `Engine/Config/BaseScalability.ini` contains **no `[LandscapeQuality@N]` section at any level** (section census in §1.4 below — 10 group families, not 11).
- `[ScalabilitySettings]` has **no `PerfIndexThresholds_LandscapeQuality`** line, so `RunHardwareBenchmark`/auto-detect cannot derive it (all ten others are present, `BaseScalability.ini:26-36`).
- It is defined only as a device-profile CVar (`BaseDeviceProfiles.ini:1047, 1062, 1077, 1094, 1128`) and as a CVar declaration (`Scalability.cpp:114`, `:191`), read/written to the ini at `Scalability.cpp:1199` / `:1237`.
- **This project has no Landscape.** Zero `Landscape` references in `Source/`; the only `Content/` hits are a vendor grass pack's material names (`Realistic_Grass_and_plant/…/M_Ground_Landscape.uasset`). The arena floor is a static mesh.
⇒ A Landscape slider would move, write `sg.LandscapeQuality`, and change **nothing** — precisely the "control that lies" `GFX-§9` forbids. **Recommendation: ship the ten, and record in the law WHY the eleventh was measured and dropped** (so nobody re-adds it from a UE docs page later).

**Levels: 0..4 for all eleven.** `sg.<Group>.NumLevels` defaults to `5` for every group including Landscape (`Scalability.cpp:190-195` for Landscape; `GetQualityLevelCounts()` `:1267-1283` reads all eleven). ⇒ **`GFX-§5`'s 5-detent slider (`StepSize 0.25`) is CORRECT.**

Useful extras found while measuring:
- `Scalability::GetScalabilityNameFromQualityLevel(int32)` → `FText`, `Scalability.h:251` — **the engine already localises "Low/Medium/High/Epic/Cinematic".** `GFX-§5`'s "live level name beside it" should use this, not a hand-written array.
- `Scalability::GetQualityLevelText(int32 Value, int32 NumLevels)` → `FText`, `Scalability.h:258`.
- `FQualityLevels::GetSingleQualityLevel()` `Scalability.h:106` — documented `-1:custom`. This is what backs `GetOverallScalabilityLevel()` (`GameUserSettings.cpp:971-974`) ⇒ **`GFX-§5`'s `Custom` state is engine-native, confirmed.**
- `Scalability::OnScalabilitySettingsChanged` — `DECLARE_MULTICAST_DELEGATE_OneParam`, `Scalability.h:161-162`. A ready-made change hook.
- `int32 FQualityLevels::GetMinQualityLevel() const` `Scalability.h:110`.

### 1.2 Non-group API — every signature the row asked for

All from `GameUserSettings.h` unless stated. **All confirmed present.**

| Requested | Verdict | Exact signature (`file:line`) | Notes |
|---|---|---|---|
| `SetOverallScalabilityLevel` | ✅ | `ENGINE_API virtual void SetOverallScalabilityLevel(int32 Value)` `:176` | `BlueprintCallable`. Body: `ScalabilityQuality.SetFromSingleQualityLevel(Value)` (`cpp:966-969`) |
| `GetOverallScalabilityLevel` | ✅ | `ENGINE_API virtual int32 GetOverallScalabilityLevel() const` `:180` | returns `-1` when custom (`cpp:971-974`) |
| `SetResolutionScaleNormalized` | ✅ | `ENGINE_API void SetResolutionScaleNormalized(float NewScaleNormalized)` `:196` | see §1.3 — **read it, there is a trap** |
| `GetResolutionScaleNormalized` | ✅ | `ENGINE_API float GetResolutionScaleNormalized() const` `:188` | |
| value-based variant | ✅ | `ENGINE_API void SetResolutionScaleValueEx(float NewScaleValue)` `:192`, `meta=(DisplayName="SetResolutionScaleValue")` | ⚠️ the C++ name carries the **`Ex` suffix**; only the BP display name drops it |
| range query | ✅ | `ENGINE_API void GetResolutionScaleInformationEx(float& CurrentScaleNormalized, float& CurrentScaleValue, float& MinScaleValue, float& MaxScaleValue) const` `:184` | likewise `Ex`-suffixed |
| `SetScreenResolution` | ✅ | `ENGINE_API void SetScreenResolution(FIntPoint Resolution)` `:71` | getter `GetScreenResolution() const` `:59` (`BlueprintPure`) |
| `SetFullscreenMode` | ✅ | `ENGINE_API void SetFullscreenMode(EWindowMode::Type InFullscreenMode)` `:83` | getter `:75`; **`EWindowMode::Type` is a plain C++ enum — per `GFX-§2(d)` it may NEVER cross a `BlueprintImplementableEvent` boundary. Marshal as `int32`.** |
| `SetVSyncEnabled` | ✅ | `ENGINE_API void SetVSyncEnabled(bool bEnable)` `:115` | getter `bool IsVSyncEnabled() const` `:119` — **note the name is `Is…`, not `Get…`** |
| `SetFrameRateLimit` | ✅ | `ENGINE_API void SetFrameRateLimit(float NewLimit)` `:167` | **`float`, not `int32`**; `0` disables. Getter `float GetFrameRateLimit() const` `:171` |
| `RunHardwareBenchmark` | ✅ | `ENGINE_API virtual void RunHardwareBenchmark(int32 WorkScale = 10, float CPUMultiplier = 1.0f, float GPUMultiplier = 1.0f)` `:379` | **populates only; applies nothing** (`cpp:1122-1126`) |
| `ApplyHardwareBenchmarkResults` | ✅ | `ENGINE_API virtual void ApplyHardwareBenchmarkResults()` `:383` | applies **and saves** (`cpp:1132-1136`) |
| `ApplySettings` | ✅ | `ENGINE_API virtual void ApplySettings(bool bCheckForCommandLineOverrides)` `:49` | **no default argument — the caller must pass the bool** |
| `ApplyNonResolutionSettings` | ✅ | `ENGINE_API virtual void ApplyNonResolutionSettings()` `:52` | |
| `SaveSettings` | ✅ | `ENGINE_API virtual void SaveSettings()` `:311` | |
| 🚨 `ConfirmVideoMode` | ✅ | `ENGINE_API virtual void ConfirmVideoMode()` `:147` | **public, `BlueprintCallable`** |
| 🚨 `RevertVideoMode` | ✅ | `ENGINE_API void RevertVideoMode()` `:151` | **public, `BlueprintCallable`** — ⚠️ **see the caveat below** |
| `GetSupportedFullscreenResolutions` | ⚠️ **NOT on `UGameUserSettings`** | `static ENGINE_API bool GetSupportedFullscreenResolutions(TArray<FIntPoint>& Resolutions)` — **`Kismet/KismetSystemLibrary.h:1786`** | see §1.5 |

Also present and worth having:
`ApplyResolutionSettings(bool)` `:55` · `GetLastConfirmedScreenResolution()` `:63` · `GetDesktopResolution()` `:67` · `GetLastConfirmedFullscreenMode()` `:79` · `GetPreferredFullscreenMode()` `:87` · `SetPreferredFullscreenMode(int32)` `:709` · `IsScreenResolutionDirty()` `:131` · `IsFullscreenModeDirty()` `:135` · `IsVSyncDirty()` `:139` · `SetBenchmarkFallbackValues()` `:155` · `IsDirty()` `:299` · `ValidateSettings()` `:303` · `LoadSettings(bool bForceReload=false)` `:307` · `ResetToCurrentSettings()` `:315` · `SetToDefaults()` `:333` · `GetDefaultResolutionScale()` `:337` · `GetDefaultResolution()` `:348` (static) · `static UGameUserSettings* GetGameUserSettings()` `:375` · `static FString GetConfigDir()` `:45` · `SupportsHDRDisplayOutput()` `:387` · `static int32 GetFramePace()` `:365`.

> ⚠️ **`GetSyncInterval()` `:361` is `UE_DEPRECATED(4.25)`** — do not use it; `GetFramePace()` `:365` replaces it.
> ⚠️ **`Scalability::GetResolutionScreenPercentage()` `Scalability.h:247-248` is `UE_DEPRECATED(5.3)`** — do not use it.

### 1.3 🚨 `RevertVideoMode()` DOES NOT APPLY — and the resolution-scale floor is 0, not 50

**(a) The revert caveat — `TASK-1118` must not assume otherwise.** `GameUserSettings.cpp:276-285`, complete body:

```cpp
void UGameUserSettings::RevertVideoMode()
{
    FullscreenMode = LastConfirmedFullscreenMode;
    ResolutionSizeX = LastUserConfirmedResolutionSizeX;
    ResolutionSizeY = LastUserConfirmedResolutionSizeY;
    DisplayID = LastUserConfirmedDisplayID;
    DisplayIndex = LastUserConfirmedDisplayIndex;

    OnGameUserSettingsVideoRevert.Broadcast();
}
```

It restores **fields only**. Nothing is pushed to the display. Symmetrically `ConfirmVideoMode()` (`cpp:267-274`) only stamps the `LastConfirmed*` fields — **it does not save.** ⇒ the `GFX-§4` sequences must be:
- **confirm:** `ConfirmVideoMode()` → `SaveSettings()`
- **revert:** `RevertVideoMode()` → **`ApplyResolutionSettings(false)`** → `SaveSettings()`

Omitting the apply on the revert path ships the exact failure `GFX-§4` exists to prevent — an unreadable screen — **while every property read-back says the setting is fine.** That is an `SC-§94` cl. A lie: the instrument reports the request, not the result. **QA criterion for `TASK-1118`: the revert path contains an apply call.**
There is a delegate for the UI: `OnGameUserSettingsVideoRevert` (broadcast at `cpp:284`).

**(b) The resolution-scale floor is `0.0`, and `0` is currently set on this machine.**
`Scalability.h:242` `inline constexpr float MinResolutionScale = 0.0f;` · `Scalability.h:245` `inline constexpr float MaxResolutionScale = 100.0f;`
`GameUserSettings.cpp:976-981` — `MinScaleValue = Scalability::MinResolutionScale;` and normalized is a plain lerp, so **normalized == percent / 100 exactly**. `SetResolutionScaleNormalized(0.f)` therefore requests a **0 % render resolution**.
⇒ `TASK-1113` cl. (3)'s 50–100 % clamp is **load-bearing, not cosmetic**, and it maps to normalized `0.50 … 1.00` with no arithmetic.
⚠️ **But there is a second edge, and it is live right now:** this machine's ini reads **`sg.ResolutionQuality=0`**, which `BaseScalability.ini:39-42` defines as a *sentinel* — `+ResolutionPresets=(Name="Default",ResolutionQuality=0.0)`, "*use the project's default screen percentage*". A naive clamp-on-load would silently rewrite that sentinel to a hard 50 %, **changing the player's picture the first time they open the menu without touching anything.** `TASK-1113` should clamp on **write**, and treat a read of `0` as "Default", not as 0 %.

### 1.4 `BaseScalability.ini` — group / level census

`C:/Program Files/Epic Games/UE_5.8/Engine/Config/BaseScalability.ini` (40,197 bytes). **The project overrides none of it** — `Config/` holds no `*Scalability*` file and `DefaultEngine.ini` contains no `sg.*`, no `[<Group>Quality@N]` section, and **no `GameUserSettingsClassName`** (independently confirming `GFX-§3`'s "no subclass, no ini edit" consequence).

**Ten group families × five levels**, plus `[ResolutionQuality]` which is special:

| Group section | Levels present | Line (level 0) |
|---|---|---|
| `[AntiAliasingQuality@N]` | `@0 @1 @2 @3 @Cine` | `:52` |
| `[ViewDistanceQuality@N]` | `@0 @1 @2 @3 @Cine` | `:110` |
| `[ShadowQuality@N]` | `@0 @1 @2 @3 @Cine` | `:132` |
| `[GlobalIlluminationQuality@N]` | `@0 @1 @2 @3 @Cine` | `:318` |
| `[ReflectionQuality@N]` | `@0 @1 @2 @3 @Cine` | `:514` |
| `[PostProcessQuality@N]` | `@0 @1 @2 @3 @Cine` | `:564` |
| `[TextureQuality@N]` | `@0 @1 @2 @3 @Cine` | `:740` |
| `[EffectsQuality@N]` | `@0 @1 @2 @3 @Cine` | `:798` |
| `[FoliageQuality@N]` | `@0 @1 @2 @3 @Cine` | `:964` |
| `[ShadingQuality@N]` | `@0 @1 @2 @3 @Cine` | `:1006` |
| `[LandscapeQuality@N]` | 🚨 **ABSENT AT EVERY LEVEL** | — |
| `[ResolutionQuality]` | not level-suffixed; a preset list | `:39` |

⚠️ **Level 4 is spelled `@Cine`, not `@4`.** Anything that composes a section name by hand must use `Scalability::GetScalabilitySectionString(...)` (`Scalability.h:260`) rather than string-concatenating the integer.

`[ScalabilitySettings]` `:16` carries `PerfIndexThresholds_<Group>` for **ten** groups (`:26-36`) — **Landscape absent** — plus `PerfIndexValues_ResolutionQuality="50 71 87 100 100"` `:37`. That last line is worth stating plainly for `GFX-§7`'s honest hints: **auto-detect will never choose a resolution scale below 50 %**, and levels 3 and 4 both mean 100 %.

**Three group bodies that matter to this project, read in full:**
- `[ViewDistanceQuality@0]` `:110` → `r.SkeletalMeshLODBias=2`, **`r.ViewDistanceScale=0.4`**. `@3` `:122` → `r.SkeletalMeshLODBias=0`, `r.ViewDistanceScale=1.0`.
- `[FoliageQuality@0]` `:964` → `foliage.DensityScale=0`, `grass.DensityScale=0`, `pcg.Quality=0`. `@3` `:988` → all `1.0` / `3`.
- `[EffectsQuality@0]` `:798` → 28 CVars incl. `r.DetailMode=0`, `r.MaterialQualityLevel=0`, `fx.Niagara.QualityLevel=0`, `r.SSGI.Quality=0`, `r.EmitterSpawnRateScale=0.125`.

✅ **`GFX-§8`'s "not boilerplate on this renderer" claim is CONFIRMED at source.** `Config/DefaultEngine.ini` really does run: `r.DynamicGlobalIlluminationMethod=1` `:56`, `r.ReflectionMethod=1` `:57`, `r.Lumen.HardwareRayTracing=True` `:60`, `r.Shadow.Virtual.Enable=1` `:69`, `r.RayTracing=True` `:70`, `r.Nanite.ProjectEnabled=True` `:76`, `r.VirtualTextures=True` `:28`. The GI/Shadow/Reflection groups scale exactly these.

### 1.5 `GetSupportedFullscreenResolutions` lives elsewhere

Not a member of `UGameUserSettings`. It is a static on `UKismetSystemLibrary`:
- `static ENGINE_API bool GetSupportedFullscreenResolutions(TArray<FIntPoint>& Resolutions)` — `Engine/Source/Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h:1786`
- `static ENGINE_API bool GetConvenientWindowedResolutions(TArray<FIntPoint>& Resolutions)` — `…:1793`

⇒ `TASK-1113` cl. (3)'s "enumerated supported-mode list" needs `#include "Kismet/KismetSystemLibrary.h"`, must handle the **`bool` failure return**, and should pick the **windowed** variant when the current mode is windowed — the fullscreen list is monitor mode-list-derived and is the wrong set for a window.

### 1.6 The current `GameUserSettings.ini` on this machine — the baseline

Contrary to the row's expectation ("or state that it does NOT EXIST YET"), **it exists**:
`C:/GitProjects/.../GitClaudeUnrealTest/Saved/Config/WindowsEditor/GameUserSettings.ini` (313-byte sibling `ConsoleHistory.ini` in the same dir; this is the **editor's** copy — a packaged build writes to `%LOCALAPPDATA%/GitClaudeUnrealTest/Saved/Config/Windows/GameUserSettings.ini`, resolved at runtime by `UGameUserSettings::GetConfigDir()` `GameUserSettings.h:45`). A coalesced copy also sits at `Intermediate/Config/CoalescedSourceConfigs/GameUserSettings.ini`.

```ini
[/Script/Engine.GameUserSettings]
bUseVSync=False              bUseDynamicResolution=False
ResolutionSizeX=3200         ResolutionSizeY=1800
LastUserConfirmedResolutionSizeX=3200   LastUserConfirmedResolutionSizeY=1800
FullscreenMode=1             LastConfirmedFullscreenMode=1   PreferredFullscreenMode=1
DisplayIndex=0               Version=5
AudioQualityLevel=0          FrameRateLimit=0.000000
DesiredScreenWidth=1280      bUseDesiredScreenHeight=False   DesiredScreenHeight=720
LastCPUBenchmarkResult=-1.000000        LastGPUBenchmarkResult=-1.000000
bUseHDRDisplayOutput=False

[ScalabilityGroups]
sg.ResolutionQuality=0       sg.ViewDistanceQuality=3   sg.AntiAliasingQuality=3
sg.ShadowQuality=3           sg.GlobalIlluminationQuality=3   sg.ReflectionQuality=3
sg.PostProcessQuality=3      sg.TextureQuality=3        sg.EffectsQuality=3
sg.FoliageQuality=3          sg.ShadingQuality=3        sg.LandscapeQuality=3
```

Four things this baseline proves rather than assumes:
1. **All twelve `sg.*` rows round-trip, Landscape included** — so `Scalability::SaveState` writes the eleventh even with no ini section behind it. The ini is not the place to detect the dead group.
2. **Everything is at `3` (Epic).** ⇒ `TASK-1122`'s "Epic = ×1.0 ⇒ byte-for-byte unchanged" default is the **actual** current state, so a regression at Epic really is a FAIL and really is detectable.
3. **`LastCPUBenchmarkResult=-1` / `LastGPUBenchmarkResult=-1`** ⇒ the hardware benchmark has **never been run on this machine**. `GFX-§6`'s Auto-Detect button will be exercising a genuinely cold path.
4. `FrameRateLimit=0` (unlimited) and `FullscreenMode=1` (`EWindowMode::WindowedFullscreen`) are the live values a first open of the panel must display.

---

## 2. THE HUMAN GATE — **CONFIRMED**

**CONFIRMED.** All three sub-clauses of cl. (3) verified by reading `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.cpp` (545 lines). The manager's two citations are **exact, to the line**.

**(i) `ConstructSettingsTree()` is called BEFORE `Super::RebuildWidget()`** — `SettingsMenuWidget.cpp:92-94`, quoted verbatim:

```cpp
	Initialize();
	ConstructSettingsTree();
	return Super::RebuildWidget();
```

(`:93` then `:94` — construct, *then* Super. The `Initialize()` at `:92` is the `GFX-§2(c)` precondition, already shipped, with its justification written in-file at `:81-91`: `WidgetTree` is allocated inside `Initialize()`, and `Super::RebuildWidget()`'s own self-heal runs too late to save a null tree.)

**(ii) `WidgetTree->RootWidget` is assigned in C++** — `SettingsMenuWidget.cpp:157`, quoted verbatim:

```cpp
	WidgetTree->RootWidget = BackdropBorder;
```

**(iii) No `WBP_SettingsMenu.uasset` exists anywhere under `Content/`.** A case-insensitive search for `*SettingsMenu*` **and** `*GraphicsMenu*` across all of `Content/` returns **zero files**. The complete `WBP_*.uasset` census is ten assets — `WBP_CardHand`, `WBP_CastleHealthBar`, `WBP_CombatantHealthBar`, `WBP_DeckBuilder`, `WBP_DeckCardTile`, `WBP_HUD`, `WBP_MainMenu`, `WBP_SessionMenu`, `WBP_VictoryScreen`, `WBP_WarMap` — and **neither `WBP_SettingsMenu` nor `WBP_GraphicsMenu` is among them.**

⇒ **`GFX-§1` holds. No Jonathan UMG minutes are owed. `/Game/UI/WBP_SettingsMenu` is reserved-not-authored, exactly as ruled.** The lane keeps its shape: `USiegeGraphicsMenuWidget` is a fourth code-authored widget and needs no designer step.

**Bonus for `TASK-1115` (free while I was in the file).** The settings tree today builds exactly six children, in this order: `TitleText` (`:183`) → `ConfirmToggleCheckBox` (`:219`) → `ConfirmToggleLabelText` (`:239`) → `ConfirmToggleHintText` (`:258`) → **`BackButton`** (constructed `:283`, added `:296`). The `GFX-§10` `GraphicsButton` + `GraphicsLabelText` pair should be inserted **before the `---- BackButton ----` block at `:267`** so Back stays last. The button idiom to clone is `:281-300` (construct → `SetContent(label)` → `AddChildToVerticalBox`), with click binding/unbinding at `:393-395` / `:413-415`. `GFX-§2(f)`'s hit-test rule is already implemented and commented at `:134-148` — clone that comment's reasoning, and note the owning `UUserWidget` stays `SelfHitTestInvisible` while the **border** does the absorbing.

---

## 3. THE SCATTER — build trigger, by line

File: `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` (2,720 lines).
**All four line numbers in the spec are EXACT** — `:516`, `:625`, `:1003`, `:1092` are the sites described.

### 3.1 The call graph

```
BeginPlay()                        :202
  └─ if (HasAuthority())           :230   ── authority only
       GenerateScatter()           :232 → :259
         ├─ ClearScatter()         :293
         ├─ seed select            :298-312   (OverrideSeed > 0 wins; else fresh RandRange; else LastSeed)
         ├─ ChosenSeed/++GenerationIndex  :319-320  (replicated pair)
         └─ RunScatterPasses(Seed, /*bAuthoritativeGenerate=*/true)   :322

OnRep_GenerationIndex()            :335   ── CLIENT mirror path
  ├─ ClearScatter()                :357
  └─ RunScatterPasses(ChosenSeed, /*bAuthoritativeGenerate=*/false)   :360

RunScatterPasses(Seed, bAuth)      :363
  ├─ FRandomStream Stream(Seed);   :365   ◄◄◄ THE SEAM
  ├─ RebuildKeepClearZones()       :371
  ├─ pass 1: !bAllowOnHills layers → ScatterLayer(Layer, Stream)      :394-400
  └─ pass 2: bAllowOnHills layers  → ScatterLayer(Layer, Stream)

ScatterLayer(const FScatterLayer& Layer, FRandomStream& Stream)       :514
  ├─ early-out: Layer.InstanceCount <= 0 || !Meshes.Num() || !ScatterConfig   :516
  ├─ MaxLayerR ← Layer.FootprintRadius (if > 0)                       :597-599, :671-673
  ├─ SpacingGrid.Init(Layer.MinSpacing + 2*MaxLayerR, Layer.MinSpacing)  :613
  ├─ OuterTarget = bRotSym ? DivideAndRoundUp(InstanceCount,2) : InstanceCount   :624-626
  ├─ for InstanceIndex < OuterTarget                                   :634
  │    └─ for Attempt < MaxPlacementAttemptsPerInstance                :637
  │         ├─ Stream.FRandRange(-HalfX, DrawMaxX)                     :647
  │         ├─ SampleBiasedY(Stream, …)                                :648
  │         └─ Resolved[Stream.RandRange(0, Num-1)]                    :660
  ├─ Comp->SetCullDistances(Max(CullStartDistance,0), Max(CullEndDistance,0))   :1003
  └─ Proxy->SetCullDistances(same)                                     :1092
```

### 3.2 Where a scalability-aware read goes — **`RunScatterPasses`, at `:365`**

**Name it precisely: `ASiegeBattlefieldScatter::RunScatterPasses(int32 Seed, bool bAuthoritativeGenerate)`, `BattlefieldScatter.cpp:363`, with the read placed alongside `FRandomStream Stream(Seed);` at `:365`.**

That is the correct site and **not** `:516` / `:625`, for three reasons:
1. It is the **single funnel** — both the authority path (`:322`) and the client mirror (`:360`) pass through it; `ScatterLayer` is only ever reached from here. A read at `:516` would execute **once per layer** (7 layers) and re-answer the same question seven times — the `SC-§90` "a name census can't catch a site that recomputes a value" shape, inverted.
2. It runs **before** any placement, so one read can be cached into a local and passed down — no subsystem lookup inside the hot rejection loop at `:637`.
3. `bAuthoritativeGenerate` is **already in scope there**, which is exactly the discriminator §3.4 shows the density lever needs.

### 3.3 Once per match? — **Yes, and it is also re-runnable. "Applies at next match start" is HONEST.**

- **Once per match, at match start:** `BeginPlay()` `:202` is the only unsolicited entry. The in-file comment at `:224` states the contract: *"Match-start scatter (Jonathan: 'randomly generated at the start of each match')."*
- **Re-runnable, and already re-run in shipped code:** `GenerateScatter()` is explicitly **idempotent** — `:291-293` comments *"A generate always starts from a clean field (idempotent: BeginPlay's first call clears nothing, **Play Again's re-scatter** clears the prior layout)"* and calls `ClearScatter()`. So a Play-Again re-scatter is a real, existing path.
- **There is no tick and no poll.** `RunScatterPasses` is called from exactly two places (`:322`, `:360`). Nothing re-enters it per frame.

⇒ **`GFX-§9`'s "read at spawn / build time, never per frame" is satisfiable exactly as written**, and the UI sentence *"these apply at the next match start"* is **literally true** — a player changing Foliage mid-match sees no change until the next `BeginPlay`/Play-Again, which is precisely what the UI would be promising.

### 3.4 🚨 LANE-SHAPE FINDING — the density multiplier breaks server↔client determinism

**This one needs a manager ruling before `TASK-1122` is written. I am reporting it, not re-boarding it.**

`TASK-1122` cl. (1) says: multiply each layer's `InstanceCount` at `:516`/`:625` by `GetFoliageDensityScale()`. But `GetFoliageDensityScale()` derives from `sg.FoliageQuality`, which `GFX-§3` correctly rules **per-machine**. A per-machine value entering the layout algorithm collides head-on with the M8/D9 contract:

- The client does **not** generate independently. `BeginPlay:230` gates on `HasAuthority()`; a client waits and mirrors the authority's seed through `OnRep_GenerationIndex:335`. The in-file comment at `:225-229` says why: *"a client instance NEVER self-generates (its local random seed is the different-battlefields bug)."*
- Both machines then run **the same `RunScatterPasses`** and are required to produce the **same field**. `:373-377` calls its log line *"the one grep-able reproducibility line… identical on both machines for the same seed."*
- ⇒ If the server is at Foliage=Epic and the client at Foliage=Low, **the same seed yields two different battlefields.** That is the exact bug D9 exists to kill, reintroduced through a settings menu.

**And it is worse than "the scaled layer thins out."** `OuterTarget` (`:624-626`) controls the **iteration count of a loop that draws from a shared `FRandomStream`** (`:647`, `:648`, `:660`, inside the attempt loop at `:637`). Fewer iterations ⇒ fewer draws ⇒ **every layer processed afterwards receives a different stream position and lays out differently too.** The file says so itself, twice, in comments written for earlier changes: `:389-392` (*"opting a layer in… intentionally changes the FRandomStream draw sequence"*) and `:651-658` (*"Drawing them earlier changes the FRandomStream draw sequence, so EXISTING seeds now produce DIFFERENT (still-valid) layouts"*). A density multiplier is the same class of change, but driven by a **per-client** value.

Third-order: blocking instances feed `ValidateTraversability()` `:2064` and `CullCorridorBlockers()` `:2464`, which run **authority-side only**. `:339-340` records the accepted residual as *"client obstacles are a superset"* — a density multiplier makes the low-spec client a **subset**, inverting the assumption the residual was accepted under.

**Three ways out, for the manager to choose between (I have no preference to impose):**
- **(a) Cull-band only.** Drop density from Tier D. `SetCullDistances` (`:1003`, `:1092`) is **render-side state applied after placement** — it consumes **zero** RNG and changes **no** layout. It is per-client-safe by construction and needs no ruling. *(This is the cheapest correct answer and still gives the Foliage/View-Distance sliders real teeth — see §3.5.)*
- **(b) Place-then-hide.** Keep the full deterministic placement; skip only the `AddInstance` call for a deterministically-chosen subset (e.g. every Nth index). Layout, stream, and blocker set stay byte-identical; only what is *drawn* thins. Costs the placement work but not the render work.
- **(c) Standalone-only.** Gate the density multiplier on `bAuthoritativeGenerate && standalone`. Simple, but it means the lever silently disappears in multiplayer — a control that lies in exactly the way `GFX-§9` forbids.

**Whichever is chosen, `TASK-1122`'s spec text ("multiply each layer's `InstanceCount` at the EXISTING build site") cannot be executed as literally written without breaking D9.**

### 3.5 Two more Tier-D corrections

**(a) The Foliage group does nothing to this scatter today — which *justifies* the Tier-D lever.**
`[FoliageQuality@N]` sets `foliage.DensityScale` and `grass.DensityScale`, which drive UE's **foliage/landscape-grass systems**. This project's scatter is its own C++-spawned HISM/ISM field, untouched by those CVars. ⇒ `GFX-§9`'s premise — *"leaving it out would mean the Foliage slider moves and nothing happens"* — is **measured true**. Good ruling, confirmed.

**(b) ⚠️ The View-Distance lever risks double-scaling.**
`[ViewDistanceQuality@N]` sets **`r.ViewDistanceScale`** (`0.4` at `@0` `:112`, `1.0` at `@3` `:124`), and that CVar already scales primitive draw distances **including HISM cull distances**. So the View-Distance group **already** moves the scatter's effective cull band with no code at all. Layering a second multiplier on `CullStartDistance`/`CullEndDistance` at `:1003`/`:1092` would compound it — Low would become `0.4 × <our factor>`, not `0.4`. `TASK-1122` should either use a **compensating** factor or leave the band alone and let `r.ViewDistanceScale` do the work.
⚠️ Related: I could **not** reproduce `TASK-1084`'s *"~72 m at `r.ViewDistanceScale 0.8`"* from config — `r.ViewDistanceScale` appears **nowhere** in `Config/` or `Source/`, and `ViewDistanceQuality@3` sets it to `1.0`. The `0.8` was presumably an editor-session or device-profile value. **`TASK-1122` should re-measure the band rather than inherit `0.8`** (`SC-§91` — a relayed number is a lower bound).

### 3.6 🚨 Volumetric fog — `GFX-§9` binds it to the wrong group, and names a symbol that does not exist

**(a) Wrong group.** `GFX-§9` says *"VOLUMETRIC FOG ← the Effects group."* The engine puts it under **Shadow**:

| Section | `r.VolumetricFog` | line |
|---|---|---|
| `[ShadowQuality@0]` | **`0`** | `:143` |
| `[ShadowQuality@1]` | **`0`** | `:178` |
| `[ShadowQuality@2]` | `1` (`GridPixelSize=16`, `GridSizeZ=64`) | `:213-216` |
| `[ShadowQuality@3]` | `1` (`GridPixelSize=8`, `GridSizeZ=128`) | `:251-254` |
| `[ShadowQuality@Cine]` | `1` (`GridPixelSize=4`, `GridSizeZ=128`, `HistoryMissSupersampleCount=16`) | `:289-292` |

`[EffectsQuality@*]` never mentions `r.VolumetricFog` — its only fog line is `r.Fog.SeparateComposition` (`@0` = `0`, `@3` = `-1`), an unrelated composition flag.
⇒ **Stock UE 5.8 already turns volumetric fog off at Shadows Low/Medium and already scales its froxel grid across High/Epic/Cinematic.** Binding a project switch to **Effects** would produce a live contradiction: a player at Shadows=Low + Effects=Epic gets `r.VolumetricFog=0` from the engine while the project switch insists fog is on — and the fog would be *gone* with the named control saying otherwise. That is the failure mode `GFX-§9` was written to prevent, arriving through the ruling itself.
**Recommendation for the manager:** either re-bind the fog lever to the **Shadow** group, or drop it entirely and let stock scalability own it (it already does the job, including a graduated quality ramp we would not otherwise get). Dropping it is defensible and shrinks the lane.

**(b) The named symbol does not exist.** `GFX-§9` cites *"`bEnableVolumetricFog` (shipped `12b8707`)"*. A literal search for `bEnableVolumetricFog` across `Source/`, `Config/` and `Tools/` returns **zero occurrences**. The only `VolumetricFog` hits in `Source/` are **comments** referencing `L_Arena`'s `VolumetricFogDistance` (`FogVolume.h:575`, `:600`, `:605`) and one test string (`Tests/SiegeFogVisualTest.cpp:801`). The project's fog code is `Siegebound/FogVolume.{h,cpp}` + `SiegeFogStatics.{h,cpp}`; `bEnableVolumetricFog` is a `UExponentialHeightFogComponent` **engine property**, set on the fog actor/BP, not a project C++ symbol.
⇒ **`TASK-1122` cannot "set `bEnableVolumetricFog` from code" as if it were ours.** Doing so means reaching into a fog component — and under `GFX-§11` (`L_Arena.umap` never saved, zero `.uasset` writes) that must be a **runtime** mutation, not an authored one. Cleanest alternative if the lever survives: set the **`r.VolumetricFog` CVar** at build time and leave every asset alone.

---

## 4. WHAT DIFFERS FROM THE BRIEF

| # | Brief / law said | Engine says | Severity |
|---|---|---|---|
| 1 | `GFX-§8`: **ten** quality groups | **Eleven** `int32` groups on `UGameUserSettings`. `LandscapeQuality` (`:291`/`:295`) is the extra — but has **no `BaseScalability.ini` section, no PerfIndex threshold**, and this project has no Landscape. **Recommend shipping ten** and recording why. | ⭐⭐ list-changing |
| 2 | `GFX-§9`: fog ← **Effects** group | Fog is `r.VolumetricFog`, owned by **Shadow** (`@0`/`@1` = off). Effects never touches it. | ⭐⭐⭐ **contradiction** |
| 3 | `GFX-§9`: *"`bEnableVolumetricFog` (shipped `12b8707`)"* | **Zero occurrences** in `Source/`/`Config/`/`Tools/`. Not a project symbol. | ⭐⭐⭐ **symbol absent** |
| 4 | `TASK-1122` cl. 1: multiply `InstanceCount` at `:516`/`:625` | **Breaks the M8/D9 determinism contract** and shifts the shared RNG stream for every later layer. Needs a ruling — three options in §3.4. | 🚨 **blocking for 1122** |
| 5 | `GFX-§4`: *"use the engine's own mechanism"* for revert | Mechanism exists — but `RevertVideoMode()` **applies nothing**. Needs `ApplyResolutionSettings(false)` after it, or the revert is a silent no-op on screen. | ⭐⭐⭐ `TASK-1118` |
| 6 | `GetSupportedFullscreenResolutions` implied on `UGameUserSettings` | It is `UKismetSystemLibrary::GetSupportedFullscreenResolutions` (`KismetSystemLibrary.h:1786`), static, returns `bool`. | ⭐ include + error path |
| 7 | `TASK-1113` cl. 3: resolution scale *"clamped to the 50–100% band"* | Engine floor is **`0.0`** (`Scalability.h:242`), and the live ini holds **`sg.ResolutionQuality=0`** — a *"use project default"* **sentinel**, not 0 %. Clamp on **write**; treat a read of `0` as Default. | ⭐⭐ silent-change risk |
| 8 | `TASK-1122`: cull-band multiplier | The View-Distance group **already** moves the band via `r.ViewDistanceScale` (`0.4`…`1.0`). A second multiplier double-scales. Also: `TASK-1084`'s `0.8` is not reproducible from config — re-measure. | ⭐⭐ |
| 9 | `GFX-§10`: `<Group>Slider` naming | **Two groups have four spellings each** (`VisualEffect`/`Effects`, `PostProcessing`/`PostProcess`). Manager must pin one. I suggest the ini/CVar spelling. | ⭐ naming ruling owed |
| 10 | `TASK-1112` cl. 5: *"or state it does NOT EXIST YET, which is the expected v1 state"* | **It exists** — full contents in §1.6. All groups at `3`, benchmark never run (`-1`/`-1`), `sg.ResolutionQuality=0`. | ⭐ baseline captured |

**Confirmed unchanged (no action):** `ConfirmVideoMode`/`RevertVideoMode` exist and are public ⇒ **`TASK-1118` is NOT re-scoped and `TASK-1113` cl. (4) does not hit its STOP condition.** `GFX-§5`'s 5 detents is right (NumLevels = 5, all groups). `GFX-§5`'s `Custom` state is engine-native (`-1`). `GFX-§8`'s Lumen/HWRT/VSM/Nanite/VT justification is true at `DefaultEngine.ini:28,56,57,60,69,70,76`. `GFX-§3`'s no-subclass/no-ini consequence holds — `DefaultEngine.ini` has no `GameUserSettingsClassName` and no `sg.*`.

---

## 5. WHAT QA SHOULD SCRUTINISE

1. **That I did not write anything.** `git status` should show only this handoff + my board status line. No `Source/`, `Tests/`, `Content/`, `Config/` diff.
2. **The human-gate verdict is the lane's foundation** — re-run the three checks independently if there is any doubt: `SettingsMenuWidget.cpp:92-94`, `:157`, and a `Content/` search for `*SettingsMenu*`. A false CONFIRMED here costs the whole lane.
3. **The eleven-vs-ten count.** I recommend ten. If the manager ships eleven, `TASK-1113` cl. (2) must still build the eleventh from *my* list, and `GFX-§9`'s "a control that lies" reasoning then applies to it.
4. **§3.4 is the finding I am least willing to have waved through.** The determinism argument rests on `BeginPlay:230`, `OnRep_GenerationIndex:335`, and the shared `Stream` at `:365`/`:637`. If QA thinks I have it wrong, say so before `TASK-1122` is written, not after.
5. **§3.6(a)** — verify the section boundaries yourself; I derived group ownership by walking back to the nearest preceding `[` header for each `r.VolumetricFog` line, and I report the mapping as `@0`:143, `@1`:178, `@2`:213, `@3`:251, `@Cine`:289.
6. **Nothing here is a runtime measurement.** Every claim is read from **static source, config and ini text**. No PIE session, no log, no pixels. The `GFX-§9`/`SC-§94` "verify by rendered pixels and the engine log" obligation is **entirely unspent** and still owed by the rows that ship behaviour.

## 6. FILES READ (zero written except this handoff)

**Engine (read-only):** `Engine/Source/Runtime/Engine/Classes/GameFramework/GameUserSettings.h` · `Engine/Source/Runtime/Engine/Private/GameUserSettings.cpp` · `Engine/Source/Runtime/Engine/Public/Scalability.h` · `Engine/Source/Runtime/Engine/Private/Scalability.cpp` · `Engine/Source/Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h` · `Engine/Config/BaseScalability.ini` · `Engine/Config/BaseDeviceProfiles.ini` · `Engine/Build/Build.version`
**Project (read-only):** `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.cpp` · `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` · `Config/DefaultEngine.ini` · `Saved/Config/WindowsEditor/GameUserSettings.ini` · `Content/` (name census only)
