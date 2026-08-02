# TASK-420 — [LLM-INT1] Compile GREEN + grammar automation tests + machine verification of the game-lane Wave 0

**Agent:** build-master · **Date:** 2026-08-02 15:37–15:41 · **Lane:** LLM-ASSISTANT game-lane Wave 0 (TASK-416 · 417 · 418 · 426)
**Verdict: ✅ COMPILE GREEN · 11/11 TESTS PASS · BLOCKER-1 CONFIRMED CLOSED BY MACHINE (headless, not PIE) · WARN-3 CONFIRMED LIVE**
**NO Git. NO push. Nothing staged. `L_Arena` never opened, never saved.**

---

## 0. QUIESCE — the module was verifiably quiet

| Check | Result |
|---|---|
| Newest source write in `Source/` | **15:21:59** (`SiegeAssistantVocabulary.h`) |
| Build started | **15:36:50** — a **14 m 51 s** quiet window |
| `▶ NOW` programmer tasks in the module | none — TASK-416/417/418 all **`qa-passed`** (finished + handoffs written), TASK-426 `ready-for-integration` (file-only, no C++) |
| Other compile gates | none — **TASK-401 closed `done` at 15:02**; TASK-403 / 422 / 425 all `backlog` |
| Build / editor processes at start | **none running** (`UnrealEditor`, `UnrealBuildTool`, `MSBuild`, `cl`, `link` — all absent) |
| Source snapshot before vs after the whole gate | **186 files, ZERO differences** in size or mtime ⇒ no foreign write during the gate, and **I wrote no source file** |

**This gate was not a no-op re-run of TASK-401's green.** TASK-401's RUN #3 (15:02) predates four lane edits — `SiegeAssistantVocabulary.cpp` 15:20:48, `SiegeAssistantGrammarTest.cpp` 15:21:16, `SiegeAssistantSnapshot.h` 15:21:51, `SiegeAssistantVocabulary.h` 15:21:59 — i.e. **TASK-417's loop-2 fix (the `militiamob` blocker close + the two new guards) had never been through a compiler until now.** UBT recompiled exactly those TUs.

---

## 1. THE BUILD — `Result: Succeeded`

```
Result: Succeeded
Total execution time: 10.96 seconds
```

Judged **on log text, not exit code** (standing law). `grep -inE "error|warning|unresolved|LNK|fatal|failed"` over the whole log returns **ZERO rows** — not one diagnostic of any kind, no unresolved externals, no warnings.

Actions run (7): compile `SiegeAssistantGrammarTest.cpp` · `SiegeAssistantVocabulary.cpp` · `SiegeAssistantSnapshot.cpp` · `Module.GitClaudeUnrealTest.1.cpp`, then **Link `.lib` + Link `.dll`**, then WriteMetadata. **The link was reached and succeeded** — every symbol across the lane resolves.

**Binary:** `UnrealEditor-GitClaudeUnrealTest.dll` → **3,283,456 B @ 2026-08-02 15:37:01** (was 3,281,920 B @ 15:02:23). It postdates every lane source, so the shipped binary contains the lane's final state.

**Attribution:** nothing to attribute. Zero diagnostics means no file — this lane's or any other's — emitted one. The Follow lane's files (`SiegePlayerController.*`, `SummonedUnit.*`, `MinerUnit.*`, `Castle.*`, `GoldNode.*`) were compiled into the same module and were silent.

### The `SiegeLlama` hold-out — done and reversed byte-identically

`Plugins/SiegeLlama/` has **no `Binaries/`** — it has never been built, so leaving it enabled would have failed the editor launch, not just the build.

