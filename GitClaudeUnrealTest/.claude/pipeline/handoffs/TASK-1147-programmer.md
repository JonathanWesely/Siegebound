# TASK-1147 — [FOGFLOOR] — gameplay-programmer handoff

**Date:** 2026-09-08 · **HEAD at my instant:** `61702e1` (re-derived, `SC-§91`) · **Suite baseline declared:** `552 / 0` at `61702e1` — **quoted, not re-measured** (I do not compile; `TASK-1149` is the host).
**Law read first:** `GFX-§12` · `GFX-§11` · the `GFX-§9` 2026-09-08 correction · `FOG-§12.1`–`§12.6` · `SC-§79` · `SC-§83` + **Addendum B** · `SC-§94` · `SC-§101` · `SC-§104`.
**Compile:** ⛔ **NO WITNESSED RED.** I did not compile and did not run the suite. Every mutation below is a **prediction**, and the duty to witness the red is transferred **BY NAME** to ⭐ **`TASK-1149`** (`SHIP-§9`).

---

# §0 🚨🚨 READ THIS BEFORE THE REST — **MY FLOOR AND 🧑 HIS COMPLAINT MAY BE ABOUT TWO DIFFERENT FOGS**

> ### ⛔ I built the seam the board specified, it is correct for what it claims, and **it is not established that it fixes what he reported.** That sentence is the deliverable of this row as much as the code is.

**What happened:** `TASK-1151` landed while I was building, and it establishes **on rendered pixels** that there are **two fogs**, and that `TASK-1146` and `TASK-1151` were each looking at a different one.

| | the **AMBIENT** fog | the **CARD'S** fog |
|---|---|---|
| what it is | `L_Arena`'s `ExponentialHeightFog` + UE's froxel volumetric system | `BP_SiegeFog` — a **raymarched translucent mesh**, density computed inside `MF_Fog` |
| governed by | `r.VolumetricFog` (Shadows group) + `bEnableVolumetricFog` on the map actor | its own material parameters |
| in the froxel grid? | **yes** | ⛔ **no** — and neither material sets `bUsedWithVolumetricFog` |
| **what my floor reaches** | ✅ **this one** | ⛔ **not this one** |
| rendered contribution at his vantage | ⛔ **≈ 0** — `TASK-1151` captured the enemy castle **crisp at ~50,000 uu**, height fog live, `BP_SiegeFog` absent by construction, no wash at any depth | 100 % of every pixel of wash in all five fogged `VID-007` frames |

⇒ ⚖️ ***`r.VolumetricFog` GOVERNS THE AMBIENT FOG. ON THE CURRENT EVIDENCE IT DOES NOT GOVERN THE FOG HE SEES WHEN HE PLAYS THE CARD.***

## §0.1 What I did about it — the code read the orchestrator asked for, and it came back **negative**

I could not use the editor or MCP (fenced), so I did a **positive-controlled name-table scan** of all **27** vendor packages under `Content/FogArea/**` plus `Content/Blueprints/BP_SiegeFog.uasset`. This is a read-only byte scan of files on disk: ⛔ no editor, ⛔ no asset load, ⛔ nothing opened, ⛔ nothing dirtied. `BP_FogArea` was **not opened** (`FOG-§12.6` intact).

**Positive control first (`SC-§39` — the reader is proven live before I trust a zero):** the same scan finds `Mesh` ×7, `RelativeScale3D` ×1 and `StaticMeshComponent` ×1 in `BP_FogArea.uasset`, and `MaterialExpressionTime` ×1 + `MaterialExpressionCameraPositionWS` ×1 in `MF_Fog.uasset` — **independently corroborating `TASK-1151`'s own node census.** The name tables are plainly readable.

| candidate route to delete/degrade `BP_SiegeFog` | the mechanism, at engine source | result |
|---|---|---|
| `r.DetailMode=0` at `[EffectsQuality@0]` (`BaseScalability.ini`) | `USceneComponent::ShouldComponentAddToScene()` is `return DetailMode <= GetCachedScalabilityCVars().DetailMode;` — **`SceneComponent.cpp:3552`**. A component above the cvar is **not added to the scene at all**. | ⛔ **REFUTED.** `DetailMode` appears **ZERO** times in the name table of **any** of the 28 packages ⇒ never overridden ⇒ it holds the `USceneComponent` default **`DM_Low` (0)** ⇒ `0 <= 0` is true and the mesh is added **at every level**. |
| `r.MaterialQualityLevel=0` at `[EffectsQuality@0]` — a cheaper raymarch branch, e.g. fewer steps | requires a Quality Switch node in the graph | ⛔ **REFUTED.** **ZERO** `MaterialExpressionQualitySwitch`, `MaterialExpressionFeatureLevelSwitch` and `MaterialExpressionShadingPathSwitch` anywhere in the pack. There is **nothing to select a cheaper branch with.** |
| draw-distance culling (`r.ViewDistanceScale=0.4` at `[ViewDistanceQuality@0]`) | scales an authored max draw distance | ⛔ **REFUTED for authored values.** **ZERO** `LDMaxDrawDistance` / `CachedMaxDrawDistance` / `MinDrawDistance` / `bAllowCullDistanceVolume` overrides anywhere, and the project sets none (`grep Source/ Config/` → zero hits for all four). A scale on "never cull" is still never cull. |
| `r.VolumetricFog=0` at `[ShadowQuality@0]`/`@1` | `ShouldRenderVolumetricFog`'s six-term conjunction | ⛔ **does not govern it** — `BP_SiegeFog` is not a froxel participant (`TASK-1151`) and `bUsedWithVolumetricFog` is **not set** on either material. |

⇒ 🚨 **I found NO measured route by which any graphics setting deletes or thins the card's fog.**

## §0.2 The consequence, stated plainly, because it is uncomfortable

There are exactly two live possibilities and **I am not picking one**:

- **(A) The exploit does not exist as diagnosed.** The card's fog already appears at every setting. In that case 🧑 Jonathan's report was a correct reading of **our own menu string** — *"Also drives volumetric fog, which the engine turns OFF at Low and Medium"* — which **shipped in `61702e1`, the day before he raised it**, and which `GFX-§9`'s own correction describes as *"he read the consequence of a ruling I wrote the day before."* On this branch the row's **real** deliverable is the hint-string correction in §5, and the floor is defensive depth.
- **(B) The exploit exists by a route nobody has named.** Candidates I could **not** settle, named rather than implied away: `r.SceneColorFormat=3` at `[EffectsQuality@0]` (a lower-precision target under an **additive** blend could plausibly thin the accumulation — not delete it); `r.TranslucencyLightingVolume.Dim=24` / `.Blur=0`; and `sg.TextureQuality` against the **virtual** `VT_Noises` under `VID-007`'s crop-verified **2798.546 MB** VRAM overdraft. **On this branch my floor does not fix it either.**

⇒ **In both branches, the floor is not the fix for the card's fog.** That is the gap, and it is declared here by name so `TASK-1148` gates it.

## §0.3 🚨 THE DECISIVE MEASUREMENT, NAMED — and it needs the editor, which I do not have

> **Raise the fog in PIE at `sg.ShadowQuality 3` / `sg.EffectsQuality 3`, capture the hero vantage; then set `sg.ShadowQuality 0` and capture again; then `sg.EffectsQuality 0` and capture again. Same camera, same seed, three frames.**
> · The wash survives all three ⇒ **branch (A)**: no exploit, and the shipped hint string is the whole fix.
> · The wash dies at **Shadows=0** ⇒ my floor is exactly right and should be verified to restore it.
> · The wash dies at **Effects=0** ⇒ **branch (B)**, the route is in the Effects group, and a **new row** is owed — ⛔ not a widening of this one.

⚠️ `TASK-1151` warns that a fog number needs a **convergence series**, never a single capture (`TASK-841` §3.2 measured 55.8 % vs 100.8 % obscuration at *identical* settings). Whoever runs this must hold the camera still and let it settle.

## §0.4 ⛔ What I did NOT do, and why

I did **not** widen the diff toward `BP_SiegeFog`, a material parameter, a detail mode or a component visibility write. The orchestrator's instruction was explicit and I agree with it: **do not widen on a hypothesis.** Every candidate above is unmeasured, and this project's own `FIELD-§7` / `TASK-1109` history is three confident causal stories refuted in a week.

---

# §1 THE SEAM — `file:line`

