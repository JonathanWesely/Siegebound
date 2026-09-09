# QA Report — TASK-1174 (gate over TASK-1173, [FOGTRIGGER-REACHABLE])

**Verdict: `PASS` — 0 code blockers · 1 DEFERRED BLOCKER owed by `TASK-1175` · 7 warnings · 3 nits**

⚠️ **THE PASS IS NOT UNCONDITIONAL.** The board's cl. (1) demands a quoted log line proving `AFogVolume`
executed and calls its absence a BLOCKER. It is absent, and `TASK-1173`'s own cl. (6) **forbade the
compile that would produce it**. I am not bouncing a diff back to an agent who is structurally barred
from satisfying the demand — that is a false fail whose review cycle can produce nothing. Instead the
demand is transferred **whole and non-waivable** to the host, as **BLOCKER-0** below. If `TASK-1175`
commits without discharging it, `TASK-1175` has shipped `SC-§36.1` and this report says so in advance.

Reviewed at base `74baab3`. Files: `Source/GitClaudeUnrealTest/Siegebound/FogVolume.{h,cpp}` ·
`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp`. No file outside those three.

---

## 0. BLOCKER-0 — DEFERRED TO `TASK-1175`, NOT WAIVED

**`PATH NEVER EXECUTED` / `COOKED TARGET NOT COMPILED`** — the programmer wrote both phrases himself
rather than papering over them (`handoffs/TASK-1173-programmer.md` §0, §8). That disclosure is correct
behaviour and is the reason this is a deferral rather than a FAIL.

⛔ **`TASK-1175` MAY NOT COMMIT until all three of these land, in this order:**

1. Compile, and **parse the log for `Result:`** — `Build.bat` returns exit 0 on a failed build.
2. Run **either** lane and produce a log:
   - `UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<log>`
   - or `-ExecCmds="Siege.Fog.Raise;Siege.Fog.Status;Quit"` (same flags).
     ⚠️ `-ExecCmds` must be **one** PowerShell argument (`CONVENTIONS.md:4814`). The three command
     names are deliberately space-free, so this is safe as written.
3. Run the discharge count and **quote the matching lines into this report**:
   - Bash: `grep -cE "AFogVolume|Siege\.Fog\." <log>`
   - PowerShell: `(Select-String -Path <log> -Pattern 'AFogVolume|Siege\.Fog\.').Count`

🚨 **`SC-§113` cl. 2 measured this count at `0` over the entire editor log. A `0` after this diff is a
BLOCKER, never "clean."** A non-zero count with at least one quoted `LogGitClaudeUnrealTest:` line is
the positive control that ends the law's instance.

✅ **I verified the grep can actually match** (a discharge that cannot match is a false green waiting to
happen):
- `FogVolume.cpp:338` — `TEXT("[%s] AFogVolume spawned — the fog-state actor now exists for this match (FOG-§10.1: one state object).")` — the literal token `AFogVolume` is in the **shipped** `FindOrSpawn`, so **both** lanes match, not only the new one.
- `FogVolume.cpp:66-68` — the three command names are `TEXT("Siege.Fog.Raise" / ".Clear" / ".Status")` and each is interpolated into its own `[%s]` prefix, so `Siege\.Fog\.` matches every console line.
- ⚠️ Note for the host: `*GetNameSafe(this)` yields `FogVolume_0`, which does **not** contain `AFogVolume`. Lines such as `Fog INTEGRITY FLOOR engaged` and `Fog VISUAL spawned` therefore match **neither** alternative. That is fine (the alternation is an OR and other lines carry it), but do not read a low count as "the floor did not run."

✅ **Verbosity re-derived, because a prior instrument printed nothing and an empty log read as a pass:**
`Source/GitClaudeUnrealTest/GitClaudeUnrealTest.h:8` — `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)`
⇒ runtime default `Log`, compile-time max `All`. Every line above prints with **no** `-LogCmds` and no
`Log LogGitClaudeUnrealTest Verbose`. I also grepped all of `Config/` for `[Core.Log]` /
`LogGitClaudeUnrealTest` — **zero hits**, so nothing overrides it. The claim holds end to end.

---

## 1. RULING ON THE THREE DELIVERABLES

### (a) `Siegebound.Fog.RaiseFogIsActuallyExecutedInARealWorldAndTheVisualAppearsThenGoes` — ✅ ACCEPTED

⭐ **The "same two doors" claim is TRUE, and I read `ResolveSpell` myself rather than accepting it.**

| | write door | raise |
|---|---|---|
| shipped card (`FogCover` arm) | `SpellLibrary.cpp:668` `AFogVolume::FindOrSpawn(World)` | `SpellLibrary.cpp:690` `FogVolume->RaiseFog()` |
| test 10 | `SiegeFogVisualTest.cpp:1634` | `SiegeFogVisualTest.cpp:1650` |
| `Siege.Fog.Raise` | `FogVolume.cpp:125` | `FogVolume.cpp:135` |

Character for character, same order, no bespoke spawn, no raw field write, no second copy of any policy.
**This is not a re-implementation wearing the entry point's name.** The `FogClear` arm
(`SpellLibrary.cpp:722` → `:740` `ApplyBrightSun`) is the other door and is untouched.

Also verified for this lane:
- **Assertions pin resolved state, not calls made** (`SC-§104`): `Volume->IsFogActive()` (`:1654`, `:1701`),
  `FindOwnedVisual(World, Volume)` (`:1643`, `:1656`, `:1703`), the console read-back at `:1718-1721`.
  The negative control at `:1628` (`TestNull` on a fresh world) is what makes the non-nulls evidence.
- **`FindOwnedVisual` asks OWNERSHIP, not class** (`:1577-1594`), which measures the wiring
  `SpawnFogVisual` actually established (`FogVolume.cpp:937` `SpawnParams.Owner = this`). A class sweep
  would stay green with the handle never attached. Correct choice.
- **The rig is reachable**: `AFogVolume::FogRenderFloorCVarVolumetricFog` is **public**
  (`FogVolume.h:848`; `public:` at `:381`, `protected:` at `:954`), so `:1714` compiles.
  `bEnforceFogRenderFloor = true` by default (`FogVolume.h:1041`), so step (6) is **not vacuous** — the
  cvar really is pinned at `ECVF_SetByCode` and really must come back.
- **The scale is deliberately not re-asserted** (`§6.5`). **I agree, and I am not overturning it.**
  `SpawnFogVisual` already owns the verdict (`FogVolume.cpp:1009`) through the pure
  `FogVisualScaleMatches`, which test 6 already exercises against the *measured* vendor substitute. A
  second predicate here would be a paraphrase that can drift, and if the two disagreed nobody could say
  which was right. The `AddInfo` at `:1686` reports MEASURED values only (`GetActorScale3D` /
  `GetActorLocation`), never an echoed request. That is the right shape.

### (b) `Siege.Fog.Raise` / `.Clear` / `.Status` — ✅ ACCEPTED, and this is the lane that closes `SC-§113` cl. 4

