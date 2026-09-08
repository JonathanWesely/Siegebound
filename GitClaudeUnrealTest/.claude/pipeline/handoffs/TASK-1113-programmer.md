# TASK-1113 — GFX-CORE — handoff (gameplay-programmer)

**Marker:** `TASK-1113-GFX-CORE` · **Date:** 2026-09-07 · **Gate:** `TASK-1114` (qa-reviewer) · **Host:** `TASK-1117`
**Spec source:** the board row + `handoffs/TASK-1112-programmer.md` (the measured API surface). Law: `GFX-§3` `GFX-§4` `GFX-§5` `GFX-§8` `GFX-§9` `GFX-§10` `GFX-§11` · `SC-§79` `SC-§83` `SC-§87` `SC-§90` `SC-§94`.

## Files written (3 — nothing else)

| File | Lines | What |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsSettingsSubsystem.h` | 836 | the facade's API + the canonical-name pin + every documented fallback |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsSettingsSubsystem.cpp` | 1184 | implementation; one mutation path, one save site, one broadcast site |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGraphicsSettingsTest.cpp` | 1115 → **1358** | ~~13~~ **16** automation tests + the SC-§83 mutation table (M1–**M14**). ⛔ The "13" was wrong at loop 0 too — it was 15. See LOOP 1 §4. |

⛔ **ZERO** `Content/**` · **ZERO** `Config/**` · **ZERO** `.uasset` · **ZERO** `CONVENTIONS.md` · **ZERO** edits to `USiegeSettingsSubsystem` · **ZERO** `GitClaudeUnrealTest.Build.cs` (nothing new was needed — `Scalability.h` and `Kismet/KismetSystemLibrary.h` both live in the already-public `Engine` module) · **NO compile, NO editor, NO MCP, NO git.**
⛔ **`TASK-931`'s four files were not touched.** `git status` shows my three new files as the only additions; `CombatantHealthBarComponent.{h,cpp}` · `SiegeCombatStatics.{h,cpp}` · `SummonedUnit.cpp` · `Tests/SiegeInvisibilityTest.cpp` are its dirt, untouched and unread-for-write.

## Clause (4) STOP: **did not fire.** `ConfirmVideoMode()` / `RevertVideoMode()` both exist, both public, both `BlueprintCallable` (`GameUserSettings.h:147` / `:151`). Nothing is hand-rolled and nothing is handed back to `TASK-1118`.

---

## 1. THE FACADE'S SHAPE

`USiegeGraphicsSettingsSubsystem : UGameInstanceSubsystem` — a thin, null-safe wrapper over `GEngine->GetGameUserSettings()`. **No `UGameUserSettings` subclass, no `GameUserSettingsClassName`, no ini edit** (`GFX-§3`). Pinned names shipped character-for-character: `USiegeGraphicsSettingsSubsystem` · `LogSiegeGraphics` · `FOnSiegeGraphicsSettingsChanged` / `OnGraphicsSettingsChanged` (`FName SettingName`).

**Tier A — preset (2).** `GetOverallScalabilityLevel()` (`-1` ⇒ Custom, engine-native) · `SetOverallScalabilityLevel(int32)` · `IsOverallQualityCustom()` · `IsOverallQualityCustomAcrossVisibleGroups()` · `AutoDetectQuality()`.

**Tier B — TEN quality groups (`AS BUILT 10` vs `AS BOARDED ~10`; the engine has 11).** One `BlueprintPure` getter + one `BlueprintCallable` setter each, plus an `FName`-keyed generic pair (`GetQualityGroupLevel` / `SetQualityGroupLevel`) and `GetQualityGroupNames()` so `TASK-1115` builds ten rows in a loop. Labels: `GetQualityGroupDisplayName(FName)` and `GetQualityLevelDisplayName(int32)` — the latter wraps `Scalability::GetScalabilityNameFromQualityLevel` rather than a hand-written Low/Medium/High array.

**Tier C — display (5).** Resolution scale (normalized + percent + sentinel-aware slider seed) · screen resolution (+ a never-empty supported list with count/label/index stepper helpers) · window mode **as `int32`** (+ count/label) · VSync · frame-rate limit on the `GFX-§5` ladder (+ index helpers).

**Tier D — this project's levers (3), shipped HERE per cl. (5).** `GetFoliageDensityScale()` · `GetViewDistanceScale()` · `ShouldEnableVolumetricFog()` — all pure, all side-effect-free, all asserted so by a test. **This is what keeps `TASK-1115` and `TASK-1122` out of the same file.**

**Apply model (`GFX-§4`).** `ApplyQualitySettings()` = `ApplyNonResolutionSettings()` + save, immediately. `ApplyVideoModeProvisional()` = `ApplyResolutionSettings(false)`, **never a save**. `ConfirmVideoModeChange()` / `RevertVideoModeChange()` close the window and release the save.

⚠️ **`EWindowMode::Type` never crosses the facade boundary** (`GFX-§2(d)`), and there is **deliberately no `UENUM` in this class at all** — so the widget cannot put one in a `BlueprintImplementableEvent` even by accident.

---

## 2. THE FOUR SURVEY TRAPS — HOW EACH WAS HANDLED

### T1 — `RevertVideoMode()` applies nothing (`SC-§94` cl. A)
`RevertVideoModeChange()` calls `RevertVideoMode()` **and then `ApplyResolutionSettings(false)`**, in that order, with the reason written at the call site and a ⛔ DO-NOT-DELETE marker. Confirm is `ConfirmVideoMode()` → save; revert is `RevertVideoMode()` → apply → save, exactly the two sequences the survey derived.
**Asserted, not asserted-in-a-comment:** `Siegebound.Graphics.RevertActuallyAppliesTheResolution` checks `ApplyResolutionSettingsCallCount == 2` after provisional+revert. A bare `RevertVideoMode()` leaves it at `1`. That is mutation **M8**.

### T2 — eleven groups, ten shipped
`LandscapeQuality` is **dropped**. Grounds are recorded in three places so it cannot be re-added from a docs page: the header's note (2), the `ReadQualityGroupLevel` fall-through comment, and `Siegebound.Graphics.CanonicalNames`, which asserts the count is `10` and that `LandscapeQuality` is **absent**. Reasons: no `[LandscapeQuality@N]` section at any level, no `PerfIndexThresholds_LandscapeQuality` for auto-detect, no Landscape in this project ⇒ the slider would move and change nothing (`GFX-§9`'s "control that lies").
⚠️ One consequence, named rather than discovered later: `GetOverallScalabilityLevel()` is answered by the engine over **all eleven** groups. Harmless, because `SetOverallScalabilityLevel` writes all eleven and nothing else in this project ever writes the eleventh — and `IsOverallQualityCustomAcrossVisibleGroups()` answers the same question over the ten a player can see.

### T3 — `sg.ResolutionQuality = 0` is a SENTINEL, not 0 %
**Clamped on WRITE, never on READ.**
- `GetResolutionScaleNormalized()` / `GetResolutionScalePercent()` return the **raw** value — `0` stays `0`.
- `IsResolutionScaleProjectDefault()` reports the sentinel explicitly (`<= ResolutionScaleSentinelEpsilon`).
- `GetResolutionScalePercentForSlider()` is a **display transform that writes nothing**: it shows `100 %` while the sentinel is live, otherwise the raw value clamped into `[50, 100]`.
- `SetResolutionScaleNormalized()` clamps to `[0.5, 1.0]`, so a player action can never re-create the sentinel and can never request a 0 % render (the engine floor really is `0.0`).
`Siegebound.Graphics.SentinelIsNotClampedOnRead` reproduces the live ini state on the scratch object (`SetResolutionScaleValueEx(0)`), reads everything, and then asserts **the sentinel is still live and nothing applied, saved or broadcast** — i.e. opening the menu changed nothing. Mutation **M5** is "clamp on read", the trap exactly.

### T4 — naming: the `GFX-§10` pin, paid
⛔ **CANONICAL = THE INI / `sg.*` / CVar SPELLING.**
`ViewDistanceQuality` · `AntiAliasingQuality` · `ShadowQuality` · `GlobalIlluminationQuality` · `ReflectionQuality` · **`PostProcessQuality`** · `TextureQuality` · **`EffectsQuality`** · `FoliageQuality` · `ShadingQuality`
Grounds: it is what a level actually *means*, what an ini/log readback shows, and what `GFX-§10`'s own worked example (`ShadowQualitySlider`) already uses ⇒ `<Group>` == the ini section name minus `@N`. Consequence for `TASK-1115`: the widget triples are `EffectsQualitySlider` / `EffectsQualityLabelText` / `EffectsQualityValueText` and `PostProcessQualitySlider` / … — **never** `VisualEffect*` or `PostProcessing*`.
The engine's two divergent spellings (`SetVisualEffectQuality`, `SetPostProcessingQuality`) appear in this project at **exactly two sites each** — the read and write forwarding lines — and all four are commented `NAME CROSSING #n OF 2`. `CanonicalNames` asserts `VisualEffectQuality` and `PostProcessingQuality` are **not** canonical names; `GroupRoundTrip` asserts the named getter agrees with the generic lookup **and** that neighbours did not move, which is what catches a swapped branch (mutation **M2**).
Also handled: `SetResolutionScaleValueEx` / `GetResolutionScaleInformationEx` are called by their real `Ex` names; `IsVSyncEnabled` (not `Get`); `SetFrameRateLimit` takes a **`float`**; `GetSupportedFullscreenResolutions` is a **`bool`-returning static on `UKismetSystemLibrary`**, and the **windowed** sibling is used when the mode is windowed.

---

## 3. TWO THINGS I FOUND THAT THE SURVEY DID NOT, AND BOTH ARE LIVE

### F-A 🚨 `ApplySettings(bool)` CALLS `SaveSettings()` ON ITS LAST LINE — `GameUserSettings.cpp:590-601`
This is the shortest route to breaking `GFX-§4`, and it is the obvious-looking one. **`ApplySettings` is called ZERO times in this facade**; the quality path uses `ApplyNonResolutionSettings()` and the video path uses `ApplyResolutionSettings(false)`.
And the deeper hazard it exposes: `SaveSettings()` writes `ResolutionSizeX/Y` and `FullscreenMode` along with everything else, so **an innocent quality-slider drag during the confirmation countdown would persist an unconfirmed video mode** — the lockout arriving through a control that is not the resolution control. Handled structurally:
- `RequestSaveSettings()` is the **only** `SaveSettings()` call site in the file (grep it: one hit), and it **refuses** while `IsVideoModeChangePending()`, counting the refusal and latching a deferred flush.
- Confirm and Revert both close the window and flush the deferred save, so a legitimate quality change made during the countdown is not silently lost.
`Siegebound.Graphics.SaveUnreachableFromProvisionalVideoMode` asserts the whole sequence, including the mid-countdown quality drag. Mutation **M7**.

### F-B 🚨 A **FRESH INSTALL** IS ALREADY IN AN UNCONFIRMED VIDEO MODE
`SetToDefaults()` sets `FullscreenMode = GetDefaultWindowMode() == WindowedFullscreen (1)` (`GameUserSettings.cpp:296`, `:831-834`) but **never touches `LastConfirmedFullscreenMode`**, a plain `int32` `UPROPERTY` defaulting to `0`. ⇒ `1 != 0` before the player has done anything.
Without a repair, the save guard in F-A would refuse **every** save forever on a first launch, and the symptom would be "my graphics settings don't stick" with only a `Log` line to explain it. `Initialize()` therefore heals a stale unconfirmed mode by calling `ConfirmVideoMode()` once, with a `Warning`. The argument for why that is truthful rather than a shortcut: the engine has **already brought the game up in that mode**, so it is demonstrably displayable and the player is demonstrably looking at it — which is the entire question the confirmation asks.
⚠️ **A flagged decision for the gate.** See F-1 below.

---

## 4. TIER-D DERIVATIONS (cl. 5) AND WHAT THEY OWE `TASK-1122`

| Getter | Derivation | Why |
|---|---|---|
| `GetFoliageDensityScale()` | Foliage: `0.25 / 0.50 / 0.75 / 1.00 / 1.00` | Epic `1.00` because `DA_BattlefieldScatter`'s authored values **are** the Epic baseline (`GFX-§9`); Cinematic never **exceeds** it. |
| `GetViewDistanceScale()` | **`1.0` at every level in v1** | ⚠️ **A measurement, not a stub.** `[ViewDistanceQuality@N]` already sets `r.ViewDistanceScale` (`0.4` @0 → `1.0` @3) and that CVar already scales HISM cull distances ⇒ a project-side multiplier on `SetCullDistances` would **double-scale** (Low becomes `0.4 × ours`). The control does **not** lie, because the engine lever behind it is real. |
| `ShouldEnableVolumetricFog()` | **Shadow** group `>= 2` | 🚨 `GFX-§9` says **Effects**; the engine says **Shadow**. See F-3. |

🚨 **CARRIED FORWARD INTO THE GETTER'S OWN DOC COMMENT, because a downstream row will read the code and not this file:** `GetFoliageDensityScale()` is a **per-machine** value and the scatter is under the M8/D9 determinism contract. Multiplying `Layer.InstanceCount` by it changes the iteration count of a loop drawing from a **shared `FRandomStream`**, so a server at Foliage=Epic and a client at Foliage=Low produce **two different battlefields from one seed**, and every layer processed afterwards is displaced too. ⛔ **A consumer may not let this value change the RNG draw sequence.** Safe shapes are render-side only.

---

## 5. TESTS — `Source/.../Siegebound/Tests/SiegeGraphicsSettingsTest.cpp`

**13 tests.** ⛔ **WRONG — CORRECTED IN LOOP 1 §4: it was 15 (the list below already named 15), and this loop ships 16.** Hermetic: every subsystem is pointed at a scratch `NewObject<UGameUserSettings>()` **and** engine applies are suppressed **in the same call**, so a run cannot read, write or apply the machine's real `GameUserSettings.ini` and cannot change the live editor's resolution. The scratch object is `ConfirmVideoMode()`-ed on creation to neutralise F-B.

`CanonicalNames` · `GroupRoundTrip` · `OutOfRangeWriteClamps` · `DelegateNeverFiresOnNoOp` · `CustomWhenGroupsDisagree` · `NullSettingsDegradesToFallbacks` · `SentinelIsNotClampedOnRead` · `ResolutionScaleClampsOnWrite` · `SaveUnreachableFromProvisionalVideoMode` · `RevertActuallyAppliesTheResolution` · `FrameRateLadderSnapsAndLabels` · `WindowModeContract` · `TierDFoliageDensityTable` · `VolumetricFogFollowsShadowNotEffects` · `TierDReadsArePure`.

### 🚨 `SC-§83` — the named mutations, and **NO WITNESSED RED**
The full table (M1–M12) is written at the top of the test file next to the code it mutates. Headline pairs: **M8** delete the `ApplyResolutionSettings` from the revert path ⇒ `RevertActuallyAppliesTheResolution`; **M7** delete the save guard ⇒ `SaveUnreachableFromProvisionalVideoMode`; **M5** clamp the resolution scale on read ⇒ `SentinelIsNotClampedOnRead`; **M1** spell `EffectsQuality` the engine's way ⇒ `CanonicalNames`; **M10** derive fog from Effects ⇒ `VolumetricFogFollowsShadowNotEffects`; **M3** delete the no-op early-return ⇒ `DelegateNeverFiresOnNoOp`.

⛔ **NO WITNESSED RED.** None of M1–M12 was executed. This row does not compile (the editor holds the DLL and the host compiles with it closed) and does not run the suite. **Every mutation above is a derived prediction from reading the code, not an observed transition** — a claim to be checked, not evidence (`SC-§90`: plausibility is not a measurement).
**Suite: NOT RUN by this row. The last real executed baseline is `498 / 0`; this row adds 13 tests, so the expected new total is `511`, and that number is a PREDICTION until the host executes it under an `SC-§87` bound.** ⛔ Do not quote `511` as measured.
⛔⛔ **THE ARITHMETIC ON THIS LINE IS WRONG AND IS SUPERSEDED BY LOOP 1 §4.** It was 15 tests, not 13, so loop 0's own prediction should have read `513`; **this loop ships 16 ⇒ `498 + 16 = 514`.** ⛔ Do not quote `511`, ⛔ do not quote `513`, and `514` is still UNMEASURED.

### `SC-§79` — what these tests **cannot** detect (written in the file too)
They cannot see whether the **screen** changed, whether a CVar moved, or whether a rendered frame differs. With suppression on, the `Apply`/`Save` counters record the facade's **decision** — that it reached the call site — not the engine's effect. The decision is exactly what `GFX-§4` constrains, so it is the right instrument for **that** failure class and for no other. The `GFX-§9` / `SC-§94` pixels-and-log obligation is **entirely unspent** and is owed by `TASK-1118` / `TASK-1119` / `TASK-1122`.

---

## 6. FLAGGED DECISIONS FOR THE GATE (`TASK-1114`) AND THE MANAGER

| # | Decision | Why it needs a ruling |
|---|---|---|
| **F-1** | `Initialize()` **auto-confirms** a stale unconfirmed video mode at boot. | It is a mutation the player did not ask for, on a control class `GFX-§4` treats as dangerous. My argument (§3 F-B) is that the game is already displaying that mode. The alternative — revert instead of confirm — would desync the fields from the screen the engine actually brought up. **Wanted: a yes/no.** |
| **F-2** | The canonical group-name pin is the **ini spelling** (`EffectsQuality`, `PostProcessQuality`). | `GFX-§10` owed this and I made the call, following `TASK-1112`'s recommendation. It fixes `TASK-1115`'s widget names character-for-character. If the manager prefers the engine-wrapper spelling, ten widget names and this facade change together. |
| **F-3** | 🚨 `ShouldEnableVolumetricFog()` derives from **Shadow**, contradicting `GFX-§9`'s "← the Effects group". | The engine owns `r.VolumetricFog` in `[ShadowQuality@N]` (`0` at @0/@1). An Effects derivation ships a live contradiction: Shadows=Low + Effects=Epic ⇒ engine turns fog **off** while our named switch says on. **`CONVENTIONS.md` is the manager's (`SC-§82`); `GFX-§9` needs re-wording or an explicit override.** |
| **F-4** | `GetViewDistanceScale()` returns **`1.0` at every level**. | Neutral by measurement, not unfinished. If `TASK-1122` rules a project lever in, it needs a **compensating** table and a **re-measured** band — `TASK-1084`'s "`0.8`" is not reproducible from config (`SC-§91`). |
| **F-5** | Quality setters **save on every real change** (the `USiegeSettingsSubsystem` idiom). | A slider drag that emits many discrete detent changes writes the ini once per detent. Cheap, and each write is a real change — but if the panel drives continuous updates it may want a debounce. Named, not silently accepted. |
| **F-6** | `GetOverallScalabilityLevel()` reports **Custom** when the **resolution scale** disagrees with the preset, not only when a group does (`Scalability.cpp:1083-1097`). | Engine-native and arguably correct, but a panel showing "Custom" with ten matching sliders looks broken. Pinned by a test; `TASK-1115` should choose deliberately which reading it labels. |
| **F-7** | ⛔⛔ **OVERRULED — THIS WAS THE BLOCKER. See LOOP 1 §0 and §1.** ~~`AutoDetectQuality()` is **not** refused during a pending video-mode window.~~ | ~~Traced rather than assumed: `ApplyHardwareBenchmarkResults` saves via `Scalability::SaveState`, which writes **only** `[ScalabilityGroups]`…~~ **The trace stopped two lines short.** The body is `GameUserSettings.cpp:1132-1143` and `:1142` is `SaveSettings()` — a full `SaveConfig` writing `ResolutionSizeX/Y` and `FullscreenMode`. `AutoDetectQuality()` **is** now refused while a video-mode change is unconfirmed. |

---

## 7. WHAT QA SHOULD SCRUTINISE

1. **The save trace (`TASK-1114` cl. c), by grep rather than by comment.** `grep -n "SaveSettings(" SiegeGraphicsSettingsSubsystem.cpp` ⇒ **one** hit, inside `RequestSaveSettings`, **below** the `IsVideoModeChangePending()` guard. `grep -n "ApplySettings("` ⇒ **zero**. If either count is wrong, `GFX-§4` is broken.
2. **The delegate is structural (cl. d).** `OnGraphicsSettingsChanged.Broadcast` appears **once**, in `BroadcastGraphicsSettingChanged`. Every mutation compares-before-write before reaching it. ⚠️ The one mutation whose new value we do not choose — `AutoDetectQuality` — snapshots before and compares after; check that logic, it is the easiest place to have got the no-op law wrong.
3. **Null-safety on every public entry point (cl. e).** Every method begins with `ResolveSettings()` and bails. ⚠️ Check `GetSupportedScreenResolutions()` in particular: it is the one that still calls an engine static with a null settings pointer, deliberately, and must never return an empty array.
4. **The group list came from the MEASUREMENT, not from `GFX-§8`** (cl. a). Ten, `LandscapeQuality` dropped, reasons in three places.
5. **The Tier-D getters are pure (cl. g)** — `TierDReadsArePure` asserts zero applies, saves and broadcasts across 32 read cycles.
6. **F-1 is the decision I am least comfortable with** and the one I most want a second opinion on. It is a write the player did not request, on the class of setting `GFX-§4` exists to protect.
7. ⚠️ **Nothing here has been compiled or executed.** Treat the mutation table and the `511` count as predictions. The three most likely compile risks, since I could not check them: UHT's acceptance of the `BlueprintPure` static functions returning `TArray<>` by value; `TestEqual`'s overload resolution in the test file (I moved every `FIntPoint` comparison to `.ToString()` and every `bool` comparison to `TestTrue`/`TestFalse` precisely to avoid it); and `Scalability.h` resolving from the `Engine` module's public include path.

---
---

# LOOP 1 — the fix for `TASK-1114` (FAIL: 1 BLOCKER · 7 WARN · 7 NIT)

**Date:** 2026-09-07 · **Marker:** `TASK-1113-GFX-CORE-LOOP1` · **QA report:** `.claude/pipeline/qa/TASK-1114.md` · **Loop 1 of 3.**
**Files touched — three, the same three, nothing else:** `SiegeGraphicsSettingsSubsystem.h` (836 → 883) · `SiegeGraphicsSettingsSubsystem.cpp` (1184 → 1286) · `Tests/SiegeGraphicsSettingsTest.cpp` (1115 → 1358).
⛔ **ZERO** `Content/**` · **ZERO** `Config/**` · **ZERO** `CONVENTIONS.md` · **ZERO** `SiegePlayerController.cpp` (`TASK-1132` holds it) · **NO compile, NO editor, NO MCP, NO git.**

## 0. THE THING I GOT WRONG, STATED PLAINLY

**The gate is right and my F-7 was wrong.** I quoted `ApplyHardwareBenchmarkResults` as `:1132-1140` and reasoned about `Scalability::SaveState` at `:1139`. **The body is `:1132-1143` and its second-to-last line is `SaveSettings()` at `:1142`** — a full `SaveConfig(CPF_Config, GGameUserSettingsIni)` (`:683`) writing `ResolutionSizeX/Y` (`GameUserSettings.h:465`) and `FullscreenMode` (`:500`). **I re-read both engine functions myself before accepting the finding** rather than taking the report on trust, and they read exactly as the gate says:

```
GameUserSettings.cpp:1132  void UGameUserSettings::ApplyHardwareBenchmarkResults()
                    :1135      Scalability::SetQualityLevels(ScalabilityQuality);
                    :1139      Scalability::SaveState(GGameUserSettingsIni);   <- where I stopped
                    :1142      SaveSettings();                                 <- the defect
                    :1143  }
```

I had already written the general form of this defect myself, in F-A: *"the lockout arriving through a control that is not the resolution control."* I then failed to apply it to the one control that reaches a save without passing through my own save site. **The mechanism was read correctly; the reachability was not.** That is the `SC-§90` failure the gate names, and I am recording it as mine.

## 1. THE BLOCKER FIX — `SiegeGraphicsSettingsSubsystem.cpp`, `AutoDetectQuality()`

Inserted **immediately after the `ResolveSettings()` null check and ABOVE the automation-suppression branch** (the gate's placement, and deliberate — below it the guard would be unreachable from a test and therefore unproven):

```cpp
if (IsVideoModeChangePending())
{
    ++RefusedSaveWhileVideoModePendingCount;
    UE_LOG(LogSiegeGraphics, Warning,
        TEXT("[SiegeGraphics] AutoDetectQuality REFUSED — a video-mode change is unconfirmed and the benchmark's own save would persist it (refusal #%d). Confirm or revert first."),
        RefusedSaveWhileVideoModePendingCount);
    return false;
}
```

- Guard reason written above it at engine-line precision, including ⛔ **DO NOT hand-roll `RunHardwareBenchmark` + `SetQualityLevels` in place of `ApplyHardwareBenchmarkResults`** (the gate's warning — it would silently drop `Scalability::SaveState`).
- ⛔ It does **not** set `bSaveDeferredByVideoModePending`. There is nothing to flush: the benchmark never ran, so no legitimate change is being withheld. A deferred flag here would make Confirm re-run nothing and log a save that never happened.
- The suppression branch now carries a comment saying it leaves the refusal counter **unchanged** — that is what makes the two `false` returns distinguishable.

**Doc comments corrected (comment-only, `SC-§93` cl. 1(a) ⇒ no decoy owed):** `.h` (the `AutoDetectQuality` block) and `.cpp` (above the benchmark call). Both now quote **`:1132-1143`**, name **both** saves with their line numbers, and carry ⛔ *"an earlier revision stopped the trace at `:1140` and concluded the opposite; do not shorten the range back"* — because the truncated range is the artefact that would repeat the defect (`SC-§97`).

**New test — `Siegebound.Graphics.AutoDetectRefusedDuringUnconfirmedVideoMode`.** Stages a mode, applies it provisionally, then asserts `AutoDetectQuality() == false` **and** `RefusedSaveWhileVideoModePendingCount` incremented by exactly 1, and that nothing applied, broadcast or saved and the mode is still pending. Then confirms, and asserts `AutoDetectQuality()` is **still `false`** but with the refusal counter **unmoved** — the two falses are separated. ⚠️ The comment says why the counter and not `SaveSettingsCallCount` is the instrument: **the engine's save is not at a counted facade call site, so `SaveSettingsCallCount` reads 0 on the broken build and the fixed one alike.** That is precisely why M1–M12 could not see this failure class at all.

**New mutation `M13`** (delete the guard) ⇒ RED on the new test, **plus a named weaker variant** (move the guard *below* the suppression branch): the first `TestFalse` still passes and **only the counter assertions catch it** — which is why that test asserts the counter twice rather than the bool twice.

## 2. F-1 — HEAL KEPT, JUSTIFICATION REWRITTEN (WARN-7)

**The heal stays** (the gate ruled yes, on premises I verified myself: `PreloadResolutionSettings` boots from the ini's *staged* `FullscreenMode`/`ResolutionSizeX/Y` at `:771-775` → `RequestResolutionChange` at `:816`; `ConfirmVideoMode` is five assignments at `:269-274` and reaches no disk; reverting instead would desync the object from the live screen).

**My fresh-install justification was FALSE and is deleted.** I verified the correction at source: the engine heals that case itself, at **two** sites, both gated on a **zero** resolution —

| site | code |
|---|---|
| `GameUserSettings.cpp:628` / `:632` | `bool bDetectingResolution = ResolutionSizeX == 0 \|\| ResolutionSizeY == 0;` then `ConfirmVideoMode();` |
| `GameUserSettings.cpp:470-480` | `if (ResolutionSizeX <= 0 …)` stamps all five `LastConfirmed*` / `LastUserConfirmed*` fields; reached from `ApplyNonResolutionSettings` (`:510`) and `ApplyResolutionSettings` (`:573`) |

Both run during engine init, long before any `UGameInstanceSubsystem`. ⇒ *"the save guard would refuse every save forever on a first launch"* is **not true of a booted game.**

**Rewritten to the case that is genuinely uncovered:** the **abandoned or crashed countdown** — a *perfectly non-zero* staged resolution with a confirmed triple that disagrees with it. Both engine heals test for **zero** and both decline; nothing else in the engine looks. Rewritten in **three** places, not one:
1. `cpp` — the `Initialize()` heal comment, now naming both engine heal sites with line numbers and ⛔ flagging the old justification as corrected.
2. `test` — `MakeScratchStore`'s comment. ⚠️ **The claim is still true there and I did not weaken it:** a bare `NewObject<UGameUserSettings>()` runs the constructor's `SetToDefaults()` and **never `LoadSettings()`**, so the `ConfirmVideoMode()` in the harness is genuinely required. What I added is the ⛔ **scope line: that is a test-harness necessity, NOT the justification for the boot heal — do not read one as evidence for the other.**
3. This handoff (§3 F-B above is superseded by this section).

**And I carried the gate's condition (i) into the code**, because it is a real coupling: the heal is safe *precisely while nothing can wrongly persist a mode*, since the heal turns a persisted staged mode into a **permanently confirmed** one on the next launch. The `Initialize()` comment now says ⛔ **the `AutoDetectQuality` refusal is this function's precondition — do not keep one without the other.**

## 3. M4 — THE RULING, AND I TAKE THE GATE'S SIDE

**M4 does not derive. I verified it at source** (`Scalability.cpp:1117-1160`): every one of the ten `FQualityLevels::Set<Group>Quality` is `FMath::Clamp(Value, 0, CVar<Group>_NumLevels->GetInt() - 1)` — the **identical** band. `-7` lands on `0` and `99` on `4` with or without my facade's clamp ⇒ **`OutOfRangeWriteClamps` stays GREEN under M4. My prediction that it would redden was wrong.**

**I took the "say it plainly" option AND re-pointed the mutation**, because both were owed:
- **M4 re-pointed** at `DelegateNeverFiresOnNoOp`, where the facade's clamp-**before**-compare really is the only thing standing (an out-of-range write that clamps *onto the current value* would broadcast without it).
- **`OutOfRangeWriteClamps` re-labelled in its own doc block:** for the ten groups and the overall level it is an **engine-backstopped contract check** — it answers board cl. (6) *"an out-of-range write clamps rather than asserts"* and **asserts the engine, not us.** ⛔ It is not evidence about this diff.
- **Named the one clamp with no engine backstop:** the resolution scale (`Scalability.h:242` `MinResolutionScale = 0.0f` — the engine really would pass a 0% render), asserted by `ResolutionScaleClampsOnWrite` / **M6**, which *is* evidence about this diff.
- The mutation table's footer now says ⛔ **M4's original prediction was one of these claims and it was false — read the rest with the same suspicion.**

## 4. THE CORRECTED ARITHMETIC — AND IT IS 16, NOT 15, NOT 13

| | |
|---|---|
| what the loop-0 prose, §5 heading and board row said | **13** ⛔ wrong (§5's own list already named 15) |
| what the gate measured | **15** ✅ correct for loop 0 |
| **what this loop ships** | **16** — 15 + `AutoDetectRefusedDuringUnconfirmedVideoMode` |
| last real **executed** baseline | **`498 / 0`** ⛔ never `306` (`SC-§95` cl. 2) |
| expected new total | **`498 + 16 = 514`** ⛔ not `511`, ⛔ not `513` |

⛔ **`514` IS A PREDICTION AND IS UNMEASURED**, exactly like `511` was. Only `TASK-1117`'s executed `N / M` under an `SC-§87` bound counts. Verified by census, not by memory: `IMPLEMENT_SIMPLE_AUTOMATION_TEST` × **16**, and all 16 names listed distinct.

## 5. EVERY WARN AND NIT — ACTED ON OR NOT, WITH A REASON FOR EACH

| # | finding | did I act? | what / why |
|---|---|---|---|
| **BLOCKER-1** | Auto-Detect persists an unconfirmed mode | ✅ **FIXED** | §1. Guard + corrected comments ×2 + new test + M13. |
| **WARN-1** | test count / suite total wrong on own inputs | ✅ **FIXED** | §4. 13 → **16**; `511` → **`514`, labelled unmeasured**. Corrected in the handoff and the board row. |
| **WARN-2** | `OutOfRangeWriteClamps` vacuous; M4 does not derive | ✅ **FIXED, both halves** | §3. M4 re-pointed at `DelegateNeverFiresOnNoOp`; the test's doc block now declares itself an engine-backstopped contract check. |
| **WARN-3** | staged-but-never-confirmed mode disables saves for the session | ⛔ **NOT MINE — declined, routed** | The gate routed this to the manager as a **`TASK-1115`/`TASK-1118` duty** (`Back`/close must revert or discard). My dispatch fences `TASK-1115`-facing work out of this loop explicitly. ⚠️ **If the manager instead rules the discard into the facade, that is a code change and I should get it as a new row** — the facade would grow `DiscardStagedVideoMode()`. I did **not** add it speculatively: an unused public mutator on the class `GFX-§4` exists to protect is worse than an absent one (`SC-§36.1` — a built, tested, zero-caller trigger is a surface, not a fix). |
| **WARN-4** | `GFX-§9`'s fog clause wrong in both clauses | ⛔ **NOT MINE — and already resolved above me** | `CONVENTIONS.md` is the manager's (`SC-§82`, `GFX-§11`). ⭐ **The manager has since DROPPED the project fog lever**, and my `ShouldEnableVolumetricFog()` (Shadow-derived, `>= 2`) is **correct and stays**, with `VolumetricFogFollowsShadowNotEffects` / **M10** unchanged. Consumer is the panel, read-only. **Not re-argued.** |
| **WARN-5** | a preset press moves TWO values and announces ONE | ✅ **FIXED — code + test + mutation** | `SetOverallScalabilityLevel` now snapshots `GetResolutionScaleNormalized()` before the write and, **only if it actually moved**, also broadcasts `SettingName_ResolutionScale` with a log line naming `Scalability.cpp:1047`. Idiom is `AutoDetectQuality`'s own snapshot-and-compare, as the gate suggested. Header sentence added: ⛔ *a preset press overwrites the scale and, on a first-ever run, destroys the sentinel.* ⚠️ **A behaviour change with no test is what a gate should catch, so I did not ship one:** `CustomWhenGroupsDisagree` now asserts (a) the scale really was overwritten, (b) **two** broadcasts, (c) the second one **names ResolutionScale**, (d) a real preset write whose scale does **not** move broadcasts **once**, (e) re-pressing the same preset broadcasts **neither**. New mutation **M14**. |
| **WARN-6** | "only something outside this facade can move the 11th" is too strong | ✅ **FIXED (comment, both sites)** | `AutoDetectQuality` **is** such a mover: `ComputeQualityLevelsFromPerfIndex` computes `LandscapeQuality` (`Scalability.cpp:727`) and, with no `PerfIndexThresholds_LandscapeQuality` in our ini, uses `ComputeOptionFromPerfIndex`'s hard-coded `{20, 50, 70}` (`:210-214`) while the other ten use their own table. Written into **both** the `GetOverallScalabilityLevel` header block and the `AutoDetectQuality` header block, and the `TASK-1115` instruction upgraded from *recommended* to ⛔ **MANDATORY: label from `IsOverallQualityCustomAcrossVisibleGroups()`, never from `GetOverallScalabilityLevel()`.** |
| **WARN-7** | fresh-install justification is false | ✅ **FIXED in 3 places** | §2. Heal kept, justification moved to the abandoned/crashed-countdown case; the test-harness claim kept (it is true there) with an explicit ⛔ scope line so the two are not conflated. |
| **NIT-1** | "one site each" said three different ways | ✅ **FIXED** | Measured: **4 sites, 2 funnels, 2 per concept, 1 per symbol per funnel.** Header parenthetical and cpp banner now both say that and both name both funnels. The four in-place markers already read `#1/#2 OF 2 (read side)` / `(write side)` and were left as they are — they are self-consistent; it was the two summaries that drifted. |
| **NIT-2** | "a bare `NewObject` is at engine defaults" is not exact | ✅ **FIXED** | Corrected: `FObjectInitializer`'s archetype copy runs **after** the C++ constructor and the CDO is config-backed ⇒ a scratch instance can carry **this machine's values** (never its file). Added the operative rule: ⛔ **no assertion in the file may assume a starting value.** Acted on rather than filed as documentation, because it is the root cause of NIT-3. |
| **NIT-3** | two host-state-dependent assertions | ✅ **FIXED, both** | (a) `WindowModeContract` now seeds `SetWindowMode(1)` + `ConfirmVideoModeChange()` and asserts nothing is pending, so the pending assertion is about the **second** write only. (b) `DelegateNeverFiresOnNoOp` now seeds `SetFrameRateLimit(60.f)` — a known ladder rung — captures the count, then re-writes 60; the two downstream assertions were converted from the hard-coded `4` to that captured value. **Reason for fixing rather than deferring: a host-dependent red would be misread as a facade defect and would cost a QA loop over nothing.** |
| **NIT-4** | force-null leaves suppression off | ✅ **FIXED (the gate's own "cheap hardening")** | `SetForceNullGameUserSettingsForAutomationTests(true)` now also sets `bSuppressEngineApplyForAutomationTests = true`. ⚠️ It only ever **sets**, never clears, so the ordering in the null test (`…(nullptr)` then `…(true)`) is correct. Reason for taking it: **M12 removes the exact check the current hermeticity rests on, at which point the mutated test writes the developer's real ini.** A mutation that damages the host is not an acceptable price for proving a guard. Verified no assertion depends on the old behaviour — under force-null every entry point bails at `ResolveSettings()` before any counter moves, so all counters stay 0 either way. |
| **NIT-5** | counters are adjacent to, not inside, the engine call | ⛔ **NOT ACTED ON — by design, and declared** | That adjacency is **what makes the counters work under suppression**, it is already declared in the header, and the gate agrees it is honest. The residual it names (a mutation deleting only the engine call is invisible) is real and **cannot be closed by any in-process test** — it is the pixels-and-log rung, owed by `TASK-1118`/`1119`/`1122`. Closing it here would mean removing suppression, i.e. letting a unit test change the editor's resolution. Declining deliberately, not overlooking. |
| **NIT-6** | four tests walk `Warning` log paths | ⛔ **NOT ACTED ON — for the host's eye** | The gate marks it "for the host's eye only" and notes 44 existing test files use the same flags. Silencing a `Warning` the code is *supposed* to emit would delete the evidence the test exists to walk past. |
| **NIT-7** | no `ShouldCreateSubsystem`; created on a dedicated server | ⛔ **NOT ACTED ON — the gate rules it correct** | It is the **designed degradation** (`GetFoliageDensityScale()` → `1.0` is the M8/D9-safe answer, and the fallbacks deliberately preserve the authored baseline). The gate named it only so a server log line is not misread as a defect. Adding the override would suppress a facade whose null path is already correct and tested. |
| §7 items 1-9 | host duties | ⛔ **NOT MINE** | `TASK-1117`'s: the compile, the executed `N / M`, the ini byte-diff, the tree-half scope check, the `Build.bat` exit-code trap. I hold no compiler and no `git` this loop. |
| §8 items 1-6 | manager duties | ⛔ **NOT MINE** | `CONVENTIONS.md` is the manager's. Item 1 is already resolved (WARN-4 row). |

## 6. `SC-§83` / `SC-§90` — WHAT THIS LOOP DID **NOT** MEASURE

⛔ **NO WITNESSED RED.** Nothing in this loop was compiled, executed, rendered or looked at. **M13 and M14 are derived predictions exactly like M1–M12 were — and M4 is the standing proof that a derived prediction in this very file can be wrong.** The suite total `514` is a prediction. The only new *facts* I added are engine source lines I opened and read myself (`GameUserSettings.cpp:1132-1143`, `:672-683`, `:628-632`, `:470-480`, `:267-274`, `:765-816`; `Scalability.cpp:1117-1160`; `GameUserSettings.h:465`, `:500`, `:504`) — those are readings, and they are checkable.

Structural invariants re-censused after the edits, since I changed the file the gate verified: **`Settings->SaveSettings()` × 1** (inside `RequestSaveSettings`, below its guard) · **`ApplySettings(` × 0** call sites (2 hits, both comments) · **`OnGraphicsSettingsChanged.Broadcast` × 1** real site (2 hits, one a comment). Brace and paren balance checked on all three files after comment/string stripping: **0 / 0**.

## 7. WHAT THE GATE SHOULD LOOK AT HARDEST THIS LOOP

1. ⭐ **The new guard's PLACEMENT, not its presence.** Above the suppression branch is load-bearing. If a future edit moves it below, the new test's first `TestFalse` still passes and only the counter assertions fail — that is M13's weaker variant and it is written into the table.
2. **The WARN-5 second broadcast is a real behaviour change**, the only one in this loop that alters what a caller observes. Check the no-op law still binds it: `CustomWhenGroupsDisagree` asserts both the two-broadcast case (scale moved) and the **one**-broadcast case (real preset write, scale unchanged), and re-pressing the same preset broadcasts neither.
3. **NIT-3's two seeds changed the arithmetic in `DelegateNeverFiresOnNoOp`.** Its final two assertions are now relative to a captured count instead of the literal `4`. Re-walk that counter chain.
4. **NIT-4 changed an automation seam's semantics.** Confirm the null test's call order still yields the intended state and that nothing downstream wanted the live path back.
5. ⚠️ **My M13/M14 predictions deserve the same suspicion M4 earned.** I have now been wrong once in this file about a mutation that "obviously" reddens.