| thing | where |
|---|---|
| **`AFogVolume::EnforceFogRenderFloor()`** | declared `FogVolume.h:1152` · defined **`FogVolume.cpp:883`** |
| **`AFogVolume::ReleaseFogRenderFloor()`** | declared `FogVolume.h:1200` · defined **`FogVolume.cpp:1033`** |
| **`AFogVolume::FogRenderFloorPriorState`** (pinned member) | `FogVolume.h:1218` — private, **not** a `UPROPERTY` |
| **`AFogVolume::bEnforceFogRenderFloor`** (pinned tunable) | `FogVolume.h:932` — `EditDefaultsOnly`, default `true`, `HIGH-§1` consequence beside it |
| ⭐ **the only enforce call site** | **`FogVolume.cpp:546`**, inside `RefreshFogVisual()`'s **fog-is-up** branch, **before** `SpawnFogVisual()` |
| ⭐ **the only release call site** | **`FogVolume.cpp:534`**, inside `RefreshFogVisual()`'s **fog-is-down** early-returning branch, beside `DestroyFogVisual()` |
| the pure state transition | `CaptureFogRenderFloorPriorState` — `.h:778` / **`.cpp:845`** |
| the one spelling of the guard | `IsFogRenderFloorEngaged` — `.h:786` / `.cpp:866` |
| the released state | `ReleasedFogRenderFloorState` — `.h:794` / `.cpp:874` |
| the one height-fog lookup | `FindHeightFogComponent` — `.h:1167` / `.cpp:820` |
| the three cvar names | declared `.h:806/809/812`, **defined once each** `.cpp:816-818` (the `USiegeSettingsSubsystem::SettingsSlotName` house pattern) |
| the two structs | `FFogRenderFloorObservation` `.h:27` · `FFogRenderFloorPriorState` `.h:72` — file scope, **plain structs, never `USTRUCT`** |

**`FOG-§12.5` compliance:** all four pinned names are used **character-for-character**; the pair is called from `RefreshFogVisual()` and **nowhere else** (asserted at 1 occurrence each, whole-file); the tests went into the **existing** files; no new log category.

**Mechanism.** The cvar is written at **`ECVF_SetByCode` (`0x0E000000`)**, which outranks **`ECVF_SetByScalability` (`0x02000000`)**; `FConsoleVariableBase::CanChange` is `NewPri >= OldPri` (`ConsoleManager.cpp:275`), so a later scalability apply is **refused by the engine itself**. ⇒ *"survives a settings change"* costs **no delegate, no poll, no tick** — `PrimaryActorTick.bCanEverTick` stays `false`.

The cvar is **necessary, not sufficient**: `ShouldRenderVolumetricFog` (`VolumetricFog.cpp:1355`, re-read at my own instant) is a six-term conjunction whose terms 5–7 are `Scene->ExponentialFogs.Num() > 0`, `bEnableVolumetricFog` and `VolumetricFogDistance > 0`. So the enforce **also** writes the height-fog component via `SetVolumetricFog(true)`, **in memory**, on the running world's own instance. ⛔ No package is marked dirty, no save is called, `L_Arena` is never opened (`GFX-§11` intact — and asserted: `MarkPackageDirty`/`SavePackage` are pinned at **0** in `FogVolume.cpp`).

⚠️ **`VolumetricFogDistance` is READ and REPORTED, never written.** Flooring it would be inventing a design number. If it reads `<= 0` the enforce logs `Error` and names `AFogVolume::FogVisualHorizontalMarginUU`'s transcription of `6000` as the thing that has gone stale with it.

---

# §2 THE RELEASE — and why `Unset` beats a pinned restore

`ReleaseFogRenderFloor()` calls **`IConsoleVariable::Unset(ECVF_SetByCode)`** on all three variables (asserted at exactly 3), and **never** writes one back by value (`->Set(` pinned at **0** in that body).