⭐ **The author's argument checks out.** `-ExecCmds` is this repo's proven execution lane — every suite
run in its history is `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit"`,
and the board's own `TASK-1175` cl. (0) prescribes that same invocation. A console command reaches it by
substituting the command list. **No new tooling, and the channel is one this project has measured itself
using** — which is exactly what cl. 4 demands and exactly what a bare `UFUNCTION` would not have given
(MCP has no function-invocation tool).

Mechanically verified against the installed 5.8 headers:
- `FAutoConsoleCommandWithWorldAndArgs(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate&, uint32 Flags = ECVF_Default)` — `IConsoleManager.h:2423`. The four call arguments at `FogVolume.cpp:225-241` match in order and type. ✅
- `DECLARE_DELEGATE_TwoParams(FConsoleCommandWithWorldAndArgsDelegate, const TArray<FString>&, UWorld*)` — `IConsoleManager.h:257`. `ExecRaiseFog/ExecClearFog/ExecLogFogState(const TArray<FString>&, UWorld*)` match **in that parameter order**. ✅ (A reversed pair is the classic failure here; it is not present.)
- The three command-name pointers (`FogVolume.cpp:66-68`) are `static const TCHAR* const` initialised from string literals ⇒ **constant-initialised**, therefore live before the dynamic initialisation of the three `FAutoConsole…` globals in the same TU. No static-init-order hazard. ✅
- **Shipping fence, both halves real:** `#if !UE_BUILD_SHIPPING` (`:56`/`:243`) removes the registration in Shipping; `ECVF_Cheat` (`IConsoleManager.h:76`) makes `IConsoleObject::IsEnabled()` refuse it wherever `DISABLE_CHEAT_CVARS` is set, which also covers **TEST**, which the first does not. This is the `USiegeCheatManager` house rule (`SiegeCheatManager.h:46` names `FAutoConsoleCommand` as the sanctioned dev-command shape), **not a second pattern**. ⇒ board cl. (5) satisfied: **a player cannot reach these in a packaged Shipping build.**
- `UWorld::IsGameWorld` — `World.h:4172`, between `#endif` at `:4067` and `#if WITH_EDITOR` at `:4199` ⇒ **not editor-guarded**. ✅

### (c) The three `CallInEditor` buttons — ✅ ACCEPTED, and they do **not** repeat `TASK-1165`'s defect

🚨 **This was the highest-risk item and I re-derived it rather than accepting the tally (`SC-§104`).**
The `TASK-1166` BLOCKER-1 defect was an *engine* API that is editor-only being called from a `"Type": "Runtime"`
module (`AActor::RerunConstructionScripts`, `Actor.h:3415-3418`), so the Editor target compiled and the
cooked target did not.

**That is not what is here.** What is guarded is *our own* declaration, symmetrically:
- declaration `FogVolume.h:900` `#if WITH_EDITOR` … `:952` `#endif // WITH_EDITOR`
- definition `FogVolume.cpp:690` `#if WITH_EDITOR` … `:758` `#endif // WITH_EDITOR`

No engine symbol inside those blocks is editor-only. The bodies call only `RaiseFog()`, `ResetFog()`,
`IsFogActive()`, `IsFogPrevented()`, `GetFogPreventionSecondsRemaining()`, `IsValid`, `GetNameSafe`,
`UE_LOG` — all this class's own or CoreUObject runtime. ⛔ **Nothing in the fog's mechanism is guarded
anywhere**: `RaiseFog`, `RefreshFogVisual`, `SpawnFogVisual`, `EnforceFogRenderFloor`,
`ReleaseFogRenderFloor`, `ResetFog`, `EndPlay` are all outside every new `#if`. ⇒ `SC-§111` cl. 3(c)'s
forbidden case (the guard **is** the mechanism) does not apply.

✅ **And the buttons will actually render** — verified, not assumed:
`Editor/PropertyEditor/Private/PropertyCustomizationHelpers.cpp:1375`
`const bool bCanCall = TestFunction->GetBoolMetaData(NAME_CallInEditor) && (TestFunction->ParmsSize == 0);`
⇒ `CallInEditor` **alone is sufficient**; `BlueprintCallable` is *not* required, and all three functions
take zero parameters. Omitting `BlueprintCallable` is therefore both correct and free.

✅ **The Blueprint-seam predicate re-derived myself (`SC-§40`), not taken on the author's word:** a
Blueprint still cannot poll fog state. `IsFogActive`, `IsFogPrevented`, `GetFogPreventionSecondsRemaining`,
`Find`, `FindOrSpawn`, `BrightSunWindowSeconds` remain plain C++ (`FogVolume.h:399-587`); the only three
`UFUNCTION`s in the file are the `CallInEditor` trio, none `BlueprintCallable`, all `#if WITH_EDITOR`.
The ban on a Blueprint-side liveness tick stands. The old paragraph (`FogVolume.h:305-325`) was **kept
verbatim** and amended below it (`:327-346`) rather than deleted — `SC-§53` cl. 3 honoured.

⚖️ **I accept the author's own framing that (c) is NOT an agent lane.** The editor bridge has no click.
The row is closed by (a) and (b); (c) is 🧑 Jonathan's convenience and its instance-only limitation is
correctly disclosed (`FogVolume.cpp:702-705`) so it is never filed as a bug.

---

## 2. 🚨 RULING ON HAZARD 1 — THE `AddExpectedError` QUESTION

**⛔ RULED: `AddExpectedError` is REFUSED anywhere in test 10, and I am stating it as a standing
instruction to `TASK-1175` and to any programmer who receives a red from this test.**

First, a correction to the premise. **There are SIX `Error` sites in `FogVolume.cpp`, not seven.** I
counted `UE_LOG(LogGitClaudeUnrealTest, Error,` directly:

| # | site | fires when | could it legitimately fire in test 10? |
|---|---|---|---|
| 1 | `:912` | `FogVisualClassPath().TryLoadClass<AActor>()` returned null | **YES, low.** Would mean the shipped path cannot load `/Game/Blueprints/BP_SiegeFog` in the sanctioned runner. **A real finding.** |
| 2 | `:955` | world refused the spawn | Effectively no — `SpawnParams.SpawnCollisionHandlingOverride = AlwaysSpawn` (`:942`). A fire would be real. |
| 3 | `:1009` | **the `TASK-1071` scale substitution returned** | **YES.** This is the one the row exists to make suite-visible. ⛔ **Suppressing it re-ships the exact defect.** |
| 4 | `:1269` | one of the three floored cvars is absent from this build | **YES, low** (Renderer module registers them even under `-nullrhi`). A fire is a *rig/config* fact, and the honest reading is `NOT MEASURED`, never a pass. |
| 5 | `:1388` | the floor did not take (four terms) | **YES, low** — see §3 below; all four terms are satisfiable in the rig. A fire is real. |
| 6 | `:1523` | the floor was not released | **YES, low.** A fire means this test stranded a process-global cvar over the rest of the suite. |

**Why `AddExpectedError` is refused, per site:**
- On **#3** it is categorically forbidden. It would restore precisely the silence `SC-§113` was written
  about, inside the row that exists to end it.
- On **#1 / #2 / #5 / #6** it would hide a genuine "the shipped path cannot run in the sanctioned lane"
  finding — the strongest information this row can produce and the reason it was boarded.
