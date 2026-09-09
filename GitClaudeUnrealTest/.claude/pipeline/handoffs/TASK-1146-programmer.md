# TASK-1146 — [FOGFLOOR-MEASURE] — gameplay-programmer handoff

**Date:** 2026-09-08 · **HEAD at my instant:** `61702e1` (re-derived, `SC-§91`) · **Mode:** DIAGNOSE-ONLY, READ-ONLY
**Net source change: ZERO.** No scratch edit was made. No `.uasset`, no `Config/**`, no `CONVENTIONS.md`, no compile, no editor, no MCP, no git write. The only write is this file.
**Law read first:** `FOG-§12.1`–`§12.6` · `GFX-§12` · the `GFX-§9` 2026-09-08 correction · `GFX-§11` · `SC-§90` · `SC-§94` · `SC-§97` · `SC-§101` · `SC-§39` · `SC-§91`.

---

## 1. THE ASYMMETRY — the answer, in one sentence

> **YES — the mechanical penalty applies IDENTICALLY at every graphics setting while the visual does not, so this is an ASYMMETRIC loss and therefore an EXPLOIT by `GFX-§12`'s own test: at Shadows=Low the player SEES through the fog AND KEEPS his opponent blind, for free.**

### 1.1 How I falsified rather than confirmed it (`SC-§39`)

The expected answer was "settings-independent." I tried to break it by tracing every input the clamp actually reads, to its terminator, rather than by reading the signature and agreeing with the brief.

The clamp's inputs are exactly two, and I read both:

**(a) `bFogActive`** — `FSiegeCombatStatics::ReadFogState`, `SiegeCombatStatics.cpp:196-244`:

```cpp
OutTuning = FSiegeFogTuning();
if (!World) { return false; }
const AFogVolume* const FogVolume = AFogVolume::Find(World);
if (FogVolume && FogVolume->IsFogActive()) { return true; }
return false;
```

and `AFogVolume::IsFogActive()`, `FogVolume.cpp:272-284`, is in its entirety a world-clock comparison:

```cpp
return World->GetTimeSeconds() < FogActiveUntilTimeSeconds;
```

`FogActiveUntilTimeSeconds` is `UPROPERTY(VisibleInstanceOnly, Transient, …) double`, `FogVolume.h:825-826` — per-match runtime state stamped by `RaiseFog`. **No CVar, no `UGameUserSettings`, no scalability group, no viewer, no team, no controller, no local player appears anywhere on this path.**

**(b) `FSiegeFogTuning`** — handed in default-constructed, **unconditionally**, on the first line of `ReadFogState` (`:200`). It is a struct of compile-time constants. The world does not get to influence it; a graphics setting has no route to it even in principle.

Both consumers then take those two values and nothing else:
- `SiegeCombatStatics.cpp:341` — the acquisition clamp inside `GatherHostileAgents`
- `SiegeCombatStatics.cpp:388` — the per-candidate `IsVisibleThroughFog` cut
- `SiegeCombatStatics.cpp:289` — `ResolveFogClampedReachUU` (retention, `TASK-1040`)

**I could not falsify it. The clamp is settings-independent, exactly as `FOG-§12.1` predicted — and that is the bad answer, not the good one.** The visual half is deleted by `r.VolumetricFog=0` at `[ShadowQuality@0]`/`@1` (§2 below); the mechanical half is not deleted by anything. Symmetric loss would have been a tradeoff. This is asymmetric in both directions at once: the low-settings player gains sight *and* retains the blinding he inflicts. `GFX-§12`: exploit.

*(Aside, offered because a reader will check it and should not think it a defect: `AFogVolume` registers no `GetLifetimeReplicatedProps` and no `DOREPLIFETIME` — verified against a positive control, since `BattlefieldScatter.cpp` / `Castle.cpp` / `HeroCharacter.cpp` do contain the symbol, so the reader is live. `FogVolume.cpp:34` says this is deliberate: "REPLICATED WHEN M8 LANDS." Not a finding, and it does not move the answer above — the clamp runs where `GatherHostileAgents` runs.)*