**Why (this is `TASK-1146`'s flagged decision F-1, which the orchestrator adopted and I implemented):**

- ⛔ A `Set(prior, ECVF_SetByCode)` restore leaves the variable **pinned at code priority forever after**, so **every** subsequent scalability apply is silently refused: the player's Shadows slider would **stop affecting volumetric fog for the rest of the session**, with nothing in any log. That trades one integrity bug for a different one.
- ⭐ `Unset` removes **only our layer**; the variable falls back through its own priority history to whatever **scalability last wrote** — i.e. the player's **live** choice, even if he changed it while the fog was up. A captured literal can only restore the value that was true at capture time.

**And it is reachable here — verified at engine source, not assumed (`SC-§91`, and this was a real risk):**

- `FConsoleVariable<T>::Unset` returns immediately when `PriorityHistory == nullptr` (`ConsoleManager.cpp:1191-1194`). ⭐ Our **own** `Set` allocates it — `TrackHistory` (`:1121-1128`) creates the history and seeds it with the pre-existing value at `ECVF_SetByConstructor`. ⇒ by the time a release can run, the history **exists**, because only an enforce can engage the guard.
- ⛔ `FDelegatedConsoleVariable::Unset` is a **no-op** — *"The getter is always authoritative"* (`ConsoleManager.cpp:1526-1529`). **None of these three is that kind:** `r.VolumetricFog`, `.GridPixelSize` and `.GridSizeZ` are all `FAutoConsoleVariableRef` (`VolumetricFog.cpp:~70`, `:118`, `:126`) ⇒ `FConsoleVariableRef` ⇒ `FConsoleVariableExtendedData`, which is the branch that **owns** `PriorityHistory` and a real `Unset`.

**The component half still restores a captured value**, because a map-actor property has no priority stack to fall back through — guarded by `Observed.bHeightFogFound` so a component we never read is never written.

**The release reads back too (`SC-§94` cl. B).** There is deliberately **no expected value** to compare against — the point is that the value is now the player's, whatever it is. What **is** checkable is that **our layer is gone**: the variable must no longer report `ECVF_SetByCode`. If it does, the release logs `Error` naming exactly the failure `Unset` was chosen to avoid.

⚠️ **HONEST LIMITS, recorded rather than implied away:**
1. **`ECVF_SetByConsole` (`0x10000000`) outranks `ECVF_SetByCode`.** A Development build's console can type `r.VolumetricFog 0` and defeat the floor. Not fixable at this seam.
2. **While the floor is held, each scalability apply logs one `LogConsoleManager: Warning … was ignored as it is lower priority than the previous 'SetByCode'` per floored variable** (`CanChange`, `ConsoleManager.cpp:~288`). ⛔ **That warning IS the floor working.** It stops the moment the fog lifts. Do not read three warnings as three defects.
3. **M8 residual, declared:** console variables are per-process, so this floors whichever machine runs the reconciler. Today that is one machine. When `AFogVolume` replicates (`FogVolume.cpp:34` — *"REPLICATED WHEN M8 LANDS"*), a client that never runs `RefreshFogVisual()` never floors its own cvar. The floor **inherits** the visual's existing M8 debt; it does not add a new one, and it is fixed in the same action.

---

# §3 THE GRID FLOOR, AND THE HYSTERESIS IT PREVENTS

`FogRenderFloorGridPixelSize = 16` (`.h:835`) and `FogRenderFloorGridSizeZ = 64` (`.h:838`) are floored **in the same action** as the switch — three `Set` calls, asserted at exactly 3.

- ⭐ **The degenerate-grid trap is FALSIFIED and I inherit that finding rather than re-derive it:** `[ShadowQuality@0]`/`@1` write **only** the on/off switch — the grid lines are **absent** — and scalability applies only a section's own lines (`Scalability.cpp:446`). Engine defaults are already `16` (`VolumetricFog.cpp:118`) and `64` (`:126`), identical to `[ShadowQuality@2]`. A naive `r.VolumetricFog=1` **would** render.
- ⚠️ **But the grid hystereses the other way, and that is what these two lines are for:** because `@0`/`@1` never *reset* the grid, a player who was at **Epic** (`8` px / `128` Z) and drops to **Low** keeps the **Epic** grid. With a switch-only floor, **the cheapest setting would run the most expensive fog** — precisely inverting `GFX-§12`'s *"its cost may scale."*
- ⇒ `[ShadowQuality@2]`'s figures on purpose: the **cheapest grid the engine ships fog on**, and the one the defaults already hold. `HIGH-§1` consequence is written beside the constant.

**This hysteresis is also load-bearing in the test fixture:** TEST 8's "player at Low" observation carries `8 / 128` — the Epic grid left behind — which is what makes it distinguishable from the floor's `16 / 64`. A fixture self-check reds if a future edit ever makes them equal.

---

# §4 THE RE-ENTRANCE GUARD — cl. (2), and its **state** test

**The trap:** `RefreshFogVisual()` runs on **every** raise, refresh and timer wake-up. A second enforce that re-captured would read back the **already-floored** machine and record **the floor itself** as the player's choice ⇒ the release would "restore" the floor, i.e. become a **permanent no-op**, silently upgrading a Low-settings player's shadows for the rest of the session, invisible in every log.

**The cure — the guard and the capture are the SAME FACT.** `FFogRenderFloorPriorState::bEngaged` is set by the capture and tested by it, so *"capture twice"* is not a mistake the shape can express:

```cpp
// FogVolume.cpp:845
if (IsFogRenderFloorEngaged(Prior))
{
    return Prior;   // ⛔ UNCHANGED — the whole guard
}
```

**And the guard is LIVE, not decorative.** The enforce does **not** short-circuit before it; it captures through the pure function on every call and only the **writes** are gated:

```cpp
const FFogRenderFloorPriorState Previous = FogRenderFloorPriorState;
FogRenderFloorPriorState = CaptureFogRenderFloorPriorState(Previous, Observed);
if (IsFogRenderFloorEngaged(Previous)) { return; }
```

⇒ on every re-entry the guard genuinely executes and its early return is what keeps the record from moving. A dead guard behind an earlier `return` would be `SC-§36.1` — a tested seam nothing reaches.

⛔ **It is not a companion `bool bFogActive`.** It answers a **different** question — *"has a code-priority layer been pushed onto the console variables?"* — which has no other representation anywhere and **cannot** be derived from `IsFogActive()`: that predicate is `true` on the first enforce **and** on the re-entrant one, so deriving the guard from it would guard nothing. The existing banned-flag census (`bFogActive`, `bFogVisible`, …) still reads **0** in both files.

**The test is `SC-§104`-compliant by construction:** `Siegebound.Fog.TheIntegrityFloorCannotRecordItsOwnFlooredValuesAsThePlayersChoice` (`SiegeFogVisualTest.cpp:1067`) asserts **which value is held**, never a tally. ⛔ **A call count here is EQUAL on both branches by construction — the fixed build and the broken build both call the capture twice** — and I say so in the test file rather than counting it as evidence. The rows that cannot discriminate are **labelled `FIXTURE SELF-CHECK (equal on both branches)`**, not quietly counted.

⚠️ **The release also resets the guard on EVERY path out**, including the failure path — a release that reported a failure and kept the guard up would make the **next** match's enforce a no-op, turning one bad session into every session after it.

⚠️ **The tunable gates ENGAGING and never RELEASING**, and the asymmetry is deliberate: if the release honoured `bEnforceFogRenderFloor`, flipping it off mid-window would **strand** the cvars at code priority forever. Pinned at 0 occurrences in the release body.

---

# §5 THE HINT STRING — old text, shipped text, and the draft I **withdrew**

`SiegeGraphicsMenuWidget.cpp:185` / `:188`.

**OLD (verbatim, what shipped in `61702e1`):**
> `"Also drives volumetric fog, which the engine turns OFF at Low and Medium. Currently: fog ON."`
> `"Also drives volumetric fog, which the engine turns OFF at Low and Medium. Currently: fog OFF."`

**SHIPPED NOW:**
> `"Also drives the world's ambient volumetric fog, which the engine turns OFF at Low and Medium. Currently: ambient fog ON. Lowering Shadows does not remove the Fog card's siege fog — its presence is a gameplay rule, not a graphics option."`
> `"… Currently: ambient fog OFF. Lowering Shadows does not remove the Fog card's siege fog — its presence is a gameplay rule, not a graphics option."`

**Why the old one had to go, on either branch of §0.2:** it tells a competitive player, **in the game's own menu**, that dropping Shadows turns the fog off. If the exploit exists it advertises it; if it does not, it invents it. Either way it is `SC-§94` **pointed at the player**, which is what `GFX-§7`'s learnability clause exists to prevent.

🚨 **AND THE DRAFT I WITHDREW, named rather than silently omitted.** My first version said *"The Fog card's siege fog **always appears at every setting** — it is a gameplay effect, not a graphics option."* ⛔ **I removed it.** That is a claim about all **ten** quality groups, and after §0 I can substantiate it for **one**. Shipping it would have replaced one false menu sentence with a **wider** false one — this row's own failure mode, one draft later. The shipped sentence is scoped to **Shadows**, the row the hint sits beside, and it is true **twice over**: `BP_SiegeFog` is not a froxel participant so Shadows never governed it, **and** the ambient half is now floored while a fog is up.

**A test now pins the withdrawal:** `TestFalse(FogOn.Contains("every setting") || FogOff.Contains("every setting"))`. Mutation **M24** below is the widening.

⛔ **`ShouldEnableVolumetricFog()` and its test `VolumetricFogFollowsShadowNotEffects` STAY, untouched** — they describe the **engine**, and the engine is unchanged. `SiegeGraphicsSettingsTest.cpp` and `SiegeGraphicsSettingsSubsystem.{h,cpp}`: **zero bytes.**

⚠️ The existing assertions in that test still hold and I verified both strings against them: they differ (`TestNotEqual`) and both contain `"Low and Medium"`.

---

# §6 cl. (3) — **VOID**, and the measurement that voids it

⛔ **I built nothing for a mid-match settings change**, because the player cannot make one. `TASK-1146` §3.0 measured, and I state it here so it is on this row too:

- the only route to the panel is `WBP_MainMenu → USettingsMenuWidget → USiegeGraphicsMenuWidget` (`SettingsMenuWidget.cpp:462` → `:555`);
- `WBP_MainMenu` lives on **`/Game/Maps/L_MainMenu`** (`Config/DefaultEngine.ini:10`, `GameDefaultMap`). The match is **`L_Arena`**. **Different maps.**
- `USettingsMenuWidget::CreateAndAddToViewport` has **zero** call sites;
- `PauseMenu` / `EscapeMenu` / `InGameMenu` → **zero** hits in `Source/` outside tests. **There is no in-match pause menu.**

⭐ **The scope deletion costs nothing even if it is reversed:** seam (a)'s priority guarantee already survives a mid-match change **for free**. The comment naming the day this changes — *"the day an in-match pause/escape menu ships a Settings entry"* — is written into the `.cpp` banner block, not left in this file only.

---

# §7 TESTS AND NAMED MUTATIONS

**Two new tests in the existing `Siegebound/Tests/SiegeFogVisualTest.cpp` (no new file, `FOG-§12.5`):**

| test | lane | line |
|---|---|---|
| `Siegebound.Fog.TheIntegrityFloorCannotRecordItsOwnFlooredValuesAsThePlayersChoice` | ⭐ **A — genuinely EXECUTED** (pure statics, no world, no actor, no asset load) | `:1067` |
| `Siegebound.Fog.TheIntegrityFloorIsEngagedAndReleasedByTheOneReconcilerAndByNothingElse` | ⚠️ **B — source-text structure** | `:1193` |

Plus **four new assertions** inside the existing `Siegebound.GraphicsMenu.ShadowHintTracksVolumetricFog` (`SiegeGraphicsMenuTest.cpp:1001`).

⛔ **`SC-§83`: the mutation budget goes to the AUTHORED guard first**, and **`SC-§83` Addendum: at least one mutation targets the CALL SITE, not the helper.** Every prediction is in **Addendum B** form (*"≥ N rows, including &lt;named&gt;"*), and per **`SC-§104` cl. 5** the asserted value is computed on **both** branches before it is claimed.

### M1 ⭐⭐⭐ (the board's own suggestion, **and the call-site mutation**) — delete `EnforceFogRenderFloor();` from `RefreshFogVisual()` (`FogVolume.cpp:546`)
⇒ **RED: ≥ 2 rows**, including `…IsEngagedAndReleasedByTheOneReconciler…`'s **"THE FLOOR IS CALLED"** row and its **"the enforce lives in the ONE reconciler"** row; and the four ordering rows **fail by construction** because `SubstringBefore` errors on a missing marker (`SC-§38`).
**BOTH BRANCHES:** fixed = `1`; broken = `0`. ⛔ Not equal.
⛔ **This is the only mutation in the set that distinguishes *"the function exists and is correct"* from *"the function is CALLED"*.** TEST 8 stays **fully green** under it — which is exactly `SC-§83`'s addendum, and this project has already shipped a built, integration-checked, committed asset with **zero callers** while nothing failed.

### M2 ⭐⭐⭐ (**the authored guard**) — delete the `if (IsFogRenderFloorEngaged(Prior)) { return Prior; }` early return from `CaptureFogRenderFloorPriorState` (`FogVolume.cpp:845`)
⇒ **RED: ≥ 3 rows**, including *"A SECOND enforce does NOT re-capture"*, *"the recorded GRID is still the one the player arrived with"* and *"…on the Z axis too"*.
**BOTH BRANCHES, COMPUTED:** fixed holds `0 / 8 / 128` (the player's); broken holds `1 / 16 / 64` (the floor's). ⛔ **Not equal** ⇒ genuine reds.
⚠️ **STRUCK from this prediction, and named:** every assertion about the **first** capture — `bEngaged == true`, `Observed.VolumetricFogEnabled == 0`, `Observed.GridPixelSize == 8`. The first capture is **identical on both branches**, so those rows are **green on both** and measure nothing about M2. They are labelled `FIXTURE SELF-CHECK` in the file.

### M3 ⭐⭐ — make `ReleasedFogRenderFloorState()` return an **engaged** state (`{ true, … }`), i.e. a release that forgets to reset
⇒ **RED: ≥ 2 rows**, including *"A released floor is NOT engaged"* and *"After a release, the NEXT fog captures the machine AFRESH"*.
**BOTH BRANCHES:** row 1 — fixed `false`, broken `true`; row 2 — fixed `1`, broken `0` (the stale default). ⛔ Not equal.
⛔ **Live cost:** the exploit returns silently **one match later**, with a green suite.

### M4 ⭐⭐ — delete `ReleaseFogRenderFloor();` from the fog-is-down branch (`FogVolume.cpp:534`)
⇒ **RED: ≥ 2 rows**, including *"exactly one ReleaseFogRenderFloor() call"* and *"THE RELEASE IS ON THE FOG-IS-DOWN BRANCH"*.
**BOTH BRANCHES:** fixed `1`, broken `0`. ⛔ Not equal.
⛔ **Live cost:** *"a seam that can be entered and not left is half a seam"* — the player's graphics settings stay overridden **permanently**.

### M5 ⭐⭐ — gate the release on `bEnforceFogRenderFloor` (add `if (!bEnforceFogRenderFloor) { return; }` at the top of `ReleaseFogRenderFloor`)
⇒ **RED: 1 row** — *"THE RELEASE NEVER READS bEnforceFogRenderFloor"*.
**BOTH BRANCHES:** fixed `0`, broken `1`. ⛔ Not equal.
⛔ **Live cost:** flipping the switch off mid-fog pins the cvars at code priority for the session. ⚠️ **This mutation is invisible to every other test in the suite**, including TEST 8.

### M6 ⭐⭐ — replace the three `Unset(ECVF_SetByCode)` calls with `Set(Prior.VolumetricFogEnabled, ECVF_SetByCode)` etc.
⇒ **RED: ≥ 2 rows**, including *"The release UNSETS the code layer on all three"* (fixed `3`, broken `0`) and *"it never writes a console variable back by value"* (fixed `0`, broken `3`). ⛔ Not equal.
⛔ **Live cost:** the Shadows slider silently stops affecting fog for the rest of the session — **a different bug from M2's, and one that survives a correct fix to M2.**

### M7 ⭐ — floor only the switch (delete the two grid `Set` calls)
⇒ **RED: 1 row** — *"The grid is floored alongside the switch — three writes, not one"*. **BOTH BRANCHES:** fixed `3`, broken `1`. ⛔ Not equal.
⛔ **Live cost:** an Epic→Low player runs the **Epic** froxel grid at Low — correct picture, wrong bill.

### M8 ⭐⭐ — move the observation read **after** the first write (the ordering twin of M2)
⇒ **RED: 1 row** — *"THE READ HAPPENS BEFORE THE FIRST WRITE"*. **BOTH BRANCHES:** fixed `true`, broken `false`. ⛔ Not equal.
⛔ **This defect is invisible to TEST 8** — it produces the same corruption as M2 through **ordering** rather than re-entry, and no pure-function test can see it. It is why the wiring test exists.

### M23 ⭐⭐⭐ — restore the pre-floor hint wording (in `SiegeGraphicsMenuTest.cpp`'s own table)
⇒ **RED: ≥ 3 rows**, including the *"Lowering Shadows does not remove…"*, *"ambient volumetric fog"* and *"ambient fog ON/OFF"* rows. **BOTH BRANCHES:** fixed `true` ×3, broken `false` ×3. ⛔ Not equal.
⚠️ **STRUCK, and named:** the existing `TestNotEqual` row and the `"Low and Medium"` row. **The old strings also differ from each other and also contain "Low and Medium"** ⇒ both are **green on both branches**. ⭐ That is exactly why the file's existing **M10** could never have caught the false sentence: it tests that the line **moves**, never that it is **true**.

### M24 ⭐⭐ — widen the hint to the draft I withdrew (*"always appears at every setting"*)
⇒ **RED: ≥ 2 rows**, including the `"every setting"` `TestFalse` row and the *"Lowering Shadows does not remove…"* row. **BOTH BRANCHES:** fixed `false`, broken `true`. ⛔ Not equal.
⚠️ The `"every setting"` row is **green on both branches of M23** and is therefore **struck from M23's prediction** — it guards M24 and nothing else.

**Suite delta (`TL-§5b`), declared as a PREDICTION:** `552 → 554` (**+2** automation tests). ⛔ No test removed, no expectation moved. ⛔ **I did not run it.**

---

# §8 FLAGGED DECISIONS FOR GATE `TASK-1148`

| # | decision | my position | who rules |
|---|---|---|---|
| **F-1** 🚨🚨 | **Does this row ship at all, given §0?** The floor is correct for the **ambient** fog and does not reach the **card's** fog, and §0.1 found **no measured route** by which any setting deletes the card's fog. | ⭐ **SHIP IT, but never as "the exploit is fixed."** Grounds: it is `FOG-§12.2`'s pinned seam, it is genuinely correct within its scope, it costs three cvar writes and one component write **per fog window**, and the **hint-string correction is unconditionally right on both branches of §0.2**. ⛔ But `TASK-1149`'s commit message must say *"floors the AMBIENT volumetric fog and corrects a false menu string"*, **not** *"closes the see-through-fog exploit"*. | ⚖️ **manager / orchestrator** |
| **F-2** 🚨 | **`GFX-§9`'s 2026-09-08 correction and `FOG-§12.1` assert a causal story `TASK-1151` appears to refute** — *"Shadows on Low or Medium turns the fog visual off"* is true of the **ambient** fog and, on current evidence, **not** of the siege fog. | ⛔ **Not mine to edit** (`SC-§82` — `CONVENTIONS.md` is the manager's). I report it. A strike-in-place with history kept is the established shape here (this is the **third** correction inside `GFX-§9`). | ⚖️ **manager** |
| **F-3** | **Does the exploit exist?** §0.3 names the decisive measurement; it needs the editor, which this row is fenced from. | ⛔ **OPEN.** Owed to `TASK-1149` (the first role in this chain that can run PIE) or to a new diagnose row. | ⚖️ **orchestrator** |
| **F-4** ✅ | **`Unset` vs the pinned `FogRenderFloorPriorState` literal restore** (`TASK-1146` F-1). | ✅ **IMPLEMENTED AS `Unset`**, per the orchestrator's instruction. ⭐ The pinned **name** survives with a **stronger** job: it is now the re-entrance guard **and** the component's restore source **and** the log's evidence. Nothing was renamed. | ✅ already ruled |
| **F-5** | **`ECVF_SetByConsole` defeats the floor in Development builds.** | Accepted limitation, documented at the declaration. Not worth a row in my view. | ⚖️ manager |
| **F-6** | **A `LogConsoleManager: Warning` per floored cvar per scalability apply, while the floor is held.** | **Expected, not a defect** — it is the engine reporting that the floor worked. Flagged so no one reads three warnings as three failures. | — |
| **F-7** | **`FFogRenderFloorObservation` was built as `TASK-1153`/`1155`'s extension point, and `TASK-1151` closed both rows unfired.** ⇒ it has **exactly one** consumer today. | Declared in the struct's own doc rather than left as a promise. ⛔ Not a defect; a count. | — |
| **F-8** | **`static const TCHAR*` + `.cpp` definition** was chosen over `static constexpr const TCHAR*` in the UCLASS body. | Grounds: the project has **zero** precedent for the latter inside a `UCLASS` and UHT parses that body; `USiegeSettingsSubsystem::SettingsSlotName` is the proven house pattern, and it matches `FogVisualClassPath`'s own *"the path lives in the .cpp"* split. | — |