- On **#4**, the only site with a colourable case (it is a property of the runner, not of the code),
  it is *still* wrong. `AddExpectedError` matches by pattern and **consumes** the error, so the same
  string arising from a real cause would be swallowed too. The correct remedy for a genuine rig gap is
  an explicit precondition that reds as `NOT MEASURED` — not a suppression that reds as nothing.

⛔ **Corollary, addressed to the host and to the next programmer:** a red from test 10 is **read**, not
reverted and not silenced. Quote the error text verbatim into this report and route it back. A red here
may be the row **working**.

---

## 3. THE OTHER VOLUNTEERED HAZARDS — RULED

**Hazard 2 (`PATH NEVER EXECUTED`)** — ruled in §0. The expected strings in `handoffs/…§4` were checked
against the shipped `TEXT(...)` literals; the discharge grep **can** match; verbosity is genuinely
default. Deferred to `TASK-1175`, not waived.

**Hazard 3 (first `CreateWorld` / `SpawnActor`; lifetime and teardown)** — ruled ACCEPTABLE with one
warning (**W1**, below). Detail:
- The fixture (`SiegeFogVisualTest.cpp:1498-1567`) matches `Developer/CQTest/Private/Components/ActorTestSpawner.cpp:80-93`
  and `:13-30` step for step. Teardown order (`RouteEndPlay` loop → `ShutdownWorldNetDriver` →
  `DestroyWorld(true)` → `SetPhysicsScene(nullptr)` → `DestroyWorldContext` → `RemoveFromRoot`) is the
  engine's own order, copied exactly. The world is created **before** the context, deliberately, so a
  failed `CreateWorld` cannot orphan a context — a real improvement on the engine's shape.
- **RAII covers every exit.** `FScopedPlayWorld Scoped;` is on the stack, so both early returns
  (`:1610` world-creation failure, `:1638` null volume) still tear down. ✅
- **No cross-test contamination on the shipped path.** The floor is engaged at `:1650` and released at
  `:1699` (`ResetFog` → `RefreshFogVisual` down-branch → `ReleaseFogRenderFloor`, `FogVolume.cpp:849-863`),
  and **there is no return between them**. Step (6) then reads the priority **back off the machine**
  (`:1718-1721`) rather than asserting the call was made — correct under `SC-§104`.
- **`AExponentialHeightFog` spawn is fidelity, not suppression — I agree with §6.3.** Verified: the
  lookup is `TActorIterator<AExponentialHeightFog>` → `GetComponent()` (`FogVolume.cpp:1185-1196`), so
  the test's actor **is** found; `UExponentialHeightFogComponent::SetVolumetricFog`
  (`ExponentialHeightFogComponent.cpp:357-365`) has **no** `AreDynamicDataChangesAllowed` guard, so
  the write takes even on a freshly-spawned actor; `VolumetricFogDistance` defaults to `6000.f` (`:112`).
  ⇒ all four terms of Error #5's condition are satisfiable, and **the check is not suppressed** — a
  floor that failed for any other reason still reds.
- **The stale headers.** ✅ Confirmed stale — but the count is wrong and one of them is in scope
  (**W2**, **W4**). The 15-file sweep is correctly *not* this row's job.
- **Vendor-package dirtying (§6.4)** — disclosed rather than dodged; the world lives in
  `GetTransientPackage()` so no map is touched, and the sanctioned runner never saves. ⚠️ The warning
  stands for anyone running this from the Session Frontend in a live editor: **do not press Save All.**

**Hazard 4 (cl. 5(iii) is stale)** — ✅ **The right call, confirmed independently.** `ApplyFogVisualMaterial`
does not exist at `74baab3`; I grepped the file and it is absent, consistent with `TASK-1165`'s revert.
Error #7 cannot be observed because it does not exist. **Inventing work to satisfy a stale clause would
have been the defect**, not the omission. The capability shipped here makes it observable the moment that
function re-lands, which is the correct discharge.

---

## 4. SCOPE FENCE + HELD BYTES (board cl. 3 and cl. 4)

**Scope — ✅ ZERO behavioural change to the fog.** No colour, no density, no cvar value, no material.
`RaiseFog`, `ApplyBrightSun`, `ResetFog`, `RefreshFogVisual`, `SpawnFogVisual`, `DestroyFogVisual`,
`EnforceFogRenderFloor`, `ReleaseFogRenderFloor`, `EndPlay`, `Find`, `FindOrSpawn`, `FogVisualTransform`,
`FogVisualClassPath`, `FogVisualScaleMatches`, `CaptureFogRenderFloorPriorState` all read as their
pre-existing selves with their `TASK-1147` / `TASK-1068` provenance intact.

