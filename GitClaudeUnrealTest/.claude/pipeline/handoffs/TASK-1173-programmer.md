# TASK-1173 — [FOGTRIGGER-REACHABLE] — gameplay-programmer handoff

**Gate:** `TASK-1174` (qa-reviewer) · **Host:** `TASK-1175` (build-master)
**Base:** `74baab3`. **Files:** `Source/GitClaudeUnrealTest/Siegebound/FogVolume.{h,cpp}` · `Siegebound/Tests/SiegeFogVisualTest.cpp`
**Diff:** `+298 / +75 / +313` = **686 insertions, 0 deletions.** No file outside the three named. No `Content/**`.

---

## 0. THE ONE SENTENCE THIS HANDOFF MUST NOT BURY

🚨 **`PATH NEVER EXECUTED`.**

Written in those words because `SC-§113` cl. 3(b) requires it and because this row **forbids me to compile or run the suite** (`SC-§27`, spec cl. 6). I have shipped three triggers, and I have executed **none** of them. Everything in §4 below is an **expected** log line derived by reading the shipped `TEXT(...)` format strings — it is **`NOT MEASURED`**, and no reader may treat it as a pass.

⚠️ **This is a structural contradiction inside the row itself, and I am naming it rather than papering over it:** cl. (2) demands *"a QUOTED LOG LINE showing `AFogVolume` EXECUTING"*, and cl. (6) forbids the compile that would produce one. **The obligation is real and it is now `TASK-1175`'s** — §4 gives it the exact command, the exact expected text, and the exact grep. If `TASK-1175` commits without running that grep, this row has shipped `SC-§36.1` again and the gate should say so.

---

## 1. THE THREE DELIVERABLES, AND HOW AN AGENT REACHES EACH

### (a) THE AUTOMATION TEST — ⭐ the only lane that makes the fog path **gate-able forever**