---

## 2. THE FULL `r.VolumetricFog*` FAMILY — every line, every level

Re-derived at engine source at my own instant (`SC-§91`), from
`C:\Program Files\Epic Games\UE_5.8\Engine\Config\BaseScalability.ini`. `TASK-1112`'s numbers are **CONFIRMED**, not inherited.

| Section | Line | CVar | Value |
|---|---|---|---|
| `[ShadowQuality@0]` | :143 | `r.VolumetricFog` | **0** |
| `[ShadowQuality@0]` | — | *`GridPixelSize` / `GridSizeZ` / `HistoryMissSupersampleCount`* | **ABSENT — not written at all** |
| `[ShadowQuality@1]` | :178 | `r.VolumetricFog` | **0** |
| `[ShadowQuality@1]` | — | *the three grid lines* | **ABSENT — not written at all** |
| `[ShadowQuality@2]` | :213 / :214 / :215 / :216 | `r.VolumetricFog` / `.GridPixelSize` / `.GridSizeZ` / `.HistoryMissSupersampleCount` | **1 / 16 / 64 / 4** |
| `[ShadowQuality@3]` | :251 / :252 / :253 / :254 | same four | **1 / 8 / 128 / 4** |
| `[ShadowQuality@Cine]` | :289 / :290 / :291 / :292 | same four | **1 / 4 / 128 / 16** |

**Every other group's sections, all levels: ZERO `r.VolumetricFog*` lines.** I enumerated all 56 sections in the file and grepped the family across the whole ini; the only hits are the eleven lines above. `[EffectsQuality@*]`'s sole fog line is the unrelated `r.Fog.SeparateComposition`.

**And the project adds nothing:** there is **no `Config/DefaultScalability.ini`** in this project (`ls Config/` → `DefaultEditor.ini`, `DefaultEditorPerProjectUserSettings.ini`, `DefaultEngine.ini`, `DefaultGame.ini`, `DefaultInput.ini`, `SiegeCloudDev.ini{,.example}`). We inherit `BaseScalability.ini` wholesale. `Config/DefaultEngine.ini` carries only `r.SupportExpFogMatchesVolumetricFog=False` (:133) and `r.VolumetricFog.LightFunction=True` (:138) — neither gates presence.

### 2.1 🚨 THE DEGENERATE-GRID HYPOTHESIS IS **FALSIFIED** — a naive `r.VolumetricFog=1` **WOULD** render

The brief's most important warning was that forcing the switch onto a degenerate froxel grid would render nothing while every read-back said ON. **Measured: it does not happen, and here is why.**

1. `@0` and `@1` write **only the on/off switch**. They never touch `GridPixelSize` or `GridSizeZ` (table above — the absence is the finding).
2. Scalability applies **only the lines present in the section**: `Scalability.cpp:446`, `ApplyCVarSettingsFromIni(*Section, *GScalabilityIni, ECVF_SetByScalability)`. An unlisted CVar retains its last value; nothing resets it.
3. The engine defaults are **`GVolumetricFogGridPixelSize = 16`** (`Runtime/Renderer/Private/VolumetricFog.cpp:118`) and **`GVolumetricFogGridSizeZ = 64`** (`:126`) — **identical to the `@2` values.**

⇒ A floored `r.VolumetricFog=1` at Shadows=Low sits on a real 16 px / 64 Z grid. **`TASK-1147` will not fail this way.**

### 2.2 ⚠️ But there IS a grid consequence, and it runs the OTHER direction — a COST bug, not a render bug

Because `@0`/`@1` never *reset* the grid, the grid **hystereses**. A player who was at Shadows=Epic (8 px / 128 Z) and then drops to Low keeps the **Epic** froxel grid, because nothing writes it back down.

⇒ With the floor in place, **the cheapest setting would run the most expensive fog** — precisely inverting `GFX-§12`'s "its cost may scale."

