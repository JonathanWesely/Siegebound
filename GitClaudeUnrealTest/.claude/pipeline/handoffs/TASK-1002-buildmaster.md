# TASK-1002 — DIAGNOSTIC COMPILE — build-master handoff

**Date:** 2026-09-04
**Agent:** build-master
**Status:** ✅ **done**
**Kind:** ⛔ **DIAGNOSTIC ONLY — evidence, never authorization.**

---

## ⭐⭐ THE HEADLINE

**The six never-together-compiled diffs BUILD. Zero errors, zero warnings.**

⛔⛔ **AND THE SHARPEST RESULT: `TASK-979`'s `SummonedUnit.cpp` COMPILED — step `[23/26]`, clean.**
Its `qa-passed` was a **READ**; the byte-identical recovery proved **TEXT**. As of this build it is
proved **COMPILABLE**. The restored diff is not merely the right bytes — it is the right *program*.

---

## ⛔ THE LITERAL `Result:` LINE (⛔ NOT an exit code)

```
Result: Succeeded
```

- Found at **line 51** of the build log.
- ⛔ `Build.bat` also returned **exit 0** — **that is NOT the evidence and was NOT relied upon.**
  Per the recorded `Build.bat`-exit-code law, exit 0 is emitted on FAILED builds too (Live Coding
  mutex). **The parsed log line is the finding.**

### Diagnostic count

| Signal | Count |
|---|---|
| `error C` (compiler) | **0** |
| `error LNK` (linker) | **0** |
| `: error` (any) | **0** |
| `: warning` (any) | **0** |
| `0x800711C7` (Smart App Control) | **0** |

**Total: 0 diagnostics.** Nothing to route. No lane implicated.

---

## ⛔ THE THREE KNOWN LIES — ALL THREE CHECKED, NONE FIRED

1. **Exit-code lie** — ⛔ **not trusted.** Parsed `Result: Succeeded` from the log instead.
2. **Editor-open lie (DLL cannot be written)** — ⛔ **ruled out by a timestamp, not by assumption.**
   `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` written **2026-09-04 03:32:54**, checked at
   **03:33:15** — **21 seconds old.** A stale/cached DLL would have carried an old date.
   Steps `[24/26] Link .lib` and `[25/26] Link .dll` both ran. **The link genuinely happened.**
3. **Smart App Control** — ⛔ **did not fire, and was pre-checked.**
   `HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy\VerifiedAndReputablePolicyState = 0` (**Off**).
   Build ran **24.11 s**, not the ~2 s SAC death. Zero `0x800711C7`.

---

## ⛔ WHAT ACTUALLY GOT COMPILED — the six lanes were genuinely in this build

Adaptive-unity **excluded** exactly the dirty files from the unity blob, which is the log's own proof
that every lane's source was compiled **as an individual translation unit**:

```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file:
  SiegeCombatStatics.cpp, SiegeFogStatics.cpp, SpellLibrary.cpp, SummonedUnit.cpp,
  SiegeCardArtRosterTest.cpp, SiegeCardRosterTest.cpp, SiegeFogClampTest.cpp,
  SiegeFogTest.cpp, SiegeUnitNoticeRangeTest.cpp
```

| Lane | Files | Result |
|---|---|---|
| **`TASK-979`** (RESTORED tonight) | `SummonedUnit.{h,cpp}` | ✅ `[23/26]` clean — ⭐ **highest-signal pass** |
| **`TASK-981`** | `SiegeFogStatics.{h,cpp}`, `Tests/SiegeFogTest.cpp` | ✅ `[13/26]`, `[18/26]` clean |
| **`TASK-839`** | `SiegeCombatStatics.cpp`, `SpellLibrary.cpp`, `CardRow.h` | ✅ `[7/26]`, `[22/26]` clean |
| **`TASK-993`** | `CardRow.h` | ✅ clean (header — consumed by the above TUs) |
| **untracked tests** | `SiegeUnitNoticeRangeTest.cpp`, `SiegeCardArtRosterTest.cpp`, `SiegeCardRosterTest.cpp` | ✅ `[16/26]`, `[9/26]`, `[8/26]` clean |

Also linked: `UnrealEditor-GitClaudeUnrealTest.lib` + `.dll`, `WriteMetadata` target.
UBT reported **`Invalidating makefile … (source file added)`** — it saw the new untracked test files
and rebuilt the makefile, so they were not silently skipped.

---

## ⭐ `UnitEngagementRadiusUU` — ⛔ **RESOLVED. THE RECOVERY IS COMPLETE.**

**Both dependent test files compiled clean**, which is the only instrument that can answer this:

- `Tests/SiegeUnitNoticeRangeTest.cpp` → ✅ `[16/26]` clean (references the symbol at **13 sites**)
- `Tests/SiegeFogClampTest.cpp` → ✅ `[19/26]` clean

⛔ **This is the falsifiable form:** an incomplete recovery would have produced `C2039`/`C2065`
undeclared-identifier errors in those two TUs. **Zero appeared.**
Declaration confirmed at `SummonedUnit.h:852` — `static constexpr float UnitEngagementRadiusUU = 2000.f;`

⛔ **No test file was edited.** The tests were intact and correct throughout; the source was what was
lost and restored, and the source is what the compiler has now vindicated.

