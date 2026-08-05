# TASK-529 — `FSiegeNavDiagnostics`, the nav telemetry library (Stage 0) — gameplay-programmer handoff

**Status:** `ready-for-qa` · **QA gate: TASK-537** (`SC-§29`) · **Compile gate: TASK-538** (⛔ I did not compile — quiet-module law)

> ⚖️ **M8 DECLARATION, VERBATIM:** *"adds no replicated property, no new replicated class, no new relevancy tier."*
> Structural reason (`NAV-§11`): this is a static library with **no instances and no reflected symbol of any kind** — no `UCLASS`, no `USTRUCT`, no `UENUM`, no `UPROPERTY`, no `UFUNCTION`. There is nothing here a replication graph could ever see, and no later refactor can accidentally make one. The declaration is also present verbatim in `SiegeNavDiagnostics.h`.

---

## 1. Files touched — ⛔ EXACTLY TWO, BOTH NEW, ZERO CALL SITES

| file | state |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeNavDiagnostics.h` | **NEW** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeNavDiagnostics.cpp` | **NEW** |

**Nothing else moved.** ⛔ No `Build.cs` change · ⛔ no `.umap` write (`NAV-§5`) · ⛔ no `.ini` change · ⛔ no call sites (TASK-535 owns those) · ⛔ no compile, no Git, no editor/MCP/PIE.

- **`Build.cs` verified, not edited:** `AIModule` (`GitClaudeUnrealTest.Build.cs:17`) and `NavigationSystem` (`:18`) are already public dependencies, and `Core`/`CoreUObject`/`Engine` cover every other include. I read the file to confirm and **did not touch it** — `NAV-§8` makes a `Build.cs` edit in this batch a FINDING. **In the event: the `.cpp` needs no AIModule header at all** — every symbol comes from `NavigationSystem` and `Engine`.
- **The behaviour-free claim is provable in one look:** the diff is a new file pair and the module contains **zero references** to either symbol. Grep `FSiegeNavDiagnostics` across `Source/` — the only hits are the two new files themselves.

---

## 2. ⭐ THE PINNED SIGNATURES, AS WRITTEN — CHARACTER-FOR-CHARACTER AGAINST `NAV-§8`

Registry version checked against: **CONVENTIONS `NAV-§8`, `SiegeNavDiagnostics.h` block, CONVENTIONS.md:2601-2612** (the 2026-08-04 authoring; the pin has not moved since).

```cpp
// ── SiegeNavDiagnostics.h ─────────────────────────────────────────────────
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeNavDiag, Log, All);

class GITCLAUDEUNREALTEST_API FSiegeNavDiagnostics
{
public:
    /** The RUNNING actor's config — this is the line that settles NAV-§9's conditional. */
    static void LogNavConfigOnce(const UWorld* World);

    /** Queue depth + pool headroom. Tag ∈ "pre-scatter" | "post-scatter" | "at-confirmation". */
    static void LogNavBuildSnapshot(const UWorld* World, const TCHAR* Tag);
};
```

**Conformance, item by item — this is what TASK-537 should diff:**

| pinned element | as shipped |
|---|---|
| category name | `LogSiegeNavDiag` ⛔ **not** `LogSiegeNav` (`LogSiegeNet` collision, `NAV-§7` ruling 7b) — the header carries the refusal in a comment so a later "simplification" is answered in place |
| category macro | `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeNavDiag, Log, All);` in the `.h`, `DEFINE_LOG_CATEGORY(LogSiegeNavDiag);` in the `.cpp` |
| class | `class GITCLAUDEUNREALTEST_API FSiegeNavDiagnostics` — `F`-prefixed, **not a UObject**, no base class |
| access | both statics under a single `public:` — **access level is part of the pin** |
| member count | **exactly two**, both `static void`, no third entry point, no data members |
| param types | `const UWorld*` and `(const UWorld*, const TCHAR*)`, `Tag` unnamed-vs-named matches the pin (`Tag`) |