⇒ **RECOMMENDATION FOR `TASK-1147`: when it floors `r.VolumetricFog` to 1, it must in the same action floor `r.VolumetricFog.GridPixelSize` to `16` and `.GridSizeZ` to `64` (the `@2` values).** That is the cheapest grid the engine ships fog on, it is what the defaults already are, and it makes the floor cost the low-spec player Medium prices rather than Epic ones. Without it, the fix is correct and the bill is wrong.

### 2.3 🚨 AND THE GATE THAT OUTRANKS THE CVAR — `r.VolumetricFog=1` is NECESSARY, NOT SUFFICIENT

`ShouldRenderVolumetricFog`, `Runtime/Renderer/Private/VolumetricFog.cpp:1355-1364`, is a **six-term conjunction**:

```cpp
return ShouldRenderFog(ViewFamily)
    && Scene
    && GVolumetricFog
    && ViewFamily.EngineShowFlags.VolumetricFog
    && Scene->ExponentialFogs.Num() > 0
    && Scene->ExponentialFogs[0].bEnableVolumetricFog
    && Scene->ExponentialFogs[0].VolumetricFogDistance > 0;
```

The CVar is **one** of six terms. The floor also depends on `bEnableVolumetricFog` and `VolumetricFogDistance > 0` on `L_Arena`'s `ExponentialHeightFog` — the property `FOG-§12.2` says has no code owner. Those are `true` / `6000` today, so **a CVar-only floor is sufficient TODAY** — but it is sufficient *by coincidence of an authored map property nobody in code owns*, and a single unrelated map edit would silently un-floor the fog while every CVar read-back stayed green. **That is the `SC-§94` shape one instrument down, relocated from the grid (where the brief expected it) to the component.** It is why seam (b) below is recommended *alongside* (a) rather than instead of it.

---

## 3. THE SEAMS — reachability, authority, and survival of a mid-match change

### 3.0 FIRST, THE MEASUREMENT THAT DELETES SCOPE: **the player CANNOT open the graphics menu during a match**

- The only route to the panel is `WBP_MainMenu --(Btn_Settings)--> USettingsMenuWidget --(GraphicsButton)--> USiegeGraphicsMenuWidget` — stated as the shipped chain at `SiegeGraphicsMenuWidget.h:185-186`, and live in code at `SettingsMenuWidget.cpp:462` (the click binding) → `:555` (`USiegeGraphicsMenuWidget::CreateAndAddToViewport`).
- `WBP_MainMenu` lives on **`/Game/Maps/L_MainMenu`** — `Config/DefaultEngine.ini:10`, `GameDefaultMap=/Game/Maps/L_MainMenu.L_MainMenu`. The match is `L_Arena`. **Different maps.**
- `USettingsMenuWidget::CreateAndAddToViewport` has **ZERO call sites** in shipping source.
- `grep -rn "PauseMenu\|EscapeMenu\|InGameMenu" Source/**` (excluding tests) → **ZERO hits.** There is no in-match pause menu.

⇒ **`TASK-1147` cl. (3): BUILD NOTHING FOR A MID-MATCH SETTINGS CHANGE.** The player cannot reach the menu without leaving the match.
⚠️ **Leave the comment naming the day this changes:** the day an in-match pause/escape menu ships a Settings entry. (Cheap insurance: seam (a) below survives that change *for free* anyway, so adopting (a) means the scope deletion costs nothing if it is later reversed.)

### 3.1 Seam (a) — **a CVar set at `ECVF_SetByCode` — ✅ RECOMMENDED, and it is the authoritative one**

**Reachable:** yes — `IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"))`, from anywhere, no world required.

**Authoritative:** **yes, and the engine's own priority system does the hard part.** Measured at `Runtime/Core/Public/HAL/IConsoleManager.h`:

| Priority | Value |
|---|---|
| `ECVF_SetByScalability` (:159) | `0x02000000` |
| `ECVF_SetByCode` (:183) | `0x0E000000` |
| `ECVF_SetByConsole` (:187) | `0x10000000` |

⇒ **a value set at `ECVF_SetByCode` is not overwritten by any later scalability apply.** "Survives a mid-match settings change" comes for free, with no delegate, no polling and no tick.