1. Backed up `GitClaudeUnrealTest.uproject` → sha256 `63058F3C…`, **859 B**.
2. Set the entry **explicitly `"Enabled": false`** — *not deleted*. The `.uplugin` carries `"EnabledByDefault": true` at line 17, so **deleting the entry would have left it enabled.** (TASK-401's trap, confirmed still true.)
3. Built. **`grep -icE "siegellama|llama"` over the build log = 0** ⇒ fully excluded.
4. **Restored** → sha256 `63058F3C…`, **859 B, `"Enabled": true`** — byte-identical. `git diff` shows only TASK-409's original 4-line insert.

⚠️ **A restore hazard I hit and caught — worth carrying forward.** For the last of the three hold-out cycles I toggled the entry with a Python round-trip instead of a text edit. Content was correct and **`git diff` looked perfectly clean**, but the file came back **805 B, sha `ca5c2cf6…`** — Python's universal-newline read had silently rewritten all 54 line endings CRLF→LF. **Git's autocrlf normalisation hides this completely; only the checksum caught it.** Restored from the backup copy; final state is `63058F3C…` / 859 B / `"Enabled": true`, verified. **Lesson: verify the hold-out restore by CHECKSUM, never by `git diff`.**

**The plugin remains UNPROVEN through UBT/UHT and keeps its TASK-412 gate.** This lane has no dependency on it (the subsystem is TASK-423, gated on the spike).

---

## 2. THE AUTOMATION TESTS — 11 found, 11 run, 11 pass, 0 skipped

Headless: `UnrealEditor-Cmd.exe … -ExecCmds="Automation RunTests Siegebound.Assistant" -TestExit="Automation Test Queue Empty" -unattended -nullrhi`

```
LogAutomationCommandLine: Display: Found 11 automation tests based on 'Siegebound.Assistant'
...
LogAutomationCommandLine: Display: ...Automation Test Queue Empty 11 tests performed.
LogExit: Display: **** TestExit: Automation Test Queue Empty ****
```

| # | Test | Result |
|---|---|---|
| 1 | `Siegebound.Assistant.Command.NeverPartiallyFills` | **Success** |
| 2 | `Siegebound.Assistant.Command.Rejection` | **Success** |
| 3 | `Siegebound.Assistant.Command.RoundTrip` | **Success** |
| 4 | `Siegebound.Assistant.Command.SelectionInvariants` | **Success** |
| 5 | `Siegebound.Assistant.Grammar.CountRange` | **Success** |
| 6 | `Siegebound.Assistant.Grammar.DegenerateInputs` | **Success** |
| 7 | `Siegebound.Assistant.Grammar.Determinism` | **Success** |
| 8 | `Siegebound.Assistant.Grammar.Grounding` | **Success** |
| 9 | `Siegebound.Assistant.Grammar.Intents` | **Success** |
| 10 | `Siegebound.Assistant.Grammar.SelectionCap` | **Success** |
| 11 | `Siegebound.Assistant.Vocabulary.SynonymTable` | **Success** |

**Counts, mechanically:** `Test Completed` lines = **11** · `Result={Success}` = **11** · `Result={Fail}` = **0** · `Skipped`/`NotRun`/`Cancelled` = **0**. Exactly the 11 names QA enumerated — none added, removed or renamed.

**QA's four load-bearing tests all pass:** `Grammar.CountRange` (the 1..30 range) · `Grammar.SelectionCap` (bounded alternation, no `*`/`+`/`?`) · and **both new guards, which live inside `Vocabulary.SynonymTable` (test 11, Success)** — guard A (`SiegeAssistantGrammarTest.cpp:955-956`, no `_` in a unit canonical) and guard B (`:968-969`, substring `mage`/`caster`/`spellcaster`), with the pre-existing whole-string check still present alongside at `:975-976`.

⚠️ **First test attempt crashed — NOT lane-attributable, recorded so nobody re-derives it.** My initial invocation added a non-standard `-noshadercompile` and hit `Fatal error: Null assigned to TNotNull` (`NotNull.cpp:12`) *after* `FEngineLoop::Init()` completed. **The callstack is 100% engine DLLs — `UnrealEditor-Core.dll` / `-Engine.dll` / `-UnrealEd.dll` / `UnrealEditor-Cmd.exe`, with ZERO frames in `UnrealEditor-GitClaudeUnrealTest.dll`.** Dropping the flag made it run clean first time. **A harness flag error of mine, not a lane defect, and not a finding against anyone.**

---

## 3. ⚠️ THE MilitiaMob CHECK — what I actually did, and what it does *not* prove

**I did NOT field a MilitiaMob and I did NOT run PIE. I claim neither.**
**Blocker:** the desktop is **LOCKED** (`LogonUI` running, PID 30612) — real input is impossible, exactly as at TASK-401.

**What I ran instead: the headless `-run=pythonscript` probe**, the substitute the spec permits. **But the substitute is narrower than the spec assumed, and that matters:** `CanonicalKind` is a **private static in `SiegeAssistantSnapshot.cpp`** and `GetUnitKinds()` is a **plain inline getter** — *neither is `UFUNCTION`-reflected*, so **Python cannot call either one.** `USiegeAssistantSnapshot` has **no reflected functions at all**. A literal "commandlet probe of `CanonicalKind`/`GetUnitKinds()`" is therefore **not possible**; that route should not be offered to a future task without this caveat.

**What IS reflected — and what I used:** `USiegeAssistantVocabulary`'s three `UPROPERTY` tables. So I read the **shipped CDO table** (the same object test 11 reads) and cross-checked it against **`DT_Cards` loaded from disk**:

```
CDO: /Script/GitClaudeUnrealTest.Default__SiegeAssistantVocabulary
TOTAL unit canonicals in shipped CDO: 13
DT_Cards row count: 30 · CardType==Unit rows (12):
  Archer Cavalry Cleric Footman Knight Longbowman MilitiaMob Ogre Pikeman Sapper Sorcerer Wizard

  Archer -> archer  YES     Knight     -> knight      YES    Pikeman  -> pikeman   YES
  Cavalry-> cavalry YES     Longbowman -> longbowman  YES    Sapper   -> sapper    YES
  Cleric -> cleric  YES     MilitiaMob -> militiamob  YES    Sorcerer -> sorcerer  YES
  Footman-> footman YES     Ogre       -> ogre        YES    Wizard   -> wizard    YES

MISSING from vocabulary: NONE
Vocabulary canonicals with no CardType==Unit row: ['miner']
    'miner' <- DT_Cards row 'Miner' of CardType 'Economy'     <- expected, QA-verified as correct

---- BLOCKER-1 focus ----
DT_Cards row spelled exactly: ['MilitiaMob']  (CardType=Unit)
CanonicalKind rule applied: 'MilitiaMob'.ToLower() -> 'militiamob'
shipped vocabulary contains 'militiamob': True
old broken spelling 'militia_mob' present in vocabulary: False
RESULT: AGREE - Zone A canonical == Zone C derived symbol

---- guard replication over the CDO table ----
unit canonicals containing '_': NONE
aliases embedding mage/caster/spellcaster: NONE
```

**What this proves:** the DT_Cards CardID is spelled exactly `MilitiaMob`; the shipped vocabulary spells `militiamob`; `militia_mob` is gone; and **all 12 Unit CardIDs + `miner` round-trip**. This is the **`DT_Cards` set-membership check QA identified as the thing that "would close the class outright"** but correctly declined to put in the asset-free suite (report §"The two new test guards"). It is now run — **externally to the suite, so it is not a regression guard**; it holds only for today's table.

**What this does NOT prove — stated plainly:**
- It **derives** `militiamob` by applying `CanonicalKind`'s documented one-line rule (`FName(*CardID.ToString().ToLower())`, `SiegeAssistantSnapshot.cpp:99-106`) **in Python. It does not execute the shipped function.** If `CanonicalKind`'s body ever stops matching its documentation, this probe would not notice.
- **No roster line was printed by the real serializer.** The live "field a MilitiaMob and watch Zone C print `militiamob`" confirmation is **still owed** and belongs to whoever first reaches PIE.

**Honest verdict: Zone A and Zone C agree on the data, by machine. The end-to-end live print is not claimed.**

### ⛔ Spec §(3) PIE sanity — NOT PERFORMED, zero items claimed

Every remaining §(3) item needs a live world and a reflected entry point; both are absent. **Explicitly NOT done and NOT claimed:** snapshot capture + full paste with char count vs `MaxSnapshotChars` · roster aggregation by CardID/group · `ancient_ground_near` ≠ `ancient_ground_far` (the 180° twin check) · no-coordinate/timestamp/prose sweep of captured text · `BuildZoneA` called twice and byte-diffed. **§(4) regression floor: NOT performed** (no PIE). These carry to the first task that reaches a live session — same disposition as TASK-401's (a)–(i) → TASK-402.

---

## 4. ⚠️ WARN-3 — SETTLED, AND IT IS LIVE. THIS GATES TASK-422.

**A green build does not close it, and did not.** Run exactly as QA specified:

```
$ git show HEAD:GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Castle.h | grep FindNearestCastleForTeam
(no output — grep exit 1)
$ git show HEAD:GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Castle.cpp | grep -c FindNearestCastleForTeam
0
```

> ⚠️ **Path note for anyone re-running this:** the repo root is **one level above** the project dir (`git rev-parse --show-prefix` = `GitClaudeUnrealTest/`). QA's literal command `git show HEAD:Source/...` fails with `fatal: path ... exists, but not ...` — **and that `fatal` exits non-zero and prints nothing, which reads exactly like "absent".** I nearly filed the right answer for the wrong reason. The command must be `HEAD:GitClaudeUnrealTest/Source/...`; the corrected form retrieved a real 26,084-byte blob and the grep genuinely ran.

**RESULT: `FindNearestCastleForTeam` is ABSENT from `HEAD` in both `Castle.h` and `Castle.cpp`.** It exists only in the **uncommitted working tree** — declared at `Castle.h:190`, defined at `Castle.cpp:655`.

**Consequence, and it is a hard ordering constraint:**
`SiegeAssistantSnapshot.cpp:234-235` calls it twice:
```cpp
const ACastle* const OwnCastle   = ACastle::FindNearestCastleForTeam(World, Team,      ReferenceLocation);
const ACastle* const EnemyCastle = ACastle::FindNearestCastleForTeam(World, EnemyTeam, ReferenceLocation);
```

> ### ⛔ **TASK-422 commit A MUST NOT land before — or without — the FOLLOW lane's `Castle.{h,cpp}` commit (TASK-403).**
> Committing the assistant lane first leaves **`main` non-compiling**, even though every working tree here is green. **Both boards look safe in isolation** — this is invisible from either one alone, which is exactly why it is written here in full.

**Options for the orchestrator:** (a) run **TASK-403 first**, then TASK-422 — the clean order; or (b) if TASK-422 must go first, its commit A **must additionally include `Castle.{h,cpp}`**, which crosses lane ownership and needs a ruling. **I did not choose; TASK-422 is not mine and nothing is staged.**

---

## 5. Other criteria

**QA note 4 — criterion (9), `AncientGround` additive-only: PASS, mechanically.**
```
$ git diff --numstat -- .../AncientGround.cpp .../AncientGround.h
44      0       AncientGround.cpp
28      0       AncientGround.h
```
**Zero deletions on both files** ⇒ zero modified lines (a modified line would show as one deletion + one addition). Purely additive, as TASK-418 claimed.

**`L_Arena`: NEVER opened, NEVER saved — 535,522 B / 7/29/2026 3:53:38 AM**, verified at gate start, after the build, and after both editor runs. Unchanged.

**Nothing dirtied.** Final `git status --porcelain` is **identical to the session-start snapshot** — no new entry, nothing staged, no `Content/` asset touched by either headless editor run (both loaded `DT_Cards` read-only). No commit, no push. `reset --hard` / `clean -fd` never invoked.

---

## 6. ⚠️ CARRY-FORWARD — the `.gitattributes` LFS ordering trap is LIVE, with numbers

Jonathan ruled `*.dll` and `*.lib` get LFS rules at the **repo root**. **Verified state today:**

- **`C:\GitProjects\GitHub\GitClaudeUnrealTesting\.gitattributes`** (305 B) covers `*.uasset *.umap *.fbx *.png *.jpg *.wav *.mp4` — **`grep -E "\*\.dll|\*\.lib"` returns NOTHING. The rule does not exist yet.**
- **`Plugins/` is 72 MB and has `git ls-files Plugins/` = 0 tracked files** — entirely untracked, nothing committed yet. **The trap has not sprung.**
- What the rule must catch: **20 `.dll`/`.lib` files**, headlined by **`ggml-vulkan.dll` at 49.9 MB**, plus `llama.dll` 2.7 MB and 15 `ggml-cpu-*.dll` at ~0.8–1.7 MB each, under `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/{bin,lib}/Win64/`.

> ### ⛔ **The pattern must be added and committed BEFORE the first commit that adds `Plugins/SiegeLlama/`.**
> Add the rule afterwards and those ~72 MB stay **raw blobs in git history permanently** — a later `.gitattributes` edit does not retroactively move them, and only a history rewrite would. **TASK-414 is what springs this**, and TASK-422 must not add `Plugins/` either.

---

## 7. Status

- **TASK-420 → `done`.** Compile GREEN · 11/11 tests · BLOCKER-1 confirmed closed by machine (headless, not live) · WARN-3 settled and LIVE.
- **TASK-421 is UNBLOCKED** — `USiegeAssistantVocabulary` is compiled and in the editor (the probe instantiated its CDO by name). ⚠️ Dispatch it with **WARN-6 and WARN-7 as explicit acceptance criteria** — my guard replication above covers the **C++ CDO defaults only**; the authored `DA_AssistantVocabulary` asset does not exist yet and **no test in the suite will ever see it.**
- **TASK-422 is BLOCKED on the WARN-3 ordering decision above** — not on anything technical in this lane.
- Still owed to a live session: all of spec §(3)/§(4), and the live MilitiaMob roster print.

**Artifacts:** build log, test log, probe script + output, and the `HEAD:Castle.h` blob are in the session scratchpad.