`Siegebound.Fog.RaiseFogIsActuallyExecutedInARealWorldAndTheVisualAppearsThenGoes`
(`Tests/SiegeFogVisualTest.cpp`, test 10; the file's test count goes **9 → 10**, and the suite total **554 → 555**.)

It builds a **real, playing `UWorld`** and calls the **real entry points** — `AFogVolume::FindOrSpawn(World)` then `Volume->RaiseFog()`, which is character-for-character what `USpellLibrary::ResolveSpell`'s `FogCover` arm calls — then `ResetFog()`. Nothing is re-implemented.

**Reached from an agent's seat by the runner this project already uses, unchanged:**

```
UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<log>
```

⭐⭐ **THIS IS THE PART THAT ANSWERS `SC-§113` cl. 4.** The law says the trigger must land on *"a channel we have MEASURED OURSELVES USING"*. The suite is that channel: it is the **only instrument in this pipeline proven to run gameplay code**, it has executed **554** tests as recently as `74baab3`, and it needs **no new tooling**. The build-master types the same command as always and the fog path runs.

🚨 **THE FIRST `UWorld::CreateWorld` AND THE FIRST `SpawnActor` IN `Siegebound/Tests/`.** That is a deliberate precedent break, and it makes **nine existing test-file headers stale** — they each assert *"every automation test in this project is HEADLESS; there is not one `UWorld::CreateWorld` and not one `SpawnActor` anywhere in `Siegebound/Tests/`"* (`SiegeAcquisitionFunnelTest` · `SiegeBrightSunTest` · `SiegeFogClampTest` · `SiegeFogReachSeamTest` · `SiegeFogVolumeTest` · `SiegeHeroCameraTest` · `SiegeHeroLadderClimbTest` · `SiegeInvisibilityTest` · `SiegeLadderClimbTest` · `SiegeRecallTest` · `SiegeRespawnLifecycleTest` · `SiegeUnitNoticeRangeTest` · `SiegeWarMapTest`, plus `HeroCharacter.h` and `SiegeLadderClimbStatics.h`). ⛔ **I did not sweep them** — they are prose in files this row does not own, a fifteen-file sweep inside a capability row turns it into a refactor, and each sentence is really about *its own* reasoning. **The divergence is DECLARED here (`SC-§91`) and the fixture's own doc comment says it too**, so a reader who trusted one of those sentences lands on the correction. **This is a candidate row for the manager, not a defect I hid.**

The world is created inside `GetTransientPackage()` — load-bearing, not tidiness: it is what guarantees this test can **never** dirty or save a map (`GFX-§11`).

### (b) THE CONSOLE COMMANDS — ⭐ the lane an agent can pull *on demand*, without the suite

Three, registered in `FogVolume.cpp` behind `#if !UE_BUILD_SHIPPING`:

| command | what it calls | writes? |
|---|---|---|
| `Siege.Fog.Raise` | `AFogVolume::FindOrSpawn(World)` → `RaiseFog()` — **the card's own two doors, in order** | yes |
| `Siege.Fog.Clear` | `AFogVolume::Find(World)` → `ResetFog()` — the match-reset door | yes |
| `Siege.Fog.Status` | `IsFogActive` / `IsFogPrevented` / `GetFogPreventionSecondsRemaining` | **no — read-only** |

**Reached from an agent's seat by substituting the command list in the runner we already own:**

```
~~ -ExecCmds="Siege.Fog.Raise;Siege.Fog.Status;Quit" ~~     ⛔ STRUCK — RUNS NOTHING, SILENTLY. SEE BELOW.

UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<log>
```

🚨🚨 **CORRECTED 2026-09-09 BY THE MANAGER (W9) — struck, not overwritten (`SC-§53` cl. 3).** The
semicolon form above was **measured by `TASK-1175`** taking the whole string as **one command name**:
no fog line, **no `Command not recognized`**, no warning, and the process **never exited**. Two
independent defects: `-ExecCmds` splits on **comma**, never `;` (`ParseExecCommands.cpp:29`), and
`Quit` does not quit an **editor** commandlet — that is **`QUIT_EDITOR`** (`EditorServer.cpp:5993`).
✅ The comma + `QUIT_EDITOR` form is **measured working and self-terminates in 12 s.**

⛔ **This does NOT generalise backwards:** the *suite* line `Automation RunTests Siegebound;Quit` is
**correct** — that `;` is split by the `Automation` handler's own parser and never reaches
`-ExecCmds`. **Do not "fix" it.** Pinned recipe for both lanes: `CONVENTIONS.md` → **`SC-§116`**.

⚠️ **`-ExecCmds` MUST ALSO BE ONE POWERSHELL ARGUMENT.** `TASK-1104`'s first two attempts ran **zero** tests because PowerShell split the value on its spaces and the engine idled to the boot bound with **no error and no warning** (`SC-§95` cl. 1). The commands above are deliberately **space-free** for exactly that reason — that is a **third** way to run nothing silently on this one flag.

⚠️ **The world a commandlet gives these is an EDITOR world.** `Siege.Fog.Raise` therefore spawns the state actor into whatever map is open and **marks it dirty**. The command **acts anyway and warns loudly** — refusing would kill the one headless channel we have measured ourselves using, which is the defect this row repairs. **`GFX-§11` still binds the operator: never save.** The visual itself already carries `RF_Transient`, so it cannot be baked in even if somebody did.

### (c) THE `CallInEditor` BUTTONS — 🧑 his lane, and only his

`DevRaiseFog` · `DevClearFog` · `DevLogFogState` on `AFogVolume`, `#if WITH_EDITOR`, **`CallInEditor` only — deliberately not `BlueprintCallable`.**

**⛔ NO AGENT CAN PRESS THESE, and I am not counting them as an agent lane.** The editor bridge has no click. They exist because 🧑 Jonathan can select the fog volume in the outliner **during a live match** and press a button with no console and no typing.

⚠️ **Their limitation, stated so it is never filed as a bug:** a button lives on an **instance**. Before the first cast of a match there is no `AFogVolume` to select, so they can refresh/clear an existing fog but **cannot create one**. To raise fog from nothing: `Siege.Fog.Raise` (it goes through `FindOrSpawn`) or play the card.

---

## 2. `SC-§111` cl. 3(a) — EVERY NEW ENGINE SYMBOL, DECLARATION OPENED, GUARD STATED

Measured by tallying preprocessor directives from line 1 of each installed UE 5.8 header — **not** relayed (`SC-§40`).

**Production code (`FogVolume.cpp`, `Runtime` module — this is the column that matters):**

| symbol | header : line | `WITH_EDITOR`-guarded? |
|---|---|---|
| `FAutoConsoleCommandWithWorldAndArgs` | `Core/Public/HAL/IConsoleManager.h:2411` | **NO** — depth 1, `#if !NO_CVARS`; a variadic-constructor **stub exists in the `#else` at `:2493`** ⇒ compiles in **both** branches |
| `FConsoleCommandWithWorldAndArgsDelegate` | `IConsoleManager.h:257` (`DECLARE_DELEGATE_TwoParams`), null twin `:387` | **NO** |
| `ECVF_Cheat` / `ECVF_Default` | `IConsoleManager.h:76` | **NO** |
| `UWorld::IsGameWorld` | `Engine/Classes/Engine/World.h:4172` | **NO** — depth 0 |
| `UWorld::GetName` (`UObject::GetName`) | `CoreUObject/Public/UObject/UObjectBaseUtility.h` | **NO** |

⇒ ✅ **ZERO editor-only engine symbols on the (a) or (b) lane.** That is the exact defect `TASK-1166` BLOCKER-1 caught on `TASK-1165` in this same file this same week (`AActor::RerunConstructionScripts`, `Actor.h:3415-3418`, editor-only), and it is **not repeated**.

⚠️ **COUNT, STATED EXACTLY BECAUSE THIS PROJECT PUNISHES A LOOSE NUMBER:** the two tables are **5 production symbols + 17 test symbols = 22 total, 0 editor-guarded.** My first Slack post and the board status line both said *"17 symbols"* — that was the **test table alone**, i.e. an **undercount of the production column**, which is the column that matters. The board line is corrected; **the Slack post stands as posted and is corrected here** (`SC-§53` cl. 3 — a struck number is named, not quietly overwritten).

**Test code (`Tests/SiegeFogVisualTest.cpp`, already wholly inside `#if WITH_DEV_AUTOMATION_TESTS`):**

| symbol | header : line | guarded? |
|---|---|---|
| `UWorld::CreateWorld` | `World.h:3375` | **NO** — depth 0, `public:` @ 3334 |
| `UWorld::DestroyWorld` | `World.h:3380` | **NO** |
| `UWorld::InitializeActorsForPlay` | `World.h:3958` | **NO** — `public:` @ 3700 |
| `UWorld::BeginPlay` | `World.h:3971` | **NO** |
| `UWorld::AreActorsInitialized` | `World.h:2821` | **NO** |
| `UWorld::SetPhysicsScene` | `World.h:2894` | **NO** |
| `UEngine::CreateNewWorldContext` | `Engine.h:3626` | **NO** |
| `UEngine::DestroyWorldContext` | `Engine.h:3628` | **NO** |
| `UEngine::ShutdownWorldNetDriver` | `Engine.h:3401` | **NO** |
| `FWorldContext::SetCurrentWorld` | `Engine.h:473` | **NO** |
| `AActor::RouteEndPlay` | `GameFramework/Actor.h:3381` | **NO** |
| `FSoftClassPath::ResolveClass` | `UObject/SoftObjectPath.h:577` | **NO** (`[[nodiscard]]`, return is used) |
| `MakeUniqueObjectName` (4-arg) | `UObject/UObjectGlobals.h:1061` | **NO** |
| `EUniqueObjectNameOptions` | `UObjectGlobals.h:1025` | **NO** |
| `FActorRange` / `TActorIterator` | `Engine/Public/EngineUtils.h:542` | **NO** |
| `FURL` | `Engine/Classes/Engine/EngineBaseTypes.h:840` | **NO** |
| `AExponentialHeightFog` | `Engine/Classes/Engine/ExponentialHeightFog.h` | **NO** |

### The two `#if` guards I *did* write, and why neither is the forbidden case

`SC-§111` cl. 3(c) warns that a guard **is the defect** where the guarded code is *the mechanism*. Neither of mine is:

1. **`#if !UE_BUILD_SHIPPING`** around the console commands — the shipping fence itself. Divergence runs in the **safe** direction: the shipped game loses a **developer trigger** and **no gameplay mechanism**. This is the house *"non-shipping by construction"* rule from `USiegeCheatManager`, **not a second pattern**.
2. **`#if WITH_EDITOR`** around the three `CallInEditor` functions — **symmetric on declaration and definition**, guarding **our own** affordance. The row itself says a `CallInEditor` button being editor-only is *"FINE and EXPECTED"*.

⛔ **Nothing in the fog's mechanism is guarded anywhere.** `RaiseFog`, `RefreshFogVisual`, `SpawnFogVisual`, `EnforceFogRenderFloor` are byte-unchanged.

**Two fences, not one, because each covers a configuration the other misses:** `!UE_BUILD_SHIPPING` removes the registration in **Shipping**; `ECVF_Cheat` makes `IConsoleObject::IsEnabled()` refuse it wherever `DISABLE_CHEAT_CVARS` is set — `UE_BUILD_SHIPPING || (UE_BUILD_TEST && !ALLOW_CHEAT_CVARS_IN_TEST)` (`Misc/Build.h:440`) — which **also covers TEST**, which the first does not.

🚨 **`COOKED TARGET NOT COMPILED`** (`SC-§111` cl. 3(d)). I compiled nothing at all.

---

## 3. SCOPE FENCE + HELD BYTES — BOTH ATTESTED, BOTH MEASURED

**Scope (spec cl. 4):** ✅ **ZERO behavioural change.** No colour, no density, no cvar, no material, **no edit inside `ApplyFogVisualMaterial`** (which does not exist at this base — it went with the revert). `RaiseFog`, `ApplyBrightSun`, `ResetFog`, `RefreshFogVisual`, `SpawnFogVisual`, `DestroyFogVisual`, `EnforceFogRenderFloor`, `ReleaseFogRenderFloor`, `EndPlay`, `Find`, `FindOrSpawn`, `FogVisualTransform`, `FogVisualClassPath`, `FogVisualScaleMatches` are **byte-unchanged**. The diff is **686 insertions and 0 deletions** — arithmetic proof that nothing existing was rewritten.

**Held bytes (spec cl. 4):** ✅ **I did NOT stack on `TASK-1165`'s held diff.** Measured, not assumed:

```
git rev-parse --show-toplevel  ->  C:/GitProjects/GitHub/GitClaudeUnrealTesting   (SC-§102: git root is ONE LEVEL UP)
git rev-parse --short HEAD     ->  74baab3
git diff --stat 20c1bea HEAD -- <the three files>   ->  EMPTY
```

⇒ my base is the **reverted** state, `TASK-1175` did its job, and `TASK-1174` cl. 4's *"which lines belong to which row"* answer is: **all 686 added lines are mine; every pre-existing line is `20c1bea`'s.**

---

## 4. 🚨 THE LOG LINES — EXPECTED TEXT, EXACT COMMAND, EXACT GREP

**Verbosity: DEFAULT. Nothing needs enabling.** `GitClaudeUnrealTest.h` declares `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)` ⇒ runtime default `Log`, compile-time max `All`, so every line below prints with **no `-LogCmds`** and **no `Log LogGitClaudeUnrealTest Verbose`**. ⭐ This is deliberately unlike the climb instrument, which printed nothing without an explicit `Verbose` and whose **empty log read as a zero offset — a false pass**.

**Run either lane, then run the grep. The grep is the discharge, not the lines.**

```
~~ -ExecCmds="Siege.Fog.Raise;Siege.Fog.Status;Quit" ~~     ⛔ STRUCK 2026-09-09 (W9) — RUNS NOTHING, SILENTLY. SEE §1(b).

UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<log>

grep -cE "AFogVolume|Siege\.Fog\." <log>          ⛔ CONFOUNDED — DO NOT USE BARE. SEE BELOW.
grep "LogGitClaudeUnrealTest:" <log> | grep -cE "AFogVolume|Siege\.Fog\."     ✅ THE DISCRIMINATOR
```

🚨 **CORRECTED 2026-09-09 BY THE MANAGER (W8) — the bare count is a confounded instrument.** On the
suite log it returned **8**, and **7 of those 8** are a **pre-existing test name** containing the
substring `AFogVolume` (`TheFogStateIsReadInExactlyOnePlaceAndTheSeamConsultsAFogVolume`, measured
present at `20c1bea`). ⇒ **a bare non-zero would have read as proof of execution while proving
nothing.** Only the `LogGitClaudeUnrealTest:`-filtered form (which returned **1**, from production
code) discriminates. Law: `SC-§113` cl. 6.

⭐⭐ **`SC-§113` cl. 2 recorded that this grep returned `0` over the entire editor log.** A **non-zero** count is the positive control that ends the law's instance. **`0` after this diff means the trigger did not fire — report it as a BLOCKER, never as "clean".**

Expected, in order (each prefixed `LogGitClaudeUnrealTest: ` in the file):

**From the shipped path (these are the ones that matter — they are *production* code executing):**

```
[FogVolume_0] AFogVolume spawned — the fog-state actor now exists for this match (FOG-§10.1: one state object).
[FogVolume_0] Fog INTEGRITY FLOOR engaged — ACHIEVED 'r.VolumetricFog'=1, 'r.VolumetricFog.GridPixelSize'=16, 'r.VolumetricFog.GridSizeZ'=64, height fog volumetric=ON (view distance 6000), ...
[FogVolume_0] Fog VISUAL spawned: 'BP_SiegeFog_C_0' — ACHIEVED Z=7000, ACHIEVED scale (640, 360, 260), read back from the actor with GetActorLocation/GetActorScale3D (requested (640, 360, 260), derived from ArenaHalfExtent + a 6000 uu margin; ...
[FogVolume_0] Fog raised for 300 s (refresh, never stack — J-F16); it lifts at world time <now+300>.
```

**From the new trigger:**

```
[Siege.Fog.Raise] AFogVolume::RaiseFog() executed on 'FogVolume_0' and returned TRUE. Fog is now UP. ⛔ This line is the POSITIVE CONTROL for `SC-§113` cl. 3(c): its presence proves the fog path RAN. ...
[Siege.Fog.Status] 'FogVolume_0' — fog UP, prevention DOWN (0 s of prevention remain). Read from the shipped accessors ...
```

**On the way down (`Siege.Fog.Clear`, or the suite's `ResetFog()`):**

```
[FogVolume_0] Fog VISUAL destroyed ('BP_SiegeFog_C_0') — the fog has lifted, been burned off by BrightSun, or the match was reset. IsFogActive() is the one predicate that decided this.
```

⚠️ **Actor instance names (`FogVolume_0`, `BP_SiegeFog_C_0`) and the world time are the ONLY parts I am guessing.** Every other character is transcribed from the shipped `TEXT(...)` literal. ⚠️ **The `640/360/260` and `Z=7000` in the visual line are the *intended* values; if the log shows `(20, 20, 5)` instead, `TASK-1071` has returned** and the `Error` line above it will say so.

---

## 5. 🚨 `FOG-§12.5c` cl. 5 — WHAT A CALLER MUST DO TO TAKE THE DISCRIMINATING SERIES

**The question, verbatim:** *is the ~181 s cycle present when the fog arrives by the CARD rather than by a hand-spawned probe?* If it is a rig artefact, the acceptance window is averaging the rig and **the pin is void**.

**This row does not answer it — it makes it answerable.** The recipe, so nobody has to re-derive it:

1. **PIE on `L_Arena`.** ⛔ Not the editor world, not a commandlet: the series is a *pixel* series and it must come from a real match, which is the whole point of the discriminator.
2. **Raise by the card path.** Play the `Fog` card, **or** type `Siege.Fog.Raise` in the PIE console — it goes through `FindOrSpawn` → `RaiseFog`, the identical two doors. **Say in the report which one you used** (`FOG-§12.5b` actuation axis).
3. ⭐⭐ **CAPTURE THE `Fog VISUAL spawned` LINE BEFORE YOU CAPTURE ONE PIXEL.** This is the cheap control the probe rig never had: it prints **ACHIEVED scale and ACHIEVED Z read off the actor**. If the card-raised fog's achieved geometry differs from the probe's, that difference **alone** could produce a different series, and it costs one grep to find out. ⛔ Named as a **lead**, not a conclusion (`SC-§109`).
4. **Hold the fog up for the whole window.** `FogDurationSeconds` is `300 s` and `WIN=cycle` needs ≥ 180 continuous seconds with **values frozen throughout**. For a ≥ 2-cycle span, re-issue `Siege.Fog.Raise` every ~120 s: `J-F16` makes it a **refresh, never a stack**, so the deadline resets. ⭐ **A refresh does NOT re-spawn the visual** — `RefreshFogVisual` only spawns `if (!IsValid(FogVisualActor.Get()))` — so **the visual's own age is not reset and the series has no discontinuity at the refresh.** That property is what makes an arbitrarily long series possible; verify it by confirming **no second `Fog VISUAL spawned` line** appears.
5. **Same vantage, same bands, same thresholds — nothing moves** (`FOG-§12.5`, `FOG-§12.5c` cl. 1-2): far-field `y ∈ [0.42, 0.48]`, `x ∈ [0.30, 0.70]` is the **gate**; optically-thick `y ∈ [0.05, 0.25]` is a **mandatory companion**. `WIN=cycle` = ≥ 180 continuous seconds, ≥ 60 evenly-spaced frames. Report **mean, min, max, fraction-of-frames-passing** — four numbers, and a mean without its range is refused.
6. **The comparison that decides it:** against the probe-rig series `B/G 0.9235 → 0.9857`, range `0.0622`, ~181 s. **Cycle present at comparable amplitude ⇒ the pin stands. Flat, like the no-fog control (`0.5774 → 0.5777`) ⇒ `FOG-§12.5c` is VOID and the manager must be told.**
7. **Keep the cross-instrument control:** on the **thick** band, 🧑 his `VID-007` gameplay recording (`0.9153`) and the probe rig (`0.9145`) agree to `0.0008`. **A row reporting only the far field has thrown away its only link to his actual screen.**
8. ⚠️ **The integrity floor defeats one measurement and not this one** — but say which you did: with the floor held, `r.VolumetricFog` is pinned at `ECVF_SetByCode`, so a *scalability-exploit* capture is invalid. Defeat it deliberately (`r.VolumetricFog 0` from the console outranks `SetByCode`) **only** if that is what you are measuring.

---

## 6. WHAT `TASK-1174` SHOULD SCRUTINISE — INCLUDING THE THINGS THAT COULD SINK ME

I would rather hand these over than have them found.

1. 🚨 **THE BIGGEST RISK: the automation test creates a world, and no test in this project ever has.** If `UWorld::CreateWorld` / `InitializeActorsForPlay` / `BeginPlay` misbehave under `-nullrhi` in the commandlet, **test 10 reds and takes the wave with it.** Mitigations I took: the shape is copied step-for-step from the engine's own `Developer/CQTest/Private/Components/ActorTestSpawner.cpp` rather than invented; and I **measured** that this project has **zero `UWorldSubsystem`s** (all six are `UGameInstanceSubsystem`s and this world has no game instance), so `UWorld::BeginPlay` runs **no project code** and is null-safe on the absent game mode. ⛔ **I could not run it.**
2. 🚨 **A red in test 10 may be a REAL FINDING, not a regression I introduced.** The automation framework routes every `UE_LOG(..., Error, ...)` raised during a test into `AddError`. `FogVolume.cpp` has **seven** `Error` sites and **none has ever been executed by anything**. So the first run of this test is also the first time those branches can fire. ⭐ **That is the row working, not the row failing** — but it means a red must be **read**, not reverted. ⛔ **Do not "fix" it with `AddExpectedError`:** that restores the exact silence `SC-§113` was written about.
3. ⚠️ **I spawn an `AExponentialHeightFog` into the test world.** `EnforceFogRenderFloor` logs an `Error` when there is none, because in a real match its absence means terms 5-7 of `ShouldRenderVolumetricFog` are unsatisfiable. A bare world would therefore red on a property **of the rig** (`SC-§112`'s shape). `L_Arena` has one; spawning one makes the world resemble the shipping world. **It does not suppress the check** — a floor that failed for any other reason still reds. **Judge whether you agree.**
4. ⚠️ **The test LOADS `/Game/Blueprints/BP_SiegeFog` and, through it, the read-only vendor pack.** `TASK-841` §5.3 measured that loading a vendor package **dirties** it (`FOG-§6`). Nothing here saves, and the sanctioned runner never saves ⇒ safe in the lane we use. 🚨 **It is NOT safe to run this from the Session Frontend in a live editor and then press Save All.** The world itself is in `GetTransientPackage()`, so **no map is touched**; the hazard is the vendor chain only. **Disclosed, not dodged.** ⇒ this is also the instrument that finally makes `TASK-1166` **WARN-10** ("does a fog raise dirty a package?") measurable — I did **not** discharge it (the row says make it possible, not discharge it); a follow-up snapshots `UPackage::IsDirty()` around step (3).
5. ⚠️ **I did NOT assert the fog visual's scale myself, on purpose.** `SpawnFogVisual` already tests exactly that and raises an `Error`, which this test's capture turns into a failure. A second predicate here would be a **paraphrase** of the shipped one that could drift, and if the two ever disagreed nobody could say which was right. The numbers are surfaced via `AddInfo`; **the judgement stays where the code makes it.** If you think that is too soft, say so — it is a one-line change.
6. ⚠️ **The `Siege.Fog.Raise` editor-world case acts and warns rather than refusing.** I argue refusing would kill the one headless agent channel and re-create this row's defect. **This is a judgement call and it is yours to overturn.**
7. ⚠️ **`AFogVolume` now has `UFUNCTION`s for the first time.** `FogVolume.h`'s Blueprint-seam paragraph said, in its own words, that this sentence *"EXPIRES rather than forbids"* if one is ever added. I have **amended it rather than deleted it** (`SC-§53` cl. 3) and re-derived the predicate that actually mattered: ⭐ **a Blueprint still cannot poll fog state** — the three new functions are `CallInEditor`-only, `#if WITH_EDITOR`, **not `BlueprintCallable`**, and the accessors remain plain C++. **The ban on a Blueprint-side liveness tick stands unchanged.** Please re-derive that yourself rather than taking my word (`SC-§40`).
8. ✅ **STRUCTURAL COUNTS — I RAN THE SUITE'S OWN ARITHMETIC AGAINST MY EDIT AND IT IS GREEN.** I reimplemented `SiegeFogVisualFixture::CountOccurrencesInCode` character-for-character (same comment-line rule) and re-ran **65 count assertions** harvested from `SiegeFogVisualTest` / `SiegeBrightSunTest` / `SiegeFogVolumeTest` against the edited files, plus all **18 `ExtractFunctionBody` signature pins**: **65 checks, 0 mismatches; 18/18 pins resolve to substantial bodies.** This covers the traps my diff could plausibly have sprung — `FogH` `BP_SiegeFog` == 0, `FogCpp` `TActorIterator<AActor>` == 0, the eight banned geometry literals (`640/360/260/32000/18000/7000/26000/12000`) == 0, `SpawnActor<AFogVolume>` == 1, `DestroyFogVisual();` == `ReleaseFogRenderFloor();` == 2, `TEXT("r.VolumetricFog` == 3, and the `MarkPackageDirty`/`SavePackage`/`GConfig` shortcut bans. ⛔ **This is a text simulation, NOT a suite run** — it proves my diff does not trip those counters; it proves nothing about compilation.
9. ⚠️ **I flattened `FogVolume.cpp` to LF with a scripted edit and restored CRLF.** Caught by `git`'s own warning, verified at byte level: **CRLF 1550 / bare LF 0**, matching the other two files. `git diff --numstat` is `298/0`, `75/0`, `313/0` — **pure insertions**. Flagged because a silent whole-file line-ending change is exactly the kind of thing that would corrupt a blob-oid check downstream.
10. ⛔ **The Unreal MCP was DOWN for this entire row** (`Unable to connect` — editor not running, consistent with `TASK-1175` leaving it down). So `SC-§113` cl. 1's *"MCP has no function-invocation tool"* is **ACCEPTED-AS-DECLARED** from `TASK-1167`, **not re-measured by me**. My own attempt is recorded above and it failed at connect, which says nothing either way about the toolset.

### Mutations I would run if I were allowed to (I was not)

| # | mutation | should redden |
|---|---|---|
| M1 | delete `RefreshFogVisual();` from `RaiseFog` | test 10 (4) visual-spawned — **and the existing ordering test**, so it is not the only witness |
| M2 | make `SpawnFogVisual` return before `FogVisualActor = Spawned;` | test 10 (4) owned-actor is null |
| M3 | delete `DestroyFogVisual();` from `RefreshFogVisual`'s down branch | test 10 (5) visual-is-gone |
| M4 | make `ResetFog` zero only the fog deadline | test 10 (5) prevention-is-down |
| M5 | delete `ReleaseFogRenderFloor();` from the down branch | test 10 (6) still-pinned-by-code — ⭐ **the cross-test-contamination guard** |
| M6 | point `FogVisualClassPath` at a non-existent package | `SpawnFogVisual`'s **Error #1** fires ⇒ test 10 reds **through the shipped instrument** |
| M7 | remove `Spawned->SetActorScale3D(RequestedScale3D);` | `SpawnFogVisual`'s scale **Error** fires ⇒ test 10 reds — ⭐ **`TASK-1071` becomes suite-visible for the first time** |
| M8 | rename the console command in `CommandNameRaise` only | `-ExecCmds="Siege.Fog.Raise"` prints *"Command not recognized"* — the **wiring**, not the callee (`SC-§36.1` in mutation form) |

⭐ **M6, M7 and M8 are the ones worth arguing about**: M6/M7 break the **callee** and redden through *production* instruments that had never fired; M8 breaks the **call site**, which is the mutation class this project has already been burned by.

---

## 7. WHAT THIS UNBLOCKS (spec cl. 5 — named so the gate can check I enabled them)

I discharged **none** of these, as instructed. I made them **possible**:

- **(i) ask (B)'s 5-position × 3-time re-measurement** — ✅ reachable: `Siege.Fog.Raise` in PIE puts real fog up on demand at any camera position and time.
- **(ii) `TASK-1166` WARN-10, "does a fog raise dirty a package?"** — ✅ reachable, and §6.4 names the exact hook (snapshot `UPackage::IsDirty()` around the raise, in test 10 or a commandlet).
- **(iii) `Error #7` observed in both directions** — ⚠️ **NOT reachable, and not for a reason I can fix: `ApplyFogVisualMaterial` does not exist at this base.** It went with `TASK-1165`'s revert (`74baab3`). ⛔ **The row's cl. 5(iii) is stale** (`SC-§91`) and I have not invented work to satisfy it. What I *have* shipped is the capability that makes it observable **the moment that function is re-landed** — its `Error` would fire inside test 10's raise and redden the suite automatically.
- **Bonus, not asked for:** `TASK-1166` **WARN-8** (the mesh readback's ANY-vs-ALL verdict) and the whole `FOG-§12.5c` cl. 5 discriminator (§5) are now reachable too.

---

## 8. WHAT I COULD NOT VERIFY WITHOUT A COMPILE — THE HONEST LIST

⛔ **`PATH NEVER EXECUTED`.** ⛔ **`COOKED TARGET NOT COMPILED`.** ⛔ **No `Build.bat`, no suite, no editor, no MCP, no Git write.**

✅🚨 **DISCHARGED 2026-09-09 BY `TASK-1175`'s CODE CASE (`4a3da63`) — annotated, not deleted (`TL-§5c` cl. 4), because a reader who remembers this honest list must find its resolution rather than re-derive it.** Items 1-4 below **all landed green**: compile `Result: Succeeded` (0 errors, 0 warnings, DLL relinked) · test 10 `Result={Success}` in a **555 / 555** suite with **zero** `LogGitClaudeUnrealTest: Error` lines · the commandlet **resolved a non-null world** · the §4 log lines printed **as transcribed**. ⛔ **The list was still correct to write** — `PATH NEVER EXECUTED` was the truthful state at authoring time, and saying so is what let the gate transfer the duty instead of guessing (`SC-§113` cl. 3(b)).

Unverified **at authoring time**, in descending order of how much it would hurt:

1. **That any of it compiles.** UHT's handling of `#if WITH_EDITOR` around `UFUNCTION`s inside `GENERATED_BODY()` is standard and engine-native, but I did not run UHT.
2. **That test 10 passes.** See §6.1 and §6.2 — a red may be a real finding.
3. **That `-ExecCmds="Siege.Fog.Raise,QUIT_EDITOR"` resolves a non-null world in the editor commandlet.** *(⛔ corrected 2026-09-09 from the struck `"Siege.Fog.Raise;Quit"` — W9, `SC-§116`. ✅ **AND IT IS NO LONGER `NOT MEASURED`: `TASK-1175` ran it and the world resolved — `[FogVolume_0] AFogVolume spawned` on an editor world, with the loud `⚠️ ACTING ON AN EDITOR WORLD ('L_Arena')` warning firing exactly as designed.**)* The delegate is `WithWorldAndArgs` and the console resolves the world itself; if it hands back null, the command says so **loudly** rather than silently doing nothing — which is the design, ~~but it is **`NOT MEASURED`**~~ **and it is now MEASURED: it resolved, and the loud editor-world warning fired.**
4. **Every log line in §4.** Transcribed from the format strings, never observed.
5. **That the suite total becomes 555.** Derived from `554 + 1`; `TASK-1175` reports the **executed** `N/M` (`CONVENTIONS.md:5773` — a suite total cites the execution, never a macro count).
6. **The `CallInEditor` buttons rendering in the Details panel.** No editor was running.