**⭐ THE RELEASE — and this is my one substantive disagreement with the pinned design, flagged not decided.**
`IConsoleVariable::Unset(EConsoleVariableFlags SetBy, FName Tag)` exists at `IConsoleManager.h:643`. `Unset(ECVF_SetByCode)` **removes the code layer** and the CVar falls back to whatever scalability last wrote — i.e. **the player's live choice**, even if he changed it while the fog was up. A captured-and-restored literal cannot do that; it restores the value that was true at capture time.

**⚠️ FAILURE MODE OF THE `Set`-BASED RELEASE, and it is silent:** restoring by `Set(prior, ECVF_SetByCode)` leaves the CVar **pinned at code priority forever after**, so every subsequent scalability apply is *silently rejected*. The player's Shadows slider would stop affecting volumetric fog for the rest of the session, with nothing in any log. This is a *different* bug from the idempotency trap `TASK-1147` cl. (2) already names, and it survives a correct fix to that one. `Unset` avoids both.

⇒ **FLAGGED FOR THE MANAGER (see §5, F-1): `FOG-§12.5` pins `FogRenderFloorPriorState` as a stored value. The measurement says `Unset` is strictly better. I am not re-deciding a pinned name — I am reporting that the pin encodes a weaker mechanism than the engine offers.**

**⚠️ HONEST LIMITATION:** `ECVF_SetByConsole` (`0x10`) **outranks** `ECVF_SetByCode`. A Development build's console can still type `r.VolumetricFog 0` and defeat the floor. Not fixable at this seam; record it rather than imply the floor is absolute.

**COST:** one `FindConsoleVariable` + one `Set` per fog raise. `RefreshFogVisual()` is not a tick.

### 3.2 Seam (b) — **`TActorIterator<AExponentialHeightFog>`, write the component IN MEMORY — ✅ RECOMMENDED ALONGSIDE (a)**

**Reachable:** yes. **Authoritative:** it is the **only** seam that reaches `bEnableVolumetricFog`, `VolumetricFogDistance` and the volumetric albedo/emissive — i.e. terms 5, 6 and 7 of §2.3's conjunction, and the colour that `FOG-§12.4` / `TASK-1155` will need for ask (C).

**Survives a mid-match change:** irrelevant — it is not a scalability-owned value.
**FAILURE MODE:** it is a per-instance runtime write, so it does **not** survive a level reload and must be re-applied. `RefreshFogVisual()` already gives that, since the fog is raised per match. It writes memory only, **saves nothing**, and therefore satisfies `GFX-§11` and the standing arena fence.
**⚠️ It must be paired with (a):** the component property alone is defeated by `r.VolumetricFog=0`, and (a) alone is defeated by a future map edit (§2.3).

### 3.3 Seam (c) — a scalability-change delegate — ⛔ RECOMMEND AGAINST