⚠️ **`const UWorld*` is load-bearing and it forced one engine-API choice** — see §5(b).

---

## 3. ⭐ THE EXACT LOG-LINE FORMATS — TASK-535 AND THE QA GATE BOTH GREP THESE

Both lines are emitted at **`Log`** verbosity (the category's default), so they appear in an ordinary PIE log with no verbosity flags. ⛔ Not `Verbose` — a `Verbose` evidence line would be invisible at the gate, which is the whole point of the batch.

### (a) `LogNavConfigOnce` — ONE line, once per world

**Format string, verbatim from `SiegeNavDiagnostics.cpp`:**
```
nav-config: actor='%s' gatherOnGameThread=%s maxTileJobs=%d cellSize=%.2f tileSizeUU=%.2f agentRadius=%.2f poolCap=%d fixedPool=%s runtimeGen=%s
```

**Worked example (what the log should read if TASK-530's ini flip LANDED):**
```
LogSiegeNavDiag: nav-config: actor='RecastNavMesh-Default' gatherOnGameThread=true maxTileJobs=8 cellSize=32.00 tileSizeUU=2000.00 agentRadius=34.00 poolCap=1024 fixedPool=true runtimeGen=Dynamic
```

| token | source (all off the **RUNNING** actor) |
|---|---|
| `gatherOnGameThread=` | `ARecastNavMesh::ShouldGatherDataOnGameThread()` (`RecastNavMesh.h:1499`) |
| `maxTileJobs=` | `GetMaxSimultaneousTileGenerationJobsCount()` (`:1280`) |
| `cellSize=` | `GetCellSize(ENavigationDataResolution::Default)` (`:1107`) — ⚠️ **not** the member, see §5(a) |
| `tileSizeUU=` | `ARecastNavMesh::TileSizeUU` (`:691`, public) |
| `agentRadius=` | `ARecastNavMesh::AgentRadius` (`:730`, public, not deprecated) |
| `poolCap=` | `ARecastNavMesh::TilePoolSize` (`:687`, public) — **the CONFIGURED pool** |
| `fixedPool=` | `ARecastNavMesh::bFixedTilePoolSize` (`:683`, bitfield, read as `!= 0`) |
| `runtimeGen=` | `GetRuntimeGenerationMode()` (`NavigationData.h:658`) → `Static` / `DynamicModifiersOnly` / `Dynamic` / `LegacyGeneration` / `Unknown` |

> ### ⛔ **`true` AND `false` ARE LOWERCASE ON PURPOSE AND THAT IS NOT A STYLE CHOICE.**
> `NAV-§9` clause 3 / TASK-540 branch A is worded against the **literal string** `gatherOnGameThread=false`. The casing is part of the evidence contract, so the `.cpp` emits it from an explicit `TEXT("true") : TEXT("false")` helper rather than any `%d`, any `LexToString`, or any `%s` on a bool. **The gate's revert test is `grep "gatherOnGameThread=false"` and it works verbatim.**

**Reading the result (`NAV-§9` clause 3):**
- `gatherOnGameThread=true` ⇒ the ini **REACHED** the serialized `L_Arena` nav actor. Stage 1 stands.
- `gatherOnGameThread=false` ⇒ the ini did **NOT** reach it. ⛔ **REVERT both keys (TASK-540 branch A).** ⛔ Not an `L_Arena` save (`NAV-§5`), not "one more thing first".

### (b) `LogNavBuildSnapshot` — ONE line per call, three calls per match

**Format string, verbatim:**
```
nav-build [%s]: remaining=%d running=%d dirtyAreas=%d hasDirty=%s activeTiles=%d poolCap=%d
```

**Worked example:**
```
LogSiegeNavDiag: nav-build [at-confirmation]: remaining=302 running=1 dirtyAreas=0 hasDirty=false activeTiles=24 poolCap=1024
```

| token | source |
|---|---|
| `[<tag>]` | the caller's `Tag`, ∈ `pre-scatter` \| `post-scatter` \| `at-confirmation` |
| `remaining=` | `UNavigationSystemV1::GetNumRemainingBuildTasks()` (`NavigationSystem.h:1106`) |
| `running=` | `GetNumRunningBuildTasks()` (`:1109`) |
| `dirtyAreas=` | `GetNumDirtyAreas()` (`:977`) |
| `hasDirty=` | `HasDirtyAreasQueued()` (`:976`), lowercase `true`/`false` |
| `activeTiles=` | `ARecastNavMesh::GetNumActiveTiles()` (`RecastNavMesh.h:1170`) — tiles actually holding data |
| `poolCap=` | `ARecastNavMesh::GetNavMeshTilesCount()` (`:1168`) — **the RUNTIME allocated pool** |

**⚠️ TAG FORMAT NOTE FOR TASK-535 AND THE GATE:** the tag is emitted **bare inside square brackets**, e.g. `[post-scatter]` — there is deliberately **no `tag=` token**, because `NAV-§7` pins the three tag *values* but does **not** pin a `tag=` key the way it pins every other token. `grep "post-scatter"` and `grep "at-confirmation"` both work verbatim.

---

## 4. ⚠️ THREE THINGS QA SHOULD SCRUTINISE — I AM FLAGGING THEM RATHER THAN LETTING THEM BE FOUND

### (a) ⭐ THE TWO `poolCap=` VALUES COME FROM DIFFERENT SOURCES. THIS IS DELIBERATE AND IT IS THE `NAV-§6` INSTRUMENT.

- config line `poolCap=` → `ARecastNavMesh::TilePoolSize` — **the number in the ini, as the actor carries it.**
- snapshot line `poolCap=` → `GetNavMeshTilesCount()` — **the pool the navmesh actually allocated**, per the engine's own comment at `RecastNavMesh.h:1169` (*"...rather than pool capacity"*, i.e. `GetNavMeshTilesCount` **is** the pool capacity).

With `bFixedTilePoolSize=True` the two **should agree**, and **if they ever diverge, the divergence is itself the `NAV-§6` signal** — which is strictly more information than printing one number twice. Both are documented in the header. ⛔ **Do not "fix" this by collapsing them to one source** — that would delete the instrument.
`NAV-§6` compliance otherwise: ⛔ `TilePoolSize` is **read and never written**; this task changes no nav tuning value of any kind.

### (b) ⚠️ A PARTIAL SNAPSHOT STILL PRINTS, WITH `-1` AS AN EXPLICIT "UNAVAILABLE" SENTINEL.

If the nav system is up but no `ARecastNavMesh` resolves, `LogNavBuildSnapshot` still emits the full line with `activeTiles=-1 poolCap=-1`. **Rationale:** dropping the whole line would throw away `remaining=`/`running=`/`dirtyAreas=`/`hasDirty=`, which are the half of that line the gate needs most (they are what decompose the `BattlefieldScatter.cpp:1953` `IsNavigationBeingBuilt` composite — `NAV-§12`'s *"which of the two returned false is UNKNOWN"* limitation is exactly what this closes). `-1` can never be a real count, so the gate can tell **unavailable** from **zero** — and for the tile-pool hazard those are opposite readings. Named as `SiegeNavDiag_Unavailable`, not a bare literal.

### (c) ⚠️ THE ONE PIECE OF STATE IN THE FILE — THE PER-WORLD LATCH — AND WHY IT DOES NOT BREAK "NO STATE".

The spec is in apparent tension with itself and I want that on the record rather than resolved silently: item **(2)** says *"No state. No members."*; item **(3)** says *"'Once' = once per world; a static bool is NOT acceptable across PIE sessions — key it off the world or a member of the nav data, and say in the handoff how you did it."* Item (3) **explicitly contemplates** a world-keyed latch, and "once per world" is unimplementable without one.

**How I did it:** a **`.cpp`-local, function-scoped `static TSet<FObjectKey>`** keyed on the `UWorld`, reached through `GetConfigLoggedWorlds()` in an anonymous namespace.

- ✅ **It is not a class member** (the class has zero data members), not gameplay state, and **not a write to any engine object** — the Stage 0 guarantee (`NAV-§9` clause 1) is intact.
- ✅ **`FObjectKey` carries the object's serial number**, so a recycled `UObject` index resolves as a *different* key — the latch **cannot false-positive** into silence on a fresh PIE world. That is the precise defect a plain `static bool` has: it would fire on the first PIE session of an editor run and stay silent for every session after, i.e. the evidence would be missing **exactly when somebody re-ran PIE to check a fix**.
- ✅ **`FObjectKey` holds no GC reference**, so the latch cannot keep a dead world alive.
- ✅ **⚠️ THE LATCH IS TAKEN ONLY ON A LINE THAT WAS ACTUALLY EMITTED.** If the nav system or nav actor is not up yet, the call logs one `Verbose` miss line and leaves the world **UNLATCHED**, so a later call still gets its chance. A latch-on-entry would have burned the world's one shot on a null.
- ✅ **Bounded:** one entry per PIE session; above 8 entries the set is rebuilt keeping only keys that still resolve. Rebuilt via range-for rather than `RemoveCurrent()` because **in UE 5.8 `TSet` is a define-injected alias over `TCompactSet` or `TSparseSet`** (`Containers/Set.h`) depending on `UE_USE_COMPACT_SET_AS_DEFAULT`, and a range-for depends on nothing that differs between them.
- ✅ **Game-thread guard:** the latch is an unsynchronised shared static, so a call from any other thread logs one `Verbose` line and returns without touching it. Every wired call site (TASK-535) is game-thread gameplay code.

---

## 5. ⚠️ TWO ENGINE-API DEVIATIONS FROM THE LITERAL SPEC WORDING — BOTH FORCED, BOTH VERIFIED FIRST-HAND

Neither touches a pinned signature or a pinned token. Both are recorded here because a reviewer diffing the spec text against the code **will** notice them.

### (a) ⭐ `cellSize=` COMES FROM `GetCellSize(ENavigationDataResolution::Default)`, NOT FROM `ARecastNavMesh::CellSize`. THE MEMBER WOULD FAIL THE BUILD.

The spec (TASKBOARD `:6997`, and the dispatch prompt) says *"the live `CellSize` … off the running actor."* **In UE 5.8 that member cannot be read from our code:**

```cpp
// Engine/Source/Runtime/NavigationSystem/Public/NavMesh/RecastNavMesh.h:710-712
UE_DEPRECATED(all, "Use NavMeshResolutionParams to set CellSize for the different resolutions instead")
UPROPERTY(config, meta = (DeprecatedProperty, DeprecationMessage = "..."))
float CellSize;
```

- `UE_DEPRECATED(all, …)` is a **C++** deprecation, not merely reflection metadata ⇒ reading it emits **C4996**, and **warnings are errors in this build** (`NAV-§8`'s own note). It would have broken TASK-538's compile.
- It is also **not the live value**: the migration that copies it into `NavMeshResolutionParams` is `WITH_EDITORONLY_DATA`-gated and version-gated (`RecastNavMesh.cpp:679-686`).
- ⭐ **The project already documents this in its own ini:** `Config/DefaultEngine.ini:271-272` — *"Default-resolution cells 19x10 -> 32x20 via NavMeshResolutionParams[1] (UE5: index 1 = Default resolution; **the loose CellSize/CellHeight keys are deprecated**)"*, and `:331` sets `NavMeshResolutionParams[1]=(CellSize=32.0,...)`.

⇒ **`GetCellSize(ENavigationDataResolution::Default)` (`RecastNavMesh.h:1107`) is the live value and the only compiling read.** The `cellSize=` token is unchanged. **I believe the spec should be corrected here** — see §7.

### (b) THE NAV SYSTEM IS FETCHED VIA `FNavigationSystem::GetCurrent<UNavigationSystemV1>(const UWorld*)`, NOT `UNavigationSystemV1::GetCurrent`.

The pinned signature takes **`const UWorld*`**. `UNavigationSystemV1::GetCurrent` has only non-const overloads (`NavigationSystem.h:1177-1178`: `UWorld*` and `UObject*`), so it is unreachable from a `const UWorld*` **without a `const_cast`** — and a `const_cast` in a library whose entire selling point is "provably read-only" would be a bad look at a gate that exists to check exactly that.

`FNavigationSystem::GetCurrent<TNavSys>(const UWorld* World)` (`Engine/Classes/AI/NavigationSystemBase.h:121`) is the engine's own **const** overload, returns `const TNavSys*`, and overload resolution is unambiguous with an explicit template argument. **Every accessor this library calls is `const`**, so nothing is lost. (This is the same nav system the rest of the project reaches via `UNavigationSystemV1::GetCurrent` — `BattlefieldScatter.cpp:1990`, `SummonedUnit.cpp:2500` — just reached const-correctly.)

### (c) Nav-actor resolution has a documented fallback.

`SiegeNavDiag_ResolveRecastNavMesh` tries `NavSys->GetDefaultNavDataInstance()` first (the instance the **default agent actually paths on** — the one whose config governs the units that wedge), then falls back to `TActorIterator<ARecastNavMesh>`. **The fallback is not defensive padding:** route 1 returns null when registration **failed**, and `RegistrationFailed_AgentNotValid` is a *named risk of this very batch* (`NAV-§2(a)`). A serialized nav actor that is present but **unregistered** is a diagnosis worth printing, not a reason to print nothing. ⛔ Both routes yield a live world actor — **never the CDO.**

---

## 6. COMPLIANCE CHECKLIST AGAINST `NAV-§10` (the criteria that apply to this task)

| # | criterion | this task |
|---|---|---|
| 6 | no `.umap` write · no `Build.cs` change · no `TilePoolSize` change · no `AgentRadius` change · no crowd/RVO | ✅ none — two new source files and nothing else; `TilePoolSize`/`AgentRadius` are **read** only |
| 7 | telemetry is READ-ONLY — no state, no ticking, no behaviour change; every engine accessor null-guarded | ✅ see §4(c) for the latch reconciliation; **every** accessor guarded — `World`, nav system, nav data, the `ARecastNavMesh` cast, and `Tag` |
| 8 | pinned-registry conformance, character-for-character, **including access levels** | ✅ §2 |
| 9 | M8 declaration present VERBATIM in the handoff **and in the header** | ✅ top of this file and `SiegeNavDiagnostics.h` |
| 10 | complete-type include law · most-vexing-parse · no shadowing of inherited reflected members | ✅ see below |

- ⛔ **Never `check()`, never `ensure()`, never `verify()`** — grep the `.cpp`, there are none. Every failure path is one `Verbose` line and a `return`.
- ⛔ **No per-tick logging, no default-verbosity spam** (`TASKBOARD.md:10254`): the whole library emits **at most 4 `Log` lines per match** (1 config + 3 snapshots). Every miss/guard path is `Verbose`.
- **Complete-type include law:** the header forward-declares `class UWorld;` (pointer-only — the `SiegeCombatStatics.h` precedent) and the `.cpp` includes the real headers explicitly, each with a comment saying which symbol it is for: `NavigationSystem.h`, `NavigationData.h`, `NavMesh/RecastNavMesh.h`, `AI/Navigation/NavigationDataResolution.h`, `AI/NavigationSystemBase.h`, `Engine/World.h`, `EngineUtils.h`, `UObject/ObjectKey.h`, `CoreGlobals.h`. **Nothing is relied on transitively.**
- **Most-vexing-parse:** the two parenthesised initialisations (`FObjectKey WorldKey(World);`, `TActorIterator<ARecastNavMesh> It(World);`) both take a **variable**, never a type, so neither can parse as a function declaration.
- **Shadowing (C4457/C4458):** not reachable — no base class, no reflected members, no data members at all.
- **`SC-§13` / `TestEqualSensitive`:** no automation test in this task (tests are `SiegeStuckStaticsTest.cpp`, `NAV-§7`, another task's file) and no `FString` claim is asserted anywhere.
- **Non-ASCII in `TEXT()` literals** is consistent with existing shipped code (`Barracks.cpp:61`, `BattlefieldScatter.cpp:217`, ~29 files) — no new encoding risk introduced.

---

## 7. ⚠️ ONE THING IN THE SPEC I BELIEVE IS WRONG — RAISED, NOT SILENTLY PATCHED (`SC-§15`)

**TASKBOARD `:6997` and CONVENTIONS `NAV-§8`'s prose both say to read "the live `CellSize`" off the running actor. That member is `UE_DEPRECATED(all)` in UE 5.8 and reading it would have failed the compile gate** (warnings-as-errors), quite apart from not being the live value. Detail and citations in §5(a).

- **It does not affect any pinned signature or any pinned token** — `cellSize=` still prints, and prints the *correct* number (`32.00`), which the deprecated member would not have been guaranteed to.
- **Suggested CONVENTIONS amendment for the manager** (I did not edit CONVENTIONS — not my file): in `NAV-§8`'s `SiegeNavDiagnostics.h` block or `NAV-§7`'s token list, note that `cellSize=` is sourced from **`ARecastNavMesh::GetCellSize(ENavigationDataResolution::Default)`**, because `ARecastNavMesh::CellSize` is `UE_DEPRECATED(all)` in UE 5.8 and the live value lives in `NavMeshResolutionParams[Default]` — as `Config/DefaultEngine.ini:271-272` already says in its own comment.
- ⚠️ **This is worth landing in the law because the next reader of `NAV-§8` will reach for the member**, and the failure mode is a broken compile gate rather than a wrong number, which costs a whole QA loop.

Nothing else in the spec looked wrong to me. `NAV-§8`'s two pinned signatures compiled against cleanly as written and I changed nothing about them.

---

## 8. FOR TASK-535 (the call-site owner) — WHAT TO CALL AND WHERE

Not my file and I added nothing to it; this is the contract summary so 535 does not have to re-derive it.

```cpp
#include "Siegebound/SiegeNavDiagnostics.h"

FSiegeNavDiagnostics::LogNavConfigOnce(World);                          // once; pairs naturally with the pre-scatter snapshot
FSiegeNavDiagnostics::LogNavBuildSnapshot(World, TEXT("pre-scatter"));
FSiegeNavDiagnostics::LogNavBuildSnapshot(World, TEXT("post-scatter"));
FSiegeNavDiagnostics::LogNavBuildSnapshot(World, TEXT("at-confirmation"));
```

- **A null `World` is safe** at every call site — no guard needed on 535's side.
- **`LogNavConfigOnce` is idempotent per world**, so calling it from a function that can run more than once is harmless.
- ⛔ **The three tag strings are pinned** (`NAV-§7`) — pass them exactly, lowercase, hyphenated.
- ⛔ **Do not call either function from a tick or a poll.** Three snapshots per match is the designed budget.

---

## 9. WHAT I DID **NOT** DO

- ⛔ **Did not compile** — TASK-538 owns the only compile (quiet-module law, one UBT module).
- ⛔ **Did not touch Git** — build-master's job.
- ⛔ **Did not open the editor, MCP, or PIE** — this task is file-only.
- ⛔ **Did not add a call site**, did not edit `BattlefieldScatter.{h,cpp}` (TASK-535's exclusive file), did not edit `Config/DefaultEngine.ini` (TASK-530's exclusive file), did not edit `CONVENTIONS.md`.
- ⛔ **Did not mark this task passing** — status is `ready-for-qa`; TASK-537 gates it.
- ℹ️ **Board edit:** I set `status:` straight to `ready-for-qa` in a single edit rather than writing `in-progress` and then `ready-for-qa`. `TASKBOARD.md` is a hot shared file with a known write-race history, and one edit halves the exposure for an identical end state.