**⭐ The `−0` claim — the strongest and cheapest-to-falsify — corroborated structurally, NOT by git.**
I hold no Git access by charter, so I verified it the way a reader can:
- `FogVolume.h` — the old Blueprint-seam paragraph survives **verbatim** at `:305-325` (*"⛔ A DATED FACT,
  ⛔ NOT A BAN: `AFogVolume` exposes NO `UFUNCTION` AT ALL…"*), with the amendment appended **below** it
  at `:327-346`. That is additive. The `#if WITH_EDITOR` block at `:900-952` sits between `EndPlay`'s
  declaration (`:898`) and `protected:` (`:954`) — a clean insertion.
- `FogVolume.cpp` — the console block occupies `:17-243`, entirely **above** `AFogVolume::AFogVolume()`
  (`:245`); the Dev block occupies `:690-758`, entirely **between** `EndPlay`'s closing brace (`:688`)
  and `FogVisualClassPath` (`:760`).
- `SiegeFogVisualTest.cpp` — the new material is `:1426-1729`, appended before `#endif // WITH_DEV_AUTOMATION_TESTS`.
  The file header at `:97-100` is **untouched** (which is W2, but it corroborates `−0`).

⛔ **`TASK-1175` OWES THE ARITHMETIC I CANNOT RUN.** Before the commit, confirm at the git root
(**one level above the project dir** — `SC-§102`):
```
git diff --numstat 20c1bea HEAD -- <the three files>     # expect 298/0, 75/0, 313/0
```
🚨 **A `−` on any of the three is an immediate STOP-AND-ESCALATE**, because it would mean either
`TASK-1165`'s held bytes were touched or the §6.9 CRLF flatten/restore did not round-trip. **Anchor the
pathspec correctly and run a deliberate-bad-path negative control first** — a mis-anchored pathspec
answers with silence, not an error.

**Which lines belong to which row:** all added lines are `TASK-1173`'s; every pre-existing line is
`20c1bea`'s. `TASK-1165`'s held diff is **not** present at this base — the author measured
`git diff --stat 20c1bea HEAD` on the three files as EMPTY before starting, and the file contents I read
are consistent with the reverted state (`ApplyFogVisualMaterial` / `VerifyFogVisualMaterial` absent).

---

## 5. THE CALL-GRAPH SWEEP — this project's scar, checked explicitly

⭐ **Both of the last two blocker classes lived in the call graph, not in either diff.** I read the
callers. **No regression found**, and the reasoning is recorded so nobody re-derives it:

**Every structural census that reads `FogVolume.cpp` / `FogVolume.h` (41 assertion sites across
`SiegeFogVisualTest.cpp`, `SiegeFogVolumeTest.cpp`, `SiegeBrightSunTest.cpp`) survives this diff.** The
ones that could plausibly have broken:

| census | site | why it survives |
|---|---|---|
| 8 banned geometry literals, **substring**-matched on code lines of `FogVolume.cpp` | `SiegeFogVisualTest.cpp:784-789` | 🚨 the dangerous one. A grep of `640\|360\|260\|32000\|18000\|7000\|26000\|12000` over the **whole** of `FogVolume.cpp` returns **zero matches, comments included**. ✅ |
| `TActorIterator<AActor>` == 0 in `FogCpp` | `:704` | the new one is in `Tests/`, not `FogVolume.cpp`. ✅ |
| `/Game/Blueprints/BP_SiegeFog` == 1 in `FogCpp`; `BP_SiegeFog` == 0 in `FogH` | `:747`, `:749` | no new `/Game/` literal; the header amendment is doc-comment lines, which the helper skips. ✅ |
| `SpawnActor<AFogVolume>` == 1 in `FogCpp` | `SiegeFogVolumeTest.cpp:582` | the console command calls `FindOrSpawn`, not `SpawnActor<>`. ✅ |
| `EnforceFogRenderFloor();` == 1, `ReleaseFogRenderFloor();` == `DestroyFogVisual();` == 2 | `SiegeFogVisualTest.cpp:1226-1236` | no new call sites. ✅ |
| `FogActiveUntilTimeSeconds =` == 3, `FogPreventedUntilTimeSeconds =` == 2 | `:520`, `SiegeBrightSunTest.cpp:800-804` | no new deadline writer. ✅ |
| banned companion flags (`bFogActive`, `bFogPrevented`, `bVisualSpawned`, …) == 0 in both files | `:695`, `:697` | new locals are `bRaised`, `bStillPinnedByCode`. ✅ |
| forbidden shortcuts (`MarkPackageDirty`, `SavePackage`, `GConfig`, …) == 0 in `FogCpp` | `:1414` | the new warning says *"MARKS IT DIRTY"*, not `MarkPackageDirty`; case-sensitive, and it is a code line either way. ✅ |
| `TEXT("r.VolumetricFog` == 3, `FSiegeFogStatics` == 0, `TransformScaleMethod` == 0 | `:1389`, `:1421`, `:963` | none added. ✅ |
| `FMath::FloorToFloat(` == 1, trace needles == 0, `FSiegeFogTuning` == 0, `Suspended` == 0, `FogVisionCeilingUU` == 0 | `SiegeBrightSunTest.cpp:383-470`, `:630` | none added. ✅ |

**`ExtractFunctionBody` probes are safe.** The helper takes the **first** match, and the new console
block sits *above* the real definitions. `"AFogVolume* AFogVolume::Find("` and
`"AFogVolume* AFogVolume::FindOrSpawn("` do **not** match the new call sites, which read
`AFogVolume* const Volume = AFogVolume::Find(World);` — the required `AFogVolume*` + ` AFogVolume::Find(`
adjacency is broken by `const Volume = `. ✅

**Every tree-wide census excludes `Tests/` by an explicit predicate** (`SiegeSpellRoutingTest.cpp:242-247`,
`SiegeAcquisitionFunnelTest.cpp:234-239`, `SiegeFogClampTest.cpp:194-199`, `SiegeInvisibilityTest.cpp:322-327`),
so the test file's new `TActorIterator<AActor>` / `GetOwner()` / `SpawnActor` cannot trip a shipping-source
scan. ✅

---

## 6. `SC-§111` cl. 3(a) — EVERY SYMBOL RE-DERIVED OFF THE INSTALLED 5.8 SOURCE

⛔ **I did not accept the author's "22 symbols, 0 editor-guarded" tally.** I resolved the enclosing
preprocessor block for each symbol by taking a directive census of the header and checking which `#if`
range each line falls in.

**Production lane (`FogVolume.cpp`, `Runtime` module — the column that matters):**

| symbol | header : line | enclosing block | guarded? |
|---|---|---|---|
| `FAutoConsoleCommandWithWorldAndArgs` | `IConsoleManager.h:2411` (ctor `:2423`) | `#if !NO_CVARS`; **variadic stub exists in the `#else` at `:2493-2497`** ⇒ compiles in **both** branches | **NO** ✅ |
| `FConsoleCommandWithWorldAndArgsDelegate` | `IConsoleManager.h:257` | same `#if`, `#else` at `:268` supplies null twins | **NO** ✅ |
| `ECVF_Cheat` / `ECVF_Default` | `IConsoleManager.h:76` / `:71` (`enum EConsoleVariableFlags` at `:63`) | file scope, before any directive | **NO** ✅ |
| `UWorld::IsGameWorld` | `World.h:4172` | between `#endif`@`4067` and `#if WITH_EDITOR`@`4199` | **NO** ✅ |
| `UObject::GetName` / `GetNameSafe` | `UObjectBaseUtility.h` / `UObjectGlobals.h` | core, unguarded; already used elsewhere in this file | **NO** ✅ |

**Test lane (`Tests/SiegeFogVisualTest.cpp`, inside `#if WITH_DEV_AUTOMATION_TESTS` — ⚠️ which is *not*
Editor-only, so it does compile into a cooked Development target and the column is load-bearing):**

| symbol | header : line | enclosing block | guarded? |
|---|---|---|---|
| `UWorld::CreateWorld` | `World.h:3375` | between `#endif`@`3362` and `#if WITH_EDITOR`@`3388` | **NO** ✅ |
| `UWorld::DestroyWorld` | `World.h:3380` | same range | **NO** ✅ |
| `UWorld::InitializeActorsForPlay` | `World.h:3958` | between `#endif`@`3579` and `#if`@`4049` | **NO** ✅ |
| `UWorld::BeginPlay` | `World.h:3971` | same range | **NO** ✅ |
| `UWorld::AreActorsInitialized` | `World.h:2821` | between `#endif`@`2104` and `#if`@`3138` | **NO** ✅ |
| `UWorld::SetPhysicsScene` | `World.h:2894` | same range | **NO** ✅ |
| `FWorldContext::SetCurrentWorld` | `Engine.h:473` | between `#endif`@`59` and `#if`@`965` | **NO** ✅ |
| `UEngine::ShutdownWorldNetDriver` | `Engine.h:3401` | between `#endif`@`3307` and `#if WITH_SERVER_CODE`@`3474` | **NO** ✅ |
| `UEngine::CreateNewWorldContext` / `DestroyWorldContext` | `Engine.h:3626` / `:3628` | between `#endif`@`3476` and `#if WITH_EDITOR`@`3823` | **NO** ✅ |
| `AActor::RouteEndPlay` | `Actor.h:3381`, `ENGINE_API`, public | unguarded | **NO** ✅ |
| `AExponentialHeightFog` | `ExponentialHeightFog.h:14` | file scope; `MinimalAPI` exports `StaticClass()`, which is all `SpawnActor<>` needs | **NO** ✅ |
| `FSoftClassPath::ResolveClass` · `MakeUniqueObjectName`(4-arg) · `EUniqueObjectNameOptions` · `FActorRange`/`TActorIterator` · `FURL` | `SoftObjectPath.h` · `UObjectGlobals.h` · `EngineUtils.h` · `EngineBaseTypes.h` | core runtime; the 4-arg `MakeUniqueObjectName` + `EUniqueObjectNameOptions::GloballyUnique` pairing is used verbatim by `ActorTestSpawner.cpp:82` | **NO** ✅ |

⇒ ✅ **ZERO editor-only engine symbols on the (a) or (b) lane, re-derived per symbol.** The
`TASK-1166` BLOCKER-1 defect is **not** repeated. The author's corrected "5 production + 17 test = 22"
matches my own accounting; his first "17" was the test table alone and he struck it rather than
overwriting it (`SC-§53` cl. 3) — correct handling.

⛔ Includes are explicit (IWYU): `HAL/IConsoleManager.h` already present (`FogVolume.cpp:10`); the test
adds `Engine/EngineBaseTypes.h`, `Engine/ExponentialHeightFog.h`, `Engine/Engine.h`, `Engine/World.h`,
`EngineUtils.h`, `GameFramework/Actor.h`, `GitClaudeUnrealTest.h`, `HAL/IConsoleManager.h`
(`SiegeFogVisualTest.cpp:6-13`). Nothing relies on a transitive pull.

---

## 7. `FOG-§12.5c` cl. 5 — CAN THIS INSTRUMENT TAKE THE DISCRIMINATING SERIES?

⚖️ **YES — through deliverable (b), and only through (b). The row is therefore a real instrument and not
merely code.** But the recipe as written carries an uncontrolled confound (W6), and a row that takes the
series without controlling for it may produce a void answer.

**Per deliverable:**
- **(a) CANNOT.** Headless, `-nullrhi`, no pixels, and it raises then resets within the same frame
  sequence. `WIN=cycle` needs ≥180 continuous seconds and ≥60 frames from a **real match**.
- **(b) CAN.** `Siege.Fog.Raise` typed into the PIE console on `L_Arena` goes through
  `FindOrSpawn` → `RaiseFog` → `RefreshFogVisual` → `SpawnFogVisual`, producing the **identical**
  `BP_SiegeFog` at the identical derived transform. That is the card-raised object, which is what cl. 5
  names as the variable.
- **(c) CANNOT create one** (instance-only), though it can refresh or clear during a live match.

**Two properties I verified, because the ≥180 s hold depends on them:**
1. `FogDurationSeconds = 300 s` ≥ 180 s ⇒ **one raise covers a full `WIN=cycle` window with no refresh
   at all.** The handoff's "re-issue every ~120 s" is only needed for spans > 300 s.
2. ✅ **A refresh does not re-spawn the visual and does not re-write the floor**, so there is no
   discontinuity in the series: `RefreshFogVisual` spawns only `if (!IsValid(FogVisualActor.Get()))`
   (`FogVolume.cpp:876`), and `CaptureFogRenderFloorPriorState` returns `Prior` unchanged on re-entry
   (`:1213-1216`) with `EnforceFogRenderFloor` returning early at `:1312`. The author's "verify by
   confirming no second `Fog VISUAL spawned` line" is the correct check.

⭐ **The step-3 control is the best thing in the recipe** and I endorse it: capture the
`Fog VISUAL spawned` line *before* one pixel. It prints **ACHIEVED** scale and Z read off the actor
(`FogVolume.cpp:1009` region reads back with `GetActorScale3D`/`GetActorLocation`), so if the card-raised
geometry differs from the probe's, one grep says so. Correctly labelled a **lead**, not a conclusion.

🚨 **But see W6:** the recipe does not control for the integrity floor, which is *the* systematic
difference between the two rigs.

**Statement of the discriminator's own window, per `SC-§115`:** the series must be `WIN=cycle`
(≥ 180 s, ≥ 60 frames, values frozen), reported as **mean, min, max, fraction-of-frames-passing**, on the
far-field gate band `y ∈ [0.42,0.48]`, `x ∈ [0.30,0.70]`, **with the optically-thick band
`y ∈ [0.05,0.25]` as a mandatory companion** (`FOG-§12.5c` cl. 1-2). The thick band is the only
cross-instrument link to 🧑 his actual screen (`VID-007` `0.9153` vs probe `0.9145`). A row reporting
only the far field has thrown that away.

---

## 8. FINDINGS

### BLOCKERS — 0 on the diff · 1 deferred

**BLOCKER-0 (DEFERRED to `TASK-1175`, NOT waived)** — `handoffs/TASK-1173-programmer.md:11` /
board `TASK-1174` cl. (1) — the quoted log line proving `AFogVolume` executed does not exist, because
`TASK-1173` cl. (6) forbade the compile that would produce it. **Discharge conditions and the exact
commands are in §0.** ⛔ `0` from the discharge grep is a BLOCKER, never "clean." A commit without the
grep is `SC-§36.1` shipped by the host.

### WARNINGS — 7

**W1 — `SiegeFogVisualTest.cpp:1541-1555` (and its justification at `:1491-1496`) — the teardown's
`RouteEndPlay` loop is DEAD AS WRITTEN, and the comment promises a protection that does not exist.**
Re-derived off the installed engine: `UWorld::BeginPlay()` (`World.cpp:6153-6187`) does **not** set
`bBegunPlay`. The **only** writer is `AWorldSettings::NotifyBeginPlay` (`WorldSettings.cpp:377`
`World->SetBegunPlay(true)`), reachable only through `AGameModeBase::StartPlay` →
`AGameStateBase::HandleBeginPlay`. This world has no game mode (`InitializeActorsForPlay(FURL())` does not
spawn one), so `World->HasBegunPlay()` stays **false** ⇒ `AActor::PostActorConstruction` never dispatches
`BeginPlay` ⇒ `AActor::RouteEndPlay` returns without calling `EndPlay` ⇒ **`AFogVolume::EndPlay` never
runs at teardown.** The comment's claim — *"a teardown that skipped it would leave `r.VolumetricFog`
pinned at `SetByCode` for the REST OF THE SUITE PROCESS — 553 other tests running under a console
variable this one stranded"* — is therefore describing a safety net that is not connected.
⛔ **It does not fire today**, and that is why this is a WARN and not a BLOCKER: step (5)'s explicit
`ResetFog()` (`:1699`) performs the release unconditionally, and there is **no return** between
`RaiseFog()` (`:1650`) and it. **Suggested fix (owning row, not me):** either (i) strike the false
justification and say plainly that `ResetFog()` is the release, or (ii) make the net real — call
`World->GetWorldSettings()->NotifyBeginPlay()` (or `World->SetBegunPlay(true)`) after
`InitializeActorsForPlay` so `RouteEndPlay` actually routes. ⛔ Do **not** silently delete the loop; it is
harmless and it becomes correct the moment (ii) lands.

**W2 — `SiegeFogVisualTest.cpp:97-100` — the file's OWN header is now FALSE, in the file this row owns
and edits.** It still asserts *"there is not one `SpawnActor` anywhere in `Siegebound/Tests/`, so ⛔
NOTHING HERE RUNS THE ACTUAL SPAWN … is ⛔ NOT executed by the suite."* Line `:1518` is
`UWorld::CreateWorld` and `:1622` is `World->SpawnActor<AExponentialHeightFog>()` — in the same file.
The correction at `:1432-1439` points **back** at the header; the header points **nowhere forward**, so a
reader who stops at `:100` is misled. The other ~15 files are legitimately out of scope (declared,
manager row); **this one is not** — `SC-§91` says sweep by SUBJECT in the file you are editing.
**Fix:** one struck-and-amended sentence at `:97`, this file only.

**W3 — `SiegeFogVisualTest.cpp:1450` (and handoff §6.2) — "SEVEN `Error` sites" is wrong; there are
SIX.** Measured: `FogVolume.cpp:912, 955, 1009, 1269, 1388, 1523` are the only
`UE_LOG(LogGitClaudeUnrealTest, Error,` in the file. The seventh was `ApplyFogVisualMaterial`'s, which
went with `TASK-1165`'s revert — the handoff says so itself at §7(iii), so the two halves of the same
document disagree. This is a **count in a comment that was wrong on the day it was written**, which is
the exact failure `FogVolume.h:311-315` documents about itself. **Fix:** state it as a predicate
(*"every `Error` site in this file"*), not a number.

**W4 — `SiegeFogVisualTest.cpp:1474` — "Nine files" understates the stale-sentence set.** The handoff's
own §1 names **13** test files plus `HeroCharacter.h` and `SiegeLadderClimbStatics.h` and calls it a
"fifteen-file sweep"; my grep finds the sentence in ≥15 test files. Two numbers for one fact, one of them
in shipped prose. Same remedy as W3: a predicate, not a count. (The **decision** not to sweep is correct
and I endorse it — a 15-file prose sweep inside a capability row is a refactor.)

**W5 — `FogVolume.cpp:99-104` — the shared editor-world warning asserts an effect `Siege.Fog.Clear`
cannot produce.** `IsWorldUsable` prints *"The fog-state actor is spawned into the map you currently have
OPEN, which ⛔ MARKS IT DIRTY"*, but `ExecClearFog` (`:166`) uses `AFogVolume::Find`, **never**
`FindOrSpawn`, and therefore cannot spawn anything. The gate is shared; the sentence is Raise-specific.
An instrument that asserts an effect its path cannot produce is precisely the class of lie this file's
own header is about. **Fix:** pass the spawn clause as a parameter, or move it into `ExecRaiseFog` and
leave the gate with the world-null and non-game-world facts only. (The *"do not save"* instruction is
still correct on the Clear path — destroying an actor in an editor world can dirty the level — so keep
that half.)

**W6 — `handoffs/TASK-1173-programmer.md` §5 — the `FOG-§12.5c` cl. 5 recipe does not control for the
integrity floor, which is the one systematic difference between the two rigs it compares.**
A card-raised fog runs `EnforceFogRenderFloor` (`FogVolume.cpp:1337-1344`), pinning `r.VolumetricFog=1`,
`r.VolumetricFog.GridPixelSize=16`, `r.VolumetricFog.GridSizeZ=64` at `ECVF_SetByCode`. **A hand-spawned
`BP_SiegeFog` probe runs no `AFogVolume` code at all and therefore never gets that froxel grid.** Grid
resolution is exactly the kind of parameter that can change the amplitude or period of a
temporal-accumulation artefact. ⇒ a row reporting *"cycle absent under the card"* could be reading the
**grid change**, not the **actuation** — and would void `FOG-§12.5c` on a confound. The recipe mentions
the floor only at step 8, and only as a scalability-exploit caveat. **Duty for whoever takes the series
(not for this diff):** either re-take the probe series with `r.VolumetricFog.GridPixelSize 16` /
`GridSizeZ 64` applied, or take the card series with `bEnforceFogRenderFloor = false` on the CDO — **and
say in the report which one, beside the actuation label** (`FOG-§12.5b`).

**W7 — `handoffs/TASK-1173-programmer.md` §4 — the discharge command is POSIX-only on a
PowerShell-primary host.** `grep -cE …` runs via the Bash tool but will not run in a PowerShell step, and
a discharge that silently fails to run is the same false-green class the row exists to end. Both
spellings are given in §0 of this report; use one and paste the count.

### NITS — 3

**N1 — `SiegeFogVisualTest.cpp:1518-1528` — `World->AddToRoot()` is redundant.**
`UWorld::CreateWorld(…, bAddToRoot = true, …)` (`World.h:3375`, 5th parameter) already roots the world.
`AddToRoot` sets a flag rather than a refcount, so the single `RemoveFromRoot()` at `:1561` still clears
it — **no leak**. Copied verbatim from `ActorTestSpawner.cpp:84-86`, which has the same redundancy.
Recorded so nobody later "fixes" the teardown by adding a second `RemoveFromRoot`, which would be the
actual bug.

**N2 — `SiegeFogVisualTest.cpp:1723-1726` — step (6)'s `else` branch `AddInfo`s *"NOT MEASURED — never a
pass"* while returning green.** It is **not** reachable without a red: if any of the three cvars is
missing, `EnforceFogRenderFloor` fires Error #4 (`FogVolume.cpp:1269`) during step (3) and the automation
framework converts that to a failure. So the branch is guarded by the shipped instrument. Recorded so the
reasoning is on the record rather than re-derived by the next reviewer.

**N3 — `FogVolume.cpp:116, 157, 193` — the `Args` parameter is unused in all three exec functions.**
Non-issue; identical to `RunTest(const FString& Parameters)` in every test in this project. Noted only so
it is not raised as a finding later.

---

## 9. NOTES FOR BUILD-MASTER (`TASK-1175`)

1. 🚨 **BLOCKER-0 IS YOURS.** Compile → run a lane → run the discharge grep → **quote the matching
   `LogGitClaudeUnrealTest:` lines into this report before the commit.** A `0` is a BLOCKER, not "clean."
   Exact commands in §0.
2. 🚨 **Parse the build log for `Result:`.** `Build.bat` returns exit 0 on a failed build (Live Coding
   mutex). Never trust `$LASTEXITCODE`.
3. 🚨 **Verify `git diff --numstat 20c1bea HEAD -- <the three files>` reads `298/0 · 75/0 · 313/0`** at the
   git root, which is **one level above the project dir**. Run a deliberate-bad-path negative control
   first — a mis-anchored pathspec answers with silence, not an error. **Any `−` ⇒ STOP AND ESCALATE**
   (`TASK-1165`'s held bytes, or the §6.9 CRLF round-trip).
4. **Suite total:** the author derives 555 from 554 + 1, but `TASK-1167` measured **556**. ⛔ Report the
   **executed** `N/M` from your own run and reconcile against your own previous run — never against a
   published absolute (`CONVENTIONS.md:5773`).
5. ⛔ **If test 10 reds, DO NOT revert it and DO NOT add `AddExpectedError`** (see §2). Quote the error
   text verbatim, append it here, and route back to `gameplay-programmer`. **A red may be the row
   working.** The three most likely reds and their meanings are tabulated in §2.
6. **The seven W-items and three N-items are not commit blockers.** W1, W2, W3, W4, W5 are one-line prose
   or one-call corrections that belong to a follow-up row on `SiegeFogVisualTest.cpp` / `FogVolume.cpp` —
   route them to the manager with this report's path. **W6 belongs to whichever row takes the
   `FOG-§12.5c` cl. 5 series** and must travel with it, or that row can produce a void answer.
7. **Do not run this test from the Session Frontend in a live editor and then press Save All.** The world
   is in `GetTransientPackage()` so no map is touched, but the test loads `BP_SiegeFog` and through it the
   read-only vendor pack, and `TASK-841` §5.3 measured that loading a vendor package dirties it. The
   sanctioned `-unattended … ;Quit` runner never saves ⇒ safe in the lane you use.
8. **Standing fences (`TL-§5e` cl. 7a):** `Content/Maps/L_Arena.umap` NEVER (report its hash) ·
   `Content/FogArea/**` NEVER · `Config/DefaultEngine.ini` NOT EDITED · `testvideo/**` NEVER staged ·
   **NEVER push.**
9. **The manager owes a row** for the ~15-file stale *"every test is HEADLESS"* sweep. It is correctly
   declared, not hidden, and it is correctly **not** this row's work.

---

## 10. DOES THIS CLOSE `SC-§113`? — EXPLICIT RULING

⚖️ **The three deliverables together DO close `SC-§113` cl. 4 — structurally now, and evidentially the
moment `TASK-1175` runs the grep in §0.**

- **cl. 4 asked for a channel this project has MEASURED ITSELF USING.** ✅ `-ExecCmds` is that channel:
  every suite run in this repository's history uses it, and the board's own `TASK-1175` cl. (0)
  prescribes it. Deliverable (b) reaches it by substituting the command list — **no new tooling**.
- **cl. 4's trap — "a `UFUNCTION` alone would not have fixed it, because the tool that would call one
  does not exist."** ✅ Not walked into. The row does not count the `UFUNCTION`s as an agent lane; it says
  so in its own header (`FogVolume.cpp:696-700`) and in the handoff (§1(c)). **The two agent-reachable
  lanes are (a) and (b), and both are real.**
- **cl. 3(c)'s positive control.** ✅ Built into the code as a log line
  (`FogVolume.cpp:140-146`) and as an assertion (`SiegeFogVisualTest.cpp:1651`). ⏳ **Not yet fired** —
  that is BLOCKER-0.
- **The durable half, which is worth more than the pokeable half:** (a) makes the fog path
  **gate-able forever**. `RaiseFog`, `RefreshFogVisual`, `EnforceFogRenderFloor`, `SpawnFogVisual` and
  `ReleaseFogRenderFloor` now execute on every suite run, and **six shipped `Error` sites that had never
  been reachable by anything become suite-visible for the first time** — including the `TASK-1071` scale
  substitution. That is the difference between an error that *cannot* fire and one that *chose* not to,
  which `SC-§113` was written about.

⛔ **What it does NOT close, stated so no green is read wider than it is:** the 300-second wait is not
executed; nothing here says the fog *looks* right (🧑 his eye remains the only instrument for legibility,
`AS-§6 A(e)`); and `FOG-§12.5c` cl. 5 is made **answerable**, not **answered** — with the confound in W6
still to be controlled by the row that answers it.

---

*Reviewed pre-compile. No code edited, no engine touched, no Git command run — by charter. Report path:
`.claude/pipeline/qa/TASK-1174-report.md` (named after the GATE row, `SC-§106`).*

---

# ✅ BLOCKER-0 — DISCHARGED 2026-09-09 BY `TASK-1175` (code case, build-master)

*Appended by the host, as §0 cl. 3 and §9 cl. 1 of this report instruct. Nothing above this line was edited.*

## 1. Compile — `Result: Succeeded`, PARSED FROM THE LOG

`Result: Succeeded` (build log line 39). `error C…` = 0, `warning C…` = 0, `Result: Failed` = 0.
Reader controlled: a bogus `^Result: ThisCannotExist` returned **0**.
⛔ `Build.bat` exit code was `0` and **that is not the evidence** — the log line is.
14 actions: `SiegeFogVisualTest.cpp` + `FogVolume.cpp` + 9 module TUs compiled, `UnrealEditor-GitClaudeUnrealTest.dll` **relinked** ⇒ the binary genuinely moved.
🚨 **`COOKED TARGET NOT COMPILED`** (`SC-§111`) — Editor target only.

## 2. Lane (a), the suite — **555 / 555, 0 Fail**

`Result={Success}` **555** · `Result={Fail}` **0** · Started **555** == Completed **555** · `**** TEST COMPLETE. EXIT CODE: 0 ****`.
Reader controlled: `Result={ThisValueCannotExist}` returned **0**.
**Exactly the predicted 555** (554 + 1). Reconciled against this host's own previous measured run (`554` at `74baab3`), never a published absolute.

`Siegebound.Fog.RaiseFogIsActuallyExecutedInARealWorldAndTheVisualAppearsThenGoes` ⇒ **`Result={Success}`**.

⛔ **NO RED.** `LogGitClaudeUnrealTest: Error` count = **0** — none of the six `Error` sites (`:912, :955, :1009, :1269, :1388, :1523`) fired. **No `AddExpectedError` was added anywhere** (§2's ruling honoured; there was nothing to suppress).

⚠️ One line matched a naive `Error` grep and is a **false positive** — it is test 10's own `AddInfo` prose *about* the Error site, quoted here so nobody re-derives it:
> `LogAutomationController: ⭐ (4) MEASURED off the spawned actor — ACHIEVED scale (640.000, 360.000, 260.000), ACHIEVED Z 7000.0. … The VERDICT on scale is SpawnFogVisual's own Error site, ⛔ not a predicate in this test`

⭐ **First-ever measurement: the `TASK-1071` scale substitution did NOT return.** ACHIEVED `(640, 360, 260)`, not `(20, 20, 5)`.

## 3. THE DISCHARGE GREP — **NON-ZERO**, and the count is NOT the evidence

```
grep -cE "AFogVolume|Siege\.Fog\." <suite log>     = 8
grep -cE "AFogVolume|Siege\.Fog\." <console log>   = 11
grep -cE "AFogVolume|Siege\.Fog\." <build log>     = 0     (reader control)
```

🚨 **W8 — A NEW FINDING ABOUT THE INSTRUMENT ITSELF. THE BARE COUNT IS CONFOUNDED AND MUST NOT BE USED ALONE.**
**7 of the suite lane's 8 matches are a PRE-EXISTING TEST NAME**, `TheFogStateIsReadInExactlyOnePlaceAndTheSeamConsultsAFogVolume`, which contains the substring `AFogVolume`. Measured: `git grep -c` at **`20c1bea`** finds it in `SiegeFogClampTest.cpp` — i.e. **before `TASK-1173` existed**.
⇒ **A bare `grep -c` would have returned NON-ZERO even with the fog path never executing.** The count alone is a false-green instrument.
✅ **This report's own §0 criterion was correctly specified and it is what discriminates** — *"a non-zero count **with at least one quoted `LogGitClaudeUnrealTest:` line**"*. Filtered:
```
grep "LogGitClaudeUnrealTest:" <suite log> | grep -cE "AFogVolume|Siege\.Fog\."   = 1
```
**Use the filtered form in future. `SC-§113`'s original `0` is not comparable to a bare count.**

## 4. THE QUOTED LINES — PRODUCTION CODE EXECUTING, BOTH LANES

**Lane (a), suite** — `FogVolume.cpp:338` inside the **shipped** `FindOrSpawn`, exactly as §0 predicted:
```
[2026.09.09-08.28.30:126][884]LogGitClaudeUnrealTest: [FogVolume_0] AFogVolume spawned — the fog-state actor now exists for this match (FOG-§10.1: one state object).
[2026.09.09-08.28.30:127][884]LogGitClaudeUnrealTest: [FogVolume_0] Fog INTEGRITY FLOOR engaged — ACHIEVED 'r.VolumetricFog'=1, 'r.VolumetricFog.GridPixelSize'=16, 'r.VolumetricFog.GridSizeZ'=64, height fog volumetric=ON (view distance 6000) …
[2026.09.09-08.28.30:163][884]LogGitClaudeUnrealTest: [FogVolume_0] Fog VISUAL spawned: 'BP_SiegeFog_C_0' — ACHIEVED Z=7000, ACHIEVED scale (640, 360, 260) …
[2026.09.09-08.28.30:163][884]LogGitClaudeUnrealTest: [FogVolume_0] Fog raised for 300 s (refresh, never stack — J-F16); it lifts at world time 300.0.
[2026.09.09-08.28.30:173][884]LogGitClaudeUnrealTest: [FogVolume_0] Fog INTEGRITY FLOOR released — ACHIEVED 'r.VolumetricFog'=1, now owned by SetByScalability …
[2026.09.09-08.28.30:174][884]LogGitClaudeUnrealTest: [FogVolume_0] Fog VISUAL destroyed ('BP_SiegeFog_C_0') …
```

**Lane (b), console — THE POSITIVE CONTROL, fired verbatim:**
```
[2026.09.09-08.52.14:568][  0]LogGitClaudeUnrealTest: [Siege.Fog.Raise] AFogVolume::RaiseFog() executed on 'FogVolume_0' and returned TRUE. Fog is now UP. ⛔ This line is the POSITIVE CONTROL for `SC-§113` cl. 3(c): its presence proves the fog path RAN.
[2026.09.09-08.52.14:568][  0]LogGitClaudeUnrealTest: [Siege.Fog.Clear] AFogVolume::ResetFog() executed on 'FogVolume_0'. Fog is now DOWN and prevention is now DOWN. ⛔ Both deadlines are ZEROED, which is the match-reset door (FOG-§10.3) — not the BrightSun card.
[2026.09.09-08.52.14:568][  0]LogGitClaudeUnrealTest: [Siege.Fog.Status] 'FogVolume_0' — fog DOWN, prevention DOWN (0 s of prevention remain). Read from the shipped accessors (IsFogActive / IsFogPrevented / GetFogPreventionSecondsRemaining), never recomputed here.
```
⇒ **`SC-§113` is closed EVIDENTIALLY, not merely structurally.** All three commands ran; `Status` **read the state back** after `Clear` (`fog DOWN`), which is a resolved-state assertion, not a call tally (`SC-§104`).

## 5. 🚨🚨 W9 — THE DOCUMENTED INVOCATION FOR LANE (b) DOES NOT WORK. THE CODE IS FINE; THE RECIPE IS WRONG.

This is a **prose defect, not a code defect**, and it is the most dangerous thing this row produced, because **it fails SILENTLY**.

**As written in this report §0, in `handoffs/TASK-1173-programmer.md` §1(b)/§4, and on the board:**
```
-ExecCmds="Siege.Fog.Raise;Siege.Fog.Status;Quit"
```
**MEASURED RESULT: nothing ran.** The engine logged the whole string as ONE command name:
```
[2026.09.09-08.30.17:543][  0]Cmd: Siege.Fog.Raise;Siege.Fog.Status;Quit
```
⛔ No fog line. ⛔ **No `Command not recognized` either** — it fails with *no error and no warning*. ⛔ The process **never exited**; I killed it at ~10 minutes.

**ROOT CAUSE, from engine source — `-ExecCmds` SPLITS ON COMMA, NEVER ON SEMICOLON:**
`Engine/Source/Runtime/Engine/Private/ParseExecCommands.cpp:29`
```cpp
else if (CurrentChar == ',' && !bInQuotes)
```
⇒ The semicolon idiom works for the **suite** only because `Automation RunTests Siegebound;Quit` is ONE command whose `;` is split by the **`Automation` handler's own** argument parser. **It is not an `-ExecCmds` feature and it does not generalise** — which is precisely the assumption §1(b) of this report accepted ("reaches it by substituting the command list").

**SECOND DEFECT — `Quit` DOES NOT QUIT AN EDITOR COMMANDLET.** `QUIT`/`EXIT` is handled by `UGameEngine::Exec` (`GameEngine.cpp:1527`); a commandlet runs `UUnrealEdEngine`, which handles **`QUIT_EDITOR`** (`EditorServer.cpp:5993`). With the comma form + `Quit`, all three fog commands ran correctly but the process **still hung** — I killed it at ~9 min, after watching the fog expire naturally at +300 s.

✅ **THE WORKING RECIPE, MEASURED — terminated on its own in 12 seconds, exit 0:**
```
UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<log>
```
⇒ **COMMAS, and `QUIT_EDITOR`.** Routed to the manager as a prose-fix row. ⛔ I did **not** edit the programmer's handoff (`SC-§53` cl. 3 — a struck line is named, not quietly overwritten).

## 6. W5 — CONFIRMED ON THE MACHINE, NOT JUST ON THE PAGE

§3's W5 predicted `ExecClearFog` prints a spawn claim it cannot perform. **Measured:**
```
LogGitClaudeUnrealTest: Warning: [Siege.Fog.Clear] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('L_Arena') … The fog-state actor is spawned into the map you currently have OPEN, which ⛔ MARKS IT DIRTY.
```
It fired on the **Clear** path, which uses `Find` and spawned nothing. W5 is upgraded from a code reading to a **measurement**.

## 7. FENCES

`git diff --numstat 20c1bea <worktree>` ⇒ **`298/0` · `75/0` · `313/0` = +686 / −0.** ⛔ **Zero deletions — the author's claim holds.**
⚠️ **Note on §9 cl. 3's command:** `git diff --numstat 20c1bea HEAD` returns **EMPTY**, because `HEAD` (`74baab3`) *is* the reverted state and the 686 lines are uncommitted in the worktree. Comparing `20c1bea` to the **worktree** is what tests the claim. Two negative controls (mis-anchored path; nonexistent file) both returned **silence at exit 0** (`SC-§102`).

`L_Arena.umap` `1f78419d…0af15622` — **UNMOVED at 3 checkpoints**, never saved (the console lane dirtied it in memory and the process was killed, discarding it).
`Content/**` · `Config/**` · `Tools/**` · `Content/FogArea/**` · `testvideo/**` — **all clean, none staged.** Index empty before staging. **NEVER pushed.**
**LFS: no subject** — every staged path is text (`git check-attr filter` ⇒ `unspecified`). No binary in this commit, so the oid-vs-sha256 check has nothing to run against; stated rather than claimed.