**Reachable:** yes (`USiegeGraphicsSettingsSubsystem`'s `OnGraphicsSettingsChanged`). **But UNNECESSARY** given (a)'s priority guarantee, **and harmful**: it would be a *second* place that decides whether the fog is floored, for a fact that has one — exactly what `FogVolume.h:31`'s one-reconciler rule and `FogVolume.h:815-816`'s "do not add a companion `bool bFogActive`" ban exist to forbid. It also cannot fire in the state that matters, since §3.0 measured that the menu is unreachable mid-match.

### 3.4 Seam (d) — the game mode's `BeginPlay` — ⛔ RECOMMEND AGAINST

**Reachable:** yes. **Wrong:** it floors for the *whole match*, including the ~99 % with no fog up — charging the cheap-PC player continuously to protect a five-minute spell. That is the cost `GFX-§12` names and forbids.

### 3.5 `AFogVolume::RefreshFogVisual()` — not a rival seam, the **CALLER** for (a)+(b)

It is the one reconciler (`FogVolume.h:873`, defined `FogVolume.cpp:504`), it already has the fog-is-up and fog-is-down branches, and it is already re-entered on every raise, refresh and timer wake-up (`FogVolume.cpp:160`, `:232`, `:269`, `:548`). `FOG-§12.5`'s "called from `RefreshFogVisual()` and nowhere else" is correct and I have no amendment to it.

### 3.6 The three forbidden shortcuts

**I propose none of them and I have no argument for any of them.** (a)+(b) is strictly better than all three: it needs no map save, no ini write, and it charges the Shadows group nothing.

---

## 4. THE `GFX-§12` CENSUS — **CANDIDATES, NOT FINDINGS**

⛔ **Nothing here was fixed. Every entry below is labelled, and the two I could settle by measurement are marked RULED OUT rather than left ominous.**

### C-1 — the invisibility veil (`WITCH-§`) — **CANDIDATE — NOT A FINDING** · ⭐ strongest analogue
`ASummonedUnit::ApplyVeilMaterial()` (`SummonedUnit.cpp:3149`) swaps **every material slot** to `/Game/Materials/MI_Unit_Invisible`. The unit is **never hidden** — no `SetActorHiddenInGame` — and the fallback log at `:3163` states the ruled failure direction in as many words: *"VEILED to enemy acquisition but LOOKS NORMAL."* The mechanical rule is elsewhere and server-side: `FSiegeInvisibilityStatics::IsVisibleTo` (`SiegeCombatStatics.cpp:122`).
⇒ **This is the fog's exact shape:** a client-side material carries the concealment, a server-side rule carries the penalty. If a quality group renders that material as a normal opaque unit, the low-settings player **sees** the veiled unit while still being barred from targeting it — sight plus the enemy's blindness.
**Group + CVar that could do it:** `[EffectsQuality@0]` `r.RefractionQuality=0` (:800 — a refraction/distortion veil, the common implementation, vanishes outright) and `r.MaterialQualityLevel=0` (:803 — a Quality Switch node's Low branch).
**UNVERIFIED.** Settling it requires reading `MI_Unit_Invisible.uasset`'s material graph, which this row's fences forbid. **I did not measure it and I am not claiming it.**

### C-2 — the battlefield scatter as visual cover — **CANDIDATE — NOT A FINDING**
`[ViewDistanceQuality@0]` sets `r.ViewDistanceScale=0.4` (`BaseScalability.ini:112`), which already scales HISM cull distances — **the project asserts this itself** at `SiegeGraphicsSettingsSubsystem.cpp:1129-1131`. ⇒ at View Distance = Low, the grass and trees that visually occlude units cull ~2.5× closer, while the scatter's **blocking** geometry is authority-side and unchanged (`ValidateTraversability` `BattlefieldScatter.cpp:2064`, `CullCorridorBlockers` `:2464`). Same shape: the visual thins, the rule does not.
⚠️ **Whether it crosses `GFX-§12`'s intent-to-hide line is genuinely open** — trees read as world geometry (rightly scalable), tall grass concealing a unit reads much more like fog. **NOT DECIDED HERE.** UNVERIFIED at pixels.

### C-3 — Niagara spell telegraphs and impact tells — **CANDIDATE — NOT A FINDING**
`[EffectsQuality@0]` sets `fx.Niagara.QualityLevel=0` (:825), `r.EmitterSpawnRateScale=0.125` (:809), `r.ParticleLightQuality=0` (:810), `r.DetailMode=0` (:802). A Niagara system whose emitters carry scalability settings can be culled **entirely** at quality level 0. Systems on that path: `/Game/VFX/NS_Spell_<CardID>` (`SpellLibrary.cpp:99`), `/Game/VFX/NS_ChainZap` (`Tower.cpp:43`), `/Game/Variant_Combat/VFX/NS_Damage` (`Projectile.cpp:67`, `SummonedUnit.cpp:237`), `/Game/VFX/NS_CastleDebris` (`Castle.cpp:35`), `/Game/VFX/NS_GoldBurst` (`SummonedUnit.cpp:55`).
⚠️ **DIRECTION MATTERS AND IT IS THE OPPOSITE OF THE FOG:** deleting a telegraph **hurts** the low-settings player — he loses the warning he was meant to dodge. That is a self-inflicted handicap, not an exploit, *unless* a telegraph is what reveals an opponent's position or action, in which case it inverts. **UNVERIFIED** — settling it needs each `NS_` asset's emitter scalability settings, which the fences forbid reading.

### C-4 — the war map / map marks — **CANDIDATE, RULED OUT BY MEASUREMENT**
`USiegeMapMarkSubsystem` is a `ULocalPlayerSubsystem` painted through **Slate** (`SiegeMapMarkSubsystem.h:56`, `:230`). Slate/UMG is not driven by the ten scalability groups. **Not a candidate.**

### C-5 — combatant health bars — **CANDIDATE, RULED OUT BY MEASUREMENT**
`UCombatantHealthBarComponent` is a **screen-space** `UWidgetComponent` (`CombatantHealthBarComponent.h:16`, `:86`, `:121`); `EWidgetSpace::World` is explicitly refused (`VIS-R1`). Slate again. **Not a candidate.**

### C-6 — hit flashes — **CANDIDATE — NOT A FINDING**, and weak
`USiegeHitFlashComponent` uses `SetOverlayMaterial` (`SiegeHitFlashComponent.cpp:83`, cleared at `:102`) — an engine overlay pass with **no on/off CVar in any group's `@0` section**. Only `r.MaterialQualityLevel=0` could alter it, and only via a Quality Switch. Direction is **symmetric** (both players lose the same feedback) ⇒ a tradeoff, not an exploit, under `GFX-§12`'s own test.

### C-7 — the placement ghost and the spell reticle decal — **CANDIDATE — NOT A FINDING**, not an exploit by direction
Both are the **local** player's own aids (`SiegePlayerController.cpp:3708` reticle, `:3909` / `:4342` group-pick circles). Degrading them costs the low-settings player and takes nothing from his opponent ⇒ symmetric-or-worse. Listed only because the brief named it.

### C-8 — **NEW, mine: the ambient height fog as distinct from the siege fog** — **CANDIDATE — NOT A FINDING**
`bEnableVolumetricFog` on `L_Arena`'s `ExponentialHeightFog` is the **same switch**, and `TASK-1147`'s floor will hold it up only for the siege-fog window. Outside that window the ambient haze still vanishes at Shadows=Low/Medium. **Whether the ambient haze itself hides units at range — i.e. whether it is intent-to-hide — is NOT MEASURED.** I name it because `TASK-1147` cl. (6)'s re-worded panel hint turns on exactly this distinction being real, and nobody has checked that it is.

**Census total: 8 candidates examined — 5 live candidates (C-1, C-2, C-3, C-6, C-7), 2 ruled out by measurement (C-4, C-5), 1 new one added by this row (C-8).**

---

## 5. FLAGGED DECISIONS FOR `TASK-1147` (and for the manager)

**F-1 ⭐ — `Unset(ECVF_SetByCode)` vs the pinned `FogRenderFloorPriorState`.** `FOG-§12.5` pins a stored prior-state member; the measurement (§3.1) says `IConsoleVariable::Unset(ECVF_SetByCode)` restores the player's **live** choice and avoids a silent failure the stored-value design cannot avoid (a `Set`-based restore pins the CVar at code priority forever, killing the Shadows slider's effect on fog for the rest of the session, with nothing in any log). **Manager's call — a pinned name is the manager's (`SC-§82`), and `SC-§101` says my prescribed remedy is a claim, not a warrant.** If the pin stands as written, `TASK-1147` must at minimum test the "slider stops working after one fog" case.

**F-2 — the grid must be floored with the switch (§2.2).** Not optional in my reading: without it the floor makes Low cost Epic. `TASK-1147` should floor `GridPixelSize=16` / `GridSizeZ=64`.

**F-3 — the component write (seam (b)) should ship WITH the CVar floor, not after it (§2.3).** A CVar-only floor is sufficient today only by coincidence of an unowned map property. Ruling needed on whether that is in `TASK-1147`'s scope or deferred to `TASK-1155` (which needs the same iterator for colour).

**F-4 — mid-match scope deletion (§3.0), MEASURED: build nothing for it.** Per `TASK-1147` cl. (3), this goes on the row.

**F-5 — the console defeats the floor in Development builds (§3.1).** Accepted limitation or a row of its own; not mine to decide.

**F-6 — C-1 (the invisibility veil) deserves its own diagnose row.** It is the same shape as the fog and I could not settle it inside these fences.

---

## 6. VERIFICATION QUALITY — measured by me vs cited (`SC-§97`)

**MEASURED BY ME, AT SOURCE, THIS INSTANT:**
- every `r.VolumetricFog*` line and its owning section, and the absence of the family from all other groups (`BaseScalability.ini`, read directly)
- the engine CVar defaults `16` / `64` (`VolumetricFog.cpp:118`, `:126`)
- `ShouldRenderVolumetricFog`'s six-term conjunction (`VolumetricFog.cpp:1355-1364`)
- that scalability writes only a section's own lines (`Scalability.cpp:446`), hence the grid hysteresis
- the `ECVF_SetBy*` priority ordering and the existence of `Unset` (`IConsoleManager.h:159`, `:183`, `:187`, `:643`)
- `bFogActive`'s complete derivation to its terminator (`SiegeCombatStatics.cpp:196-244` → `FogVolume.cpp:272-284`) and the unconditional default-construction of `FSiegeFogTuning` (`SiegeCombatStatics.cpp:200`)
- the three clamp consumer sites (`SiegeCombatStatics.cpp:289`, `:341`, `:388`)
- the graphics-menu reachability chain, `GameDefaultMap`, and the absence of any in-match pause menu
- the absence of a project `Config/DefaultScalability.ini`
- `AFogVolume`'s absent replication registration, **against a positive control** (three other classes do carry `GetLifetimeReplicatedProps`, so the reader was proven live before I trusted the zero)
- every census render mechanism cited in §4
- `HEAD` = `61702e1`

**CITED, NOT MEASURED BY ME — accepted from the brief and the law:**
- that `12b8707` / `TASK-1036` is what set `bEnableVolumetricFog = true` on `L_Arena`. I verified only the negative half myself: my own grep of `Source/`, `Config/`, `Tools/` returns no project symbol — every hit is a comment or the unrelated `r.VolumetricFog.LightFunction` / `r.SupportExpFogMatchesVolumetricFog` ini lines. **I did not open the map.**
- `L_Arena`'s `VolumetricFogDistance = 6000` — a header comment (`FogVolume.h:600`) itself citing `TASK-841`. A citation of a citation; I did not read the actor.
- that `61702e1` shipped the Shadows slider — I read the commit **subject**, not the diff.
- 🧑 Jonathan's pixels.

**PIXELS: NOT MEASURED.**
This dispatch fences the editor, MCP, compiling and git (READ-ONLY). No arena capture at Shadows=0 vs Shadows=3 was taken, and I imply no picture I did not see.
⇒ **DUTY TRANSFERRED BY NAME to `TASK-1147`** (it compiles, and can witness the red its cl. (7) names) **and to `TASK-1149`** (the integration host — the board already names it as the first role in this chain that *can*).

---

## 7. FENCE COMPLIANCE

`Source/**` net change: **ZERO**, and no scratch edit was made at any point. `Content/**`, `Config/**`, `Tests/**`, `.uasset`: **untouched**. `CONVENTIONS.md`: **not written** (`SC-§82`). `L_Arena`: **never opened, never saved**. `Content/FogArea/**`: **not touched**; `BP_FogArea`: **not opened**. No compile, no editor, no MCP, no commit, **no push**. The fog **mechanic** — the two constants, the clamp, `FSiegeFogStatics`' signatures — is **byte-for-byte untouched**; §1 only *read* it. Ask (B)'s uniformity question and ask (C)'s colour question were **not attempted** (`VID-007` / `TASK-1151` own them). Writes this row made: **this file only.**