---

# §9 VERIFICATION QUALITY (`SC-§97`) — measured by me vs cited

**MEASURED BY ME, AT SOURCE, THIS INSTANT:**
- `ECVF_SetByScalability = 0x02000000` / `ECVF_SetByCode = 0x0E000000` / `ECVF_SetByConsole = 0x10000000` (`IConsoleManager.h:159`, `:183`, `:187`) and `CanChange` = `NewPri >= OldPri` (`ConsoleManager.cpp:275`)
- ⭐ that `Unset` is **real** for these three cvars and **not** the delegated no-op — the `FConsoleVariableRef → FConsoleVariableExtendedData` inheritance, `PriorityHistory`'s allocation in `TrackHistory`, and the `FDelegatedConsoleVariable::Unset` no-op it is **not** (`ConsoleManager.cpp:965`, `:1087`, `:1121`, `:1191`, `:1526`, `:1744`)
- ⛔ **and the `PriorityHistory == nullptr` early return** — the one way `Unset` could have silently done nothing, and why it cannot here
- `ShouldRenderVolumetricFog`'s six-term conjunction (`VolumetricFog.cpp:1355`) and the `16` / `64` defaults (`:118`, `:126`)
- `UExponentialHeightFogComponent::SetVolumetricFog` is `ENGINE_API`, is a **no-op when the value already matches**, and marks render state dirty only on a real change (`ExponentialHeightFogComponent.cpp:357-365`)
- ⭐ **`USceneComponent::ShouldComponentAddToScene()` = `DetailMode <= GetCachedScalabilityCVars().DetailMode`** (`SceneComponent.cpp:3549-3552`) and the `EDetailMode` default `DM_Low`
- `[EffectsQuality@0]`'s full cvar list, including `r.DetailMode=0` and `r.MaterialQualityLevel=0` (`BaseScalability.ini`)
- ⭐ the **positive-controlled name-table scan** of 27 vendor packages + `BP_SiegeFog` (§0.1)
- **77 source-census assertions** — every pre-existing pin in `SiegeFogVisualTest.cpp`, `SiegeBrightSunTest.cpp`, `SiegeFogVolumeTest.cpp` **and** every new one I wrote — re-computed against the post-edit files with a faithful re-implementation of the house `CountOccurrencesInCode`. **All 77 green.** That includes the banned-literal sweep (`640/360/260/32000/18000/7000/26000/12000` → 0), the banned-flag sweep, the trace-needle sweep, `FogActiveUntilTimeSeconds =` still `3`, and `SpawnFogVisual`'s `Error`-site count still `3`.
- that the em-dash I added is **byte-identical** (`e2 80 94`) to the one already shipping in the same file, and that the file is strict UTF-8
- `HEAD` = `61702e1`