---

## ⚠️ THE `C2248` ACCESS HAZARD — ⛔ DID NOT MATERIALISE

The carried-over hazard (a pattern legal in `SiegeBuildingStackTest.cpp` only because that class's
member is `public`, copied where members are `protected`) **produced no `C2248`.**

Static access map of `SummonedUnit.h` corroborating the compiler:

- `public:` @ **200** → runs to **1128**
- `protected:` @ **1129** → runs to **1723**
- `private:` @ **1724**

Every symbol the tests reach for lands inside the **public** block:

| Symbol | Line | Region |
|---|---|---|
| `UnitEngagementRadiusUU` | 852 | ✅ public |
| `ResolveNoticeRadiusUU` | 882 | ✅ public |
| `ResolveEffectiveLeashRangeUU` | 933 | ✅ public |
| `GetEngagementRadiusUU()` | 945 | ✅ public |
| `GetLeashRangeFloorUU()` | 961 | ✅ public |
| `GetClassDefaultEngagementRadiusUU()` | 981 | ✅ public |
| `AggroRadius` | 1258 | 🔒 **protected** — ⛔ and the tests never touch it directly |

⭐ The one apparent exception is not one: `SiegeUnitNoticeRangeTest.cpp:623` names
`"float AggroRadius = UnitEngagementRadiusUU;"` inside `CountOccurrencesInCode(UnitHeader, …)` —
that is a **text scan of the header read off disk**, not a member access. **QA's 3-access-specifier
verification is now compiler-confirmed.**

---

## ⛔⛔ WHAT THIS BUILD DOES **NOT** ESTABLISH

- ⛔ **It did NOT spend `TASK-987`'s compile.** `QUIET-MODULE`'s "one compile" is the **COMMIT-GATE**
  compile — the one that *authorises a commit*. **This build committed nothing, so it cannot have
  spent that gate. `TASK-987` still runs its own compile.**
- ⛔ **NO SUITE WAS RUN.** Per `TL-§5c`, the count remains **445 declared / 34 declared** —
  ⛔ **declared, never executed.** No `Result={Success}`/`Result={Fail}` pair was produced, so no
  executed number may be written by anyone citing this row.
- ⛔ **A clean compile is not a passing test.** It falsifies "these six diffs don't fit together."
  It says nothing about whether they *behave* correctly. `TASK-987` remains the real gate.
- ⛔ **`TASK-1001` is still correctly blocked.** This compile makes `NoticeRange` exist in the
  editor's schema, but the `DT_Cards` write remains `TASK-987`-gated as boarded.

---

## Editor

| | |
|---|---|
| Found running | **PID 16020** (up since 00:13) |
| Action | ⛔ **Closed by name** under Jonathan's standing grant (2026-08-30). He is asleep; he was **not** asked. |
| Save prompts | ⛔ **None accepted.** Force-kill discards unsaved state — correct under the never-save law. |
| `L_Arena` | ⛔ **NEVER saved.** Pinned hash intact. |
| `Docs/Data/DT_Cards.uasset` | ✅ Already persisted to disk earlier tonight by an explicit one-asset save — safe to discard from memory, and was. |
| Reopened | ✅ Yes — build succeeded and was quick (24 s) |
| **New PID** | **38852** |

⚠️ **Flag for the orchestrator:** the editor is now **UP**, but **`TASK-987`'s compile needs it
CLOSED again.** Whoever takes `TASK-987` must re-close it (they hold the same standing grant).
Reopening was done because the success condition was met; the re-close is routine, not a defect.

---

## ⛔ GIT — ZERO COMMANDS RUN

⛔ **I ran no Git command of any kind: no `add`, `commit`, `checkout`, `restore`, `stash`, `reset`,
`clean` — not even `git status`.** Nothing was staged, committed, reverted or discarded. All six
uncommitted diffs remain exactly as I found them. (`SC-§71` honoured.)

⚠️ **One honest disclosure, because it appears in the log and would otherwise look like a violation:**
line 6 reads

```
Using 'git status' to determine working set for adaptive non-unity build (C:\GitProjects\GitHub\GitClaudeUnrealTesting).
```

**That is UnrealBuildTool's own internal, read-only invocation** for adaptive-unity working-set
detection — ⛔ **not a command I issued**, and read-only in any case (it is precisely how UBT knew to
pull the nine dirty files out of the unity blob). I am recording it rather than letting a future
reader find it in the log and mistake it for my doing.

---

## Artefacts

- Build log: `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\task1002_build.log`
- UBT log: `C:\Users\wesel\AppData\Local\UnrealBuildTool\Log.txt`
- Output DLL: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Binaries\Win64\UnrealEditor-GitClaudeUnrealTest.dll` (8,363,520 bytes, 2026-09-04 03:32:54)

## Follow-ups for the manager

1. ⭐ **`TASK-979`'s recovery is now compiler-verified**, not merely byte-verified. Its `qa-passed`
   can be upgraded from "a READ" to "a READ + a BUILD."
2. ⛔ **`TASK-987` must still run its own compile** — this one was diagnostic and authorises nothing.
3. ⚠️ The editor is up (**38852**) and will need closing for `TASK-987`.
4. ⛔ **445 / 34 remain DECLARED.** Nothing executed this wave.