**CITED, NOT MEASURED BY ME:**
- ⭐ **everything in `TASK-1151`** — the castle-crisp-at-50,000-uu capture, the `MF_Fog` node census, the `VID-007` per-channel RGB. I read the report; I did not re-render anything.
- `TASK-1146`'s `BaseScalability.ini` table and its graphics-menu-reachability chain (I re-read the `[EffectsQuality@0]` section and the `r.VolumetricFog*` family myself; I did **not** re-walk the menu chain).
- the `552 / 0` suite baseline — quoted from the board.
- 🧑 Jonathan's pixels.

**⛔ NOT MEASURED, IN THOSE WORDS:**
- **PIXELS: NOT MEASURED.** I rendered nothing. The duty stays transferred to **`TASK-1149`**, and §0.3 now names *which* capture matters and why.
- **NO WITNESSED RED.** I did not compile and did not run the suite. Every mutation in §7 is a prediction. Duty transferred **by name** to **`TASK-1149`** (`SHIP-§9`).
- **No frame-time measurement.** The floor's cost is three `IConsoleVariable::Set` calls, one `TActorIterator` over `AExponentialHeightFog`, and one `SetVolumetricFog` — **once per fog window**, bounded by the guard, on a function that is not a tick. I did not measure it, and I do not claim a number.

---

# §10 FENCE COMPLIANCE

**Files I wrote — five source files and this handoff, and nothing else:**
- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h`
- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` — **the hint strings + their comment ONLY** (35 lines, all inside that one `SiegeGraphicsMenuText` block)
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGraphicsMenuTest.cpp`
- `.claude/pipeline/handoffs/TASK-1147-programmer.md`
- plus **one `- status:` line** on `TASKBOARD.md`, via the `Edit` tool, single line.

**Untouched, verified:** `SiegeFogStatics.{h,cpp}` · `SiegeGraphicsSettingsSubsystem.{h,cpp}` · `SiegeGraphicsSettingsTest.cpp` · **any `.uasset`** · **any `Config/**`** · `CONVENTIONS.md` (`SC-§82`). `git status` shows **zero** `Content/` entries from me.
**`GFX-§11` / `FOG-§12.6`:** `L_Arena` never opened and never saved — asserted in code (`MarkPackageDirty` / `SavePackage` pinned at **0**). `Content/FogArea/**` **not edited**; `BP_FogArea` **not opened** (the byte scan reads the file, it does not load the asset). `DA_BattlefieldScatter` untouched. `testvideo/**` untouched.
**`FOG-§12.2`'s three forbidden shortcuts:** none taken, and each is now an **executable** ban in the wiring test (`MarkPackageDirty`, `SavePackage`, `GConfig`, `SetQualityLevels`, `ScalabilityQuality` → all 0).
**The fog MECHANIC:** byte-for-byte untouched — `FSiegeFogStatics` pinned at **0** occurrences in `FogVolume.cpp`, no vision-ceiling reference, no deadline write added.
**No compile. No editor. No MCP. No git. No push.**


---
---

# 🔁 LOOP 1 — the gate `TASK-1148` FAILED this row (1 BLOCKER · 6 WARN · 4 NIT). This is the fix.

**Date:** 2026-09-08 · **HEAD at my instant:** `61702e1` (re-derived, unchanged since loop 0) · **Loop 1 of 3.**
**Compile:** ⛔ **NO WITNESSED RED.** I did not compile and did not run the suite. Every mutation below is a **prediction**; the duty stays transferred **BY NAME** to ⭐ `TASK-1149` (`SHIP-§9`).
**Baseline:** the last **measured** suite figure is `552 / 0` at `61702e1` — **quoted, still not re-measured by anyone in this chain.** My loop-0 prediction of `552 → 554` is **unchanged by this loop** (⛔ no test added or removed — the four new assertions went **inside** the existing wiring test, which adds no `IMPLEMENT_*_AUTOMATION_TEST` declaration).

> ### ⚖️ THE GATE WAS RIGHT AND THE FINDING IS ONE I SHOULD HAVE MADE MYSELF.
> I wrote *"idempotent … **exactly like `DestroyFogVisual()`**"* on the release's own declaration, and then called it from **one** of the two sites `DestroyFogVisual()` is called from. The analogy was **stated in my own hand and broken at the one site that would have made it true.** I answered the board's question (*"how many CALL SITES?"*) correctly and never asked the question that decided the defect (*"how many **EXITS**?"*). ⛔ **That is `FOG-§12.5a`'s general form, and the manager has since written it into law:** *a seam that can be entered on one path and left on only some of its exit paths is not idempotent — it is a leak.*

---

## L1.1 ⛔ BLOCKER-1 — **FIXED.** The teardown release, with its `file:line`

| | |
|---|---|
| ⭐ **the new call site** | **`FogVolume.cpp:450`** — `ReleaseFogRenderFloor();` inside `AFogVolume::EndPlay` (`:431-460`), **unconditionally**, **beside** `DestroyFogVisual();` (`:457`) and **before** `Super::EndPlay(EndPlayReason);` (`:459`) |
| the pre-existing call site | `FogVolume.cpp:562` — the reconciler's fog-is-down branch (was `:534`; the line moved only because of the comment block above it) |
| the enforce | `FogVolume.cpp:574` — **still exactly ONE call site**, still inside `RefreshFogVisual()`'s fog-is-up branch (was `:546`) |
| the declaration doc | `FogVolume.h:1169-1215` — the *"nowhere else"* sentence is **struck and replaced** by the two-site rule with the teardown argument written out |
| the argument, in code | `FogVolume.cpp:414-429` — the block above `EndPlay`, which already said *"TEARDOWN is NOT a fourth exit"* and now says **why that is exactly the reason it must release** |

**The ordering is deliberate and matches the down branch:** clear the timer → **release the floor** → destroy the visual → `Super::EndPlay`. The release runs **before** `Super::EndPlay`, so `GetWorld()` is still valid when it looks the height fog up.

### ⛔ WHY IT CANNOT DOUBLE-RELEASE — and the guarantee is STRUCTURAL, not a convention two call sites have to keep

`ReleaseFogRenderFloor()` (`FogVolume.cpp:1093`) is bracketed by the **same one flag** the capture sets:

- **First statement:** `if (!IsFogRenderFloorEngaged(FogRenderFloorPriorState)) { return; }` (`:1099-1105`).
- **Last statement, on BOTH log branches:** `FogRenderFloorPriorState = ReleasedFogRenderFloorState();` (`:1182`).

⇒ the three enumerated exits from FOGGED already reset the flag through the reconciler, so by the time `EndPlay` runs on a fog that lifted normally, the guard is **down** and this call is a **no-op**. A match that never saw fog is a no-op for the same reason. ⭐ **The idempotence is the same fact as the re-entrance guard** (`bEngaged`, `FogVolume.h:75`), which is what makes "release twice" **inexpressible** rather than merely discouraged — the second call cannot reach a single write, an `Unset`, a log line or the component.

⭐ **And the cost of the added call on the common path is one bool**: the guard is read **before** the `TActorIterator`, so an unengaged teardown does not iterate the world.

⚠️ **A property of the release worth stating, because it is what makes this safe in the most hostile teardown ordering:** the **process-wide half has no world dependency**. If `GetWorld()` were already gone, `FindHeightFogComponent` returns `nullptr` (`FogVolume.cpp:858`) and the component restore is skipped — but the three `Unset(ECVF_SetByCode)` calls, which are the entire point, go through `IConsoleManager::Get()` and run regardless. The half that could fail during teardown is the half that is free during teardown.

### ⭐ HOUSE PRECEDENT, MEASURED RATHER THAN CLAIMED — this project already ships this exact shape

`AClimbableTower::EndPlay` (`ClimbableTower.cpp:262`, `TOWER-§10` L-5) calls the climber's **idempotent** exit on teardown for the identical reason: what the actor took hold of (a `MOVE_Flying` climber; here, three process-wide cvars) **outlives the actor**, and teardown never reaches the reconciler that would normally let it go. Its comment even names the belt-and-braces second call and says *"`ReleaseClimber` is idempotent precisely so this second call is free."* ⇒ **the fix is not a new pattern in this codebase; it is the pattern I failed to apply.**

### The reachability I inherited from the gate, restated so this file holds it

`SessionMenuWidget.cpp:123` → `USiegeSessionSubsystem::LeaveMatch()` → `UGameplayStatics::OpenLevel(MenuMapPath, /*bAbsolute*/ true)` ⇒ **absolute travel to `L_MainMenu` with fog still up** ⇒ `EndPlay(LevelTransition)`. Plus **PIE stop** with fog up ⇒ the **editor process** keeps the pin. ⛔ And `L_MainMenu` is the **only** map from which the graphics panel is reachable, so the stranded pin landed exactly where the player acts — while `ShouldEnableVolumetricFog()` reads the **group level**, not the cvar (`SiegeGraphicsSettingsSubsystem.cpp:1166`), so the panel would have printed *"ambient fog OFF"* over a cvar reading `1`. **I did not re-measure `LeaveMatch`'s chain myself this loop — it is CITED from `TASK-1148` §4, which read it.**

---

## L1.2 ⭐⭐⭐ THE NEW MUTATION — **M25**, the one that reddens on a stranded floor

`SC-§83` Addendum B form, **both branches computed** (`SC-§104` cl. 5), rows equal on both branches **struck by name**.

### M25 ⭐⭐⭐ — delete `ReleaseFogRenderFloor();` from `AFogVolume::EndPlay` (`FogVolume.cpp:450`) — **the teardown call-site mutation, and it is the defect this loop fixes**

⇒ **RED: ≥ 3 rows**, including:
1. *"…and exactly **TWO** `ReleaseFogRenderFloor()` calls — the reconciler's down branch AND teardown"* (`SiegeFogVisualTest.cpp:1219-1220`). **BOTH BRANCHES:** fixed `2`, broken `1`. ⛔ Not equal.
2. ⭐ *"**TEARDOWN LETS THE FLOOR GO**: exactly one `ReleaseFogRenderFloor()` in `EndPlay`"* (`:1240-1241`). **BOTH BRANCHES:** fixed `1`, broken `0`. ⛔ Not equal. **This is the discriminating row** — it is the only one that separates M25 from M4.
3. ⭐ *"**THE PAIRING IS THE RULE**: the release has exactly as many call sites as the despawn"* (`:1226-1228`). **BOTH BRANCHES:** fixed `2 == 2` (green); broken `1` vs `2` (red). ⛔ Not equal.

⚠️ **STRUCK FROM THIS PREDICTION, BY NAME (equal on both branches, so they measure nothing about M25):** the whole-file `EnforceFogRenderFloor();` row (`1` both) · both `RefreshBody` rows (`1`/`1` both) · every `BeforeEnforce` ordering row (untouched by the mutation) · the `EndPlay` `DestroyFogVisual();` row (`1` both — it measures the *pairing partner*, not the mutation) · the two `EndPlay` refusal rows (`EnforceFogRenderFloor();` and `RefreshFogVisual();`, `0` both).

⛔ **LIVE COST:** exactly the shipped defect — leaving a match or stopping PIE with fog up strands `r.VolumetricFog` + both grid axes at `ECVF_SetByCode` for the process, silently refusing the Shadows slider at the main menu, with **no `Error` line anywhere** (no release runs, so nothing can report).

### M26 ⭐⭐ — replace the teardown release with `RefreshFogVisual();` (the shape `FOG-§12.5a` explicitly **REFUSES**)

⇒ ~~**RED: ≥ 2 rows** — the *"TEARDOWN LETS THE FLOOR GO"* row (`1` → `0`) and *"…and never calls the reconciler, which would take the fog-is-**UP** branch and re-spawn the box mid-teardown"* (`:1250-1251`, `0` → `1`). **BOTH BRANCHES:** ⛔ not equal on either.~~

🚨 **CORRECTED IN PLACE 2026-09-08 — `TASK-1161`, from `TASK-1148` § LOOP 1 `NIT-6`. STRUCK, NOT DELETED** (the `TASK-1083` strike-in-place precedent). ⛔ **THE DEFECT IS THE FORM, NOT ONLY THE NUMBER:** the struck line names two rows and stops, so it reads as an **exact count**, and `SC-§83` Addendum B requires *"≥ N rows, **including** &lt;named&gt;"* — the shape every other prediction in this set already uses. ⛔ And the number was also low: replacing the teardown release with `RefreshFogVisual();` **REMOVES A RELEASE CALL**, so M25's rows 1 and 3 redden with it ⇒ **≥ 4**.

⇒ **RED: ≥ 4 rows**, including:
1. ⭐ *"**TEARDOWN LETS THE FLOOR GO**: exactly one `ReleaseFogRenderFloor()` in `EndPlay`"* (`SiegeFogVisualTest.cpp:1240-1241`). **BOTH BRANCHES:** fixed `1`, broken `0`. ⛔ Not equal.
2. *"…and never calls the reconciler, which would take the fog-is-**UP** branch and re-spawn the box mid-teardown"* (`:1250-1251`). **BOTH BRANCHES:** fixed `0`, broken `1`. ⛔ Not equal.
3. ⭐ *"…and exactly **TWO** `ReleaseFogRenderFloor()` calls — the reconciler's down branch AND teardown"* (`:1219-1220`) — **M25's row 1, and it is red here for the same reason it is red there.** **BOTH BRANCHES:** fixed `2`, broken `1`. ⛔ Not equal.
4. ⭐ *"**THE PAIRING IS THE RULE**: the release has exactly as many call sites as the despawn"* (`:1226-1228`) — **M25's row 3.** **BOTH BRANCHES:** fixed `2 == 2` (green); broken `1` vs `2` (red). ⛔ Not equal.

⚠️ **STRUCK FROM THIS PREDICTION, BY NAME** (equal on both branches ⇒ they measure nothing about M26): the whole-file `EnforceFogRenderFloor();` row (`1` both, `:1217-1218`) · the `EndPlay` `DestroyFogVisual();` row (`1` both, `:1242-1243`) · the `EndPlay` `EnforceFogRenderFloor();` refusal row (`0` both, `:1248-1249`) · both `RefreshBody` rows (`1`/`1` both, `:1258-1261` — M26 does not touch the reconciler's body) · every `BeforeEnforce` ordering row · the three-exits loop (`:531-548`), which iterates `RaiseFog`/`ApplyBrightSun`/`ResetFog` only.

✅ **THE PREDICTION ITSELF HELD** — the gate derived exactly this and recorded *"RED, WIDER — ≥ 4"* (`qa/TASK-1148.md` § LOOP 1, M26 row), and **wider ⊇ predicted is CONFIRMATION** under Addendum B. ⛔ What failed was the way I wrote it down, which is the whole point of the correction: a bare count invites a reader to treat an unlisted red row as a **surprise** rather than as **confirmation**.
⛔ **THE BOARD'S COPY of this number was already corrected by the manager on `TASK-1147`'s status line; `TASK-1161` did NOT edit that row.**

⛔ **LIVE COST:** a teardown that **re-enforces** the floor and **re-spawns** a fog box into a dying world — the leak with the sign flipped. ⭐ Worth a mutation precisely because it is the *plausible* wrong fix: it looks like reuse of the one reconciler.

### ⚠️ AND ONE EARLIER PREDICTION MOVED — **M4 is now WIDER than loop 0 stated** (Addendum B: wider = confirmation)

**M4** (delete the release from the fog-is-down branch, `FogVolume.cpp:562`) was predicted at *"≥ 2 rows"* and the gate derived **≥ 3**. Post-fix it is **≥ 4**: whole-file `2 → 1` · `RefreshBody` `1 → 0` · `BeforeEnforce` `1 → 0` · **the new pairing row** `2 == 2 → 1 vs 2`. ⛔ **And M4 and M25 are now genuinely distinguishable**, which they were not before: M4 leaves the `EndPlay` row green, M25 leaves the `RefreshBody` rows green. That is the `SHIP-§9` complaint the gate filed — *the old gate was validated against ONE route to the failure* — closed by construction rather than by a promise.

⛔ **M1, M2, M3, M5, M6, M7, M8, M23, M24 are UNCHANGED by this loop.** Nothing in the diff touches the guard, the capture, the `Unset` chain, the grid, the ordering or either hint string. The gate hand-derived all ten and found M2/M3/M4 **wider** than I predicted and M1's ordering-row wording **loose** — see NIT-1 below.

---

## L1.3 ⚖️ `FOG-§12.5` — **THE AMENDMENT LANDED BEFORE I FINISHED. I am not shipping a silent violation.**

✅ **`FOG-§12.5a` is in `CONVENTIONS.md` (`:11027-11035`), added 2026-09-08 by the manager on this blocker, and I read it at my own instant before writing the call.** The pinned table row for the release now reads *"**AMENDED 2026-09-08 … TWO call sites, BOTH REQUIRED — (1) the fog-is-down branch of the same reconciler AND (2) `AFogVolume::EndPlay`, unconditionally, beside `DestroyFogVisual()`. A THIRD site is still a QA FAIL"*, and the enforce row is explicitly marked **UNAMENDED — the ENTRY still has exactly ONE door.**

**Conformance, clause by clause:**

| `FOG-§12.5a` clause | what I shipped |
|---|---|
| `ReleaseFogRenderFloor();` **unconditionally** beside `DestroyFogVisual();` in `EndPlay` | ✅ `FogVolume.cpp:450`, no condition, no new flag |
| ⛔ REFUSED: calling `RefreshFogVisual()` from `EndPlay` | ✅ not done — and **pinned at 0** in the `EndPlay` body (`test:1250-1251`) |
| ⛔ REFUSED: hiding the release **inside** `DestroyFogVisual()` | ✅ not done — the two facts stay two functions; the counts are literal and separately assertable |
| a **THIRD** site is a QA FAIL | ✅ whole-file count asserted at **exactly 2**, and the enforce still at **exactly 1** |
| the wiring test moves **with** the law: whole-file count `2` + a NEW row on the `EndPlay` body | ✅ both, plus the pairing row and the two refusal rows |
| acceptance is the **red under the mutation**, not conformance to the sketch | ✅ **M25** named above, with the discriminating row identified; ⛔ **witnessing is `TASK-1149`'s** |

⇒ **STATUS LINE ANSWER: the amendment landed in time; nothing in this loop is a silent departure from law.**

---

## L1.4 ⚠️ THE SIX WARNs — disposition and reason for **each**

### **WARN-1** — the hint asserts a rendered outcome no capture has demonstrated → ⚖️ **PARTLY ACTIONED. String KEPT; the falsifier is now WRITTEN AT THE STRING.**
**Reason for keeping the string:** it is the best-supported sentence available, it is correctly scoped to **Shadows**, and the gate itself ruled the scoping *"not luck"* — every unrefuted branch-(B) candidate lives in the **Effects** group, so branch (B) cannot falsify a Shadows-scoped claim. Replacing it with silence would restore a menu that says nothing about a coupling the player can feel; replacing it with a hedge invents a doubt the evidence does not support **in the Shadows row**.
**Reason for the action:** the falsifier lived only in a handoff. It now lives at `SiegeGraphicsMenuWidget.cpp:185-203`, in the same comment block as the string, naming (i) that **no frame of this game has ever been captured at Shadows=Low with a Fog card up**, (ii) that `TASK-1149`'s capture is what falsifies it, and (iii) that **the capture requires defeating the floor on purpose** or it proves nothing. ⛔ If that capture shows the wash dying at Shadows=0, **this string is false and must be corrected in the same action** — one false menu sentence replaced by another is this row's own failure mode, and it is now written where the next editor of the string will read it.

### **WARN-2** — the floor **confounds** the measurement that would settle the premise → ✅ **ACTIONED TWICE, and it is L1.6 below.**
Written as a named instruction in this handoff (**L1.6**) *and* in code at the exact line that causes the confound (`FogVolume.cpp:989-1003`, immediately above the `r.VolumetricFog` write). **Reason it belongs in code too:** a handoff is read by the next agent in this chain; the confound will still exist for whoever measures this in a month. ⛔ **I did not attempt the capture: I hold no editor** (fenced), and the row's `PIXELS: NOT MEASURED` stands.

### **WARN-3** — F-6's disclosure is incomplete; it teaches the reader to dismiss the only symptom there is → ✅ **ACTIONED IN BOTH PLACES THE GATE ASKED FOR.**
**Amended F-6 (this handoff supersedes §8's row):** the `LogConsoleManager: Warning … lower priority` line is expected **ONLY while a fog window is up**. ⛔ **WHERE it appears is the whole reading.** The same warning **at the main menu** means a release did not run. ⭐ After this loop there is **no known path to that state** — teardown releases too — so a main-menu occurrence is a **real finding**, and the release's own `Error` line should be beside it. The qualifier is also written **beside the constant** at `FogVolume.cpp:989-1003`, as asked.
⚠️ **And I record the shape, because it is the lesson:** my disclosure was true of the state I had built and false of a state I had not enumerated, and it **pre-emptively explained away the only evidence** of the defect. A disclosure that tells the reader what to ignore must state the conditions under which it applies.

### **WARN-4** — `GFX-§9` / `FOG-§12.1` assert a causal story the evidence does not support → ⛔ **DECLINED — NOT MINE, AND DECLINING IS THE CORRECT ACT.**
`CONVENTIONS.md` is the manager's (`SC-§82`, and `FOG-§12.6` repeats it as a fence on this very batch). I raised it as **F-2** in loop 0 and the gate **upheld** it with verbatim quotes; it is routed. ✅ **STATUS AT MY INSTANT: the manager has been in that file today** — `FOG-§12.5a` landed and the `TASK-1149` board row's commit-message clause has been **struck and rewritten** in my favour — but **I did not verify whether the `GFX-§9:10901`/`:10902` and `FOG-§12.1:10962`/`:10964` corrections themselves have landed**, because reading further into that file to grade the manager's work is not my role. ⛔ **`TASK-1149` must not compose a commit message that disagrees with `CONVENTIONS.md`** (gate §6 cl. 7).

### **WARN-5** — the row's **title** is wider than the row → ⛔ **DECLINED, AND THE REASON IS A FENCE, NOT A DISAGREEMENT.**
I agree with the finding: *"THE SIEGE FOG'S PRESENCE STOPS BEING A GRAPHICS OPTION"* describes a deliverable that did not ship, and a title is a citation (`SC-§97`). ⛔ **But my board fence is my row's single `- status:` line and nothing else** — *"Edit no other line"* — and a title is not that line. Rewriting it would be exactly the second-agent write that fails silently on a lock-free board. ⇒ **routed to the manager/orchestrator**, and the propagation is stopped where I *can* stop it: the gap is declared in §0, on the status line, in this section, and the commit-message constraint (gate §6) is the mechanism that keeps it out of git.

### **WARN-6** — the hint's **first** clause is momentarily false while a fog window is up → ⚖️ **STRING KEPT; THE FALSIFIER IS NOW RECORDED AT THE STRING, AND CROSS-LINKED.**
**Reason for keeping:** `TASK-1146` cl. (3) **measured** that the panel is unreachable during a match, so the clause is true **at every instant the player can read it**. Rewording it to *"except while siege fog is up"* would make the menu describe a state the reader cannot be in — a hedge that costs clarity today to buy accuracy for a build that does not exist.
**Reason for the action:** the falsifier was recorded in `FogVolume.cpp`'s banner and **not** at the string it falsifies. It is now at `SiegeGraphicsMenuWidget.cpp:196-203`, and the scope-deletion comment in `FogVolume.cpp:832-841` now **names it back**, so whoever ships an in-match Settings entry meets the obligation from either end. ⭐ The suggested replacement wording is written out, so the future edit is a transcription rather than a re-derivation.

---

## L1.5 📝 THE FOUR NITs — disposition and reason for **each**

### **NIT-1** — §7 M1's parenthetical: the four ordering rows are **skipped**, not failed → ✅ **CORRECTION ACCEPTED AND MADE HERE.**
The gate is right and its mechanism is exactly right: `SubstringBefore` `AddError`s on a missing marker and the block is **skipped**, so M1's actual failure is **one `AddError` plus the two named `TestEqual`s**, not two + four. ⛔ **The prediction still holds under Addendum B** (both *named* rows red); only the parenthetical over-counted. ⭐ **Corrected wording for M1:** *"≥ 2 rows, including 'THE FLOOR IS CALLED' and 'the enforce lives in the ONE reconciler'; **plus one `AddError` from the stale ordering marker, whose four rows are then SKIPPED rather than failed**."* I am recording it rather than editing §7 in place, so the gate's finding and my correction both stay legible.

### **NIT-2** — *"no in-match pause or escape menu anywhere in `Source/`"* is not exact → ✅ **ACTIONED** (`FogVolume.cpp:832-841`).
The comment now rests on the **two load-bearing facts** the gate confirmed still hold (`USettingsMenuWidget::CreateAndAddToViewport` has **zero** call sites; the panel lives on a **different map**) and **names `USessionMenuWidget`** as the in-match menu that exists but carries no Settings entry. ⭐ **It is worth more than tidiness:** that widget's Back button is the absolute travel that made teardown a route at all, so the comment now points at `EndPlay` from the place my loop-0 sentence had implied nothing was there. The scope deletion itself **stands** — the gate re-checked both facts.

### **NIT-3** — *"once per fog window"* bounds the writes and the log, **not** the observation read → ✅ **ACTIONED** (`FogVolume.cpp:957-962`).
The comment now says exactly which: the `TActorIterator` + three `FindConsoleVariable` run on **every** call including re-entries, because the guard sits **below** them **on purpose** — a short-circuit above would make the guard dead code (`SC-§36.1`), which is the shape this file exists to refuse. The cost is named (a handful of iterations per match, on a non-ticking actor) and the trade is declared deliberate. ⛔ **No number is claimed:** I did not profile it.

### **NIT-4** — date-stamp the `VolumetricFogDistance = 6000` transcription → ✅ **ACTIONED** (`FogVolume.cpp:1024-1027`), as a **comment**, not inside the `Error` string.
It records that `TASK-1151` re-measured `6000` uu **live on 2026-09-08**, so the transcription the error text calls stale is **currently accurate and the falsifier has not fired**, and it tells the next reader to **re-measure rather than re-derive it from the comment**. ⛔ Kept out of the log string deliberately: a date inside a runtime message ages into a lie the moment nobody updates it, and the string is asserted by count elsewhere.

---

## L1.6 🚨🚨 **THE MEASUREMENT-DEFEAT INSTRUCTION — for whoever takes the §0.3 capture (`TASK-1149`, or a later row)**

> ### ⛔ **MY FIX DESTROYS THE EVIDENCE UNLESS YOU DEFEAT IT ON PURPOSE. READ THIS BEFORE YOU CAPTURE ANYTHING.**

**The confound, stated mechanically.** With the floor live, `EnforceFogRenderFloor()` writes `r.VolumetricFog = 1` at **`ECVF_SetByCode` (`0x0E000000`)**. `FConsoleVariableBase::CanChange` is `NewPri >= OldPri` (`ConsoleManager.cpp:275-280`), and scalability writes at **`ECVF_SetByScalability` (`0x02000000`)**. ⇒ **while a fog window is up, `sg.ShadowQuality 0` CANNOT drive `r.VolumetricFog` to `0`.** A capture taken that way shows the wash **surviving at Shadows=0 whether or not the exploit exists** — a false *"branch (A) — no exploit"* that **looks exactly like evidence**.

**The instruction, mechanically, in order:**

1. **Record the `git HEAD` you ran against, and whether the floor is LIVE in that binary.** A capture on a pre-`61702e1`+floor build answers a different question than one on a post-floor build, and the two are indistinguishable afterwards.
2. Raise the fog in PIE at `sg.ShadowQuality 3` / `sg.EffectsQuality 3`. Capture the hero vantage.
3. Set `sg.ShadowQuality 0`. ⛔ **THEN READ `r.VolumetricFog` BACK** (`SC-§94` cl. B — do **not** assume the write landed).
4. ⛔ **IF IT READS `1`, THE FLOOR REFUSED THE WRITE AND YOUR MEASUREMENT IS DEAD UNTIL YOU DEFEAT IT.** Type **`r.VolumetricFog 0`** at the console: **`ECVF_SetByConsole` (`0x10000000`) OUTRANKS `ECVF_SetByCode`** (`IConsoleManager.h:183`, `:187`), so a console write wins where scalability loses. **Re-read until it reads `0`.** Capture.
   - **Alternative:** run with `bEnforceFogRenderFloor = false` on the `AFogVolume` CDO (`FogVolume.h:932`). ⛔ **Weaker**, because it changes the binary's behaviour rather than overriding one variable, and it cannot un-pin a variable an **earlier** session already pinned.
5. Then `sg.EffectsQuality 0` and capture again.
6. ⛔ **SAY IN THE HANDOFF WHICH METHOD YOU USED, AND QUOTE THE ACHIEVED CVAR VALUE AT EVERY STEP.** *"I set Shadows to Low"* is the **request**, not the measurement — which is the same `SC-§94` failure the whole fog lane has now hit three times.
7. ⚠️ **A fog number needs a CONVERGENCE SERIES, never a single frame.** `TASK-841` §3.2 measured **55.8 %** vs **100.8 %** obscuration at *identical* settings. Hold the camera still and let the temporal accumulation settle.
8. ⭐ **Run in a FRESH editor process.** Before this loop, a stranded pin from an earlier PIE session survived into the next one — and in that state **even `bEnforceFogRenderFloor = false` could not un-pin it**, because nothing would call the release. ⛔ That specific trap is closed by this fix, but a session started **before** it shipped can still be holding one.

**What each outcome means (unchanged from §0.3, restated so this section stands alone):** wash survives all three ⇒ **branch (A)**, no exploit, and the hint-string correction is the whole fix · wash dies at **Shadows=0** ⇒ the floor is exactly right and should be verified to restore it · wash dies at **Effects=0** ⇒ **branch (B)**, the route is in the Effects group, and a **new row** is owed — ⛔ not a widening of this one.

---

## L1.7 WHAT CHANGED IN THIS LOOP — files, lines, and what QA should scrutinise

**Files written (four source files + this handoff + one board `- status:` line, and nothing else):**

| file | what changed |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | ⭐ **the fix**: `ReleaseFogRenderFloor();` at `:450` in `EndPlay` + the teardown argument at `:414-429` and `:441-449` · WARN-2 + WARN-3 at `:989-1003` · NIT-2 at `:832-841` · NIT-3 at `:957-962` · NIT-4 at `:1024-1027` |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | the release's declaration doc `:1169-1215` — *"nowhere else"* replaced by the **two-site** rule with both sites and the reason for each |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` | wiring test: whole-file release count `1 → 2` (`:1219`) · **new** pairing row (`:1226`) · **new** `EndPlay` block with four rows (`:1230-1252`) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` | **comment only** — the two falsifiers of the hint string recorded at the string (`:185-203`). ⛔ **Neither string changed by one byte.** |

**⛔ NOT touched this loop:** `SiegeGraphicsMenuTest.cpp` (no assertion needed to move) · `SiegeFogStatics.{h,cpp}` · `SiegeGraphicsSettingsSubsystem.{h,cpp}` · `SiegeGraphicsSettingsTest.cpp` · **any `.uasset`** · **any `Config/**`** · `CONVENTIONS.md` (`SC-§82`) · `L_Arena` (never opened — `MarkPackageDirty`/`SavePackage` still pinned at **0**) · `Content/FogArea/**` · `BP_FogArea` · `testvideo/**`. **No editor. No MCP. No git. No push. No compile.**

### 🔍 WHAT I ASK THE GATE TO SCRUTINISE — the four places this loop could be wrong

1. ⛔ **The exit census, again, and this time as a question about EXITS rather than names.** I claim `RefreshFogVisual()`'s down branch and `EndPlay` are **all** the ways this actor stops holding the floor. The pairing row (`release count == DestroyFogVisual count`) is my structural defence, but it inherits `DestroyFogVisual()`'s own census. ⛔ **If `DestroyFogVisual()` is itself missing an exit, my pairing row is green and both leaks are live.** I did not re-audit that.
2. ⛔ **Teardown-time safety of the component half.** The release calls `FindHeightFogComponent(GetWorld())` during `EndPlay`. I argue it is safe (`TActorIterator` skips pending-kill; a null world returns `nullptr`; the cvar half needs no world) and that the worst case is a **skipped** restore on a dying world, which is free. ⛔ **That is reasoning, not an executed teardown** — I hold no editor.
3. ⛔ **`EndPlay(RemovedFromWorld)` / streaming.** I treat every `EndPlayReason` identically and release unconditionally, per `FOG-§12.5a`. If an `AFogVolume` were ever streamed **out and back in** mid-match with fog up, the floor would be released and then **re-taken** by the next `RefreshFogVisual()` — correct, but it costs one extra capture/release cycle and one extra log line. ⛔ **No level streaming exists on `L_Arena` today** and I did not build for it.
4. ⛔ **`TestEqual` with two computed counts** (the pairing row) is the one assertion in this diff whose *expected* side is not a literal. It is deliberate — a literal `2` there would be a second place to update — but it means the row **cannot** detect a change that moves both counts together. Its discriminating partner is the `EndPlay`-body row, which **is** a literal.

**Suite delta this loop:** ⛔ **zero new `IMPLEMENT_*_AUTOMATION_TEST` declarations** — four assertions inside the existing wiring test. ⇒ the loop-0 prediction of `552 → 554` is **unchanged**, and `TASK-1149` still reconciles against what the **last host actually measured** (`SC-§95` cl. 2), never against a published absolute. ⛔ **NO WITNESSED RED.**
