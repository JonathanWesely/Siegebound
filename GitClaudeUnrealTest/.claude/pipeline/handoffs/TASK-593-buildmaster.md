# TASK-593 — [WR-39] THE COMPILE + ⭐ THE ROW (m) RE-OBSERVATION (build-master handoff)

**Date:** 2026-08-16 · **HEAD:** `f205eb5` (⛔ **UNCHANGED — no commit, no push**) · **Branch:** `main`, `0 0` vs `origin/main`
**Gate honoured:** `qa/TASK-592.md` = **PASS** (0 BLOCKER / 3 WARN / 4 NIT) before a single compile action.
**Route:** board flips → entry ledger → `SC-§9` dirtiness sweep → **graceful close** → **COMPILE** → **SUITE** → ⭐ **FRESH EDITOR BOOT** → **PIE 2m34s** → row (m) read → StopPIE → exit ledger.
⛔ **NO COMMIT. NO PUSH. NO `L_Arena` SAVE. NO ASSET SAVE OF ANY KIND. NOTHING ADDED TO THE GIT INDEX.**

---

## 0. ⭐⭐ THE THREE-LINE RESULT

1. ⭐⭐ **ROW (m) IS ZERO. THE DEFECT IS CLOSED BY MEASUREMENT.** `Blueprint Runtime Error` = **EXACTLY 0** across **154 s (2 m 34 s)** of live PIE with both commanders spawned — **3.1× the 49 s window that produced the original 1,806**. `TryGetPawnOwner` = **0**. `ABP_Footman` = **0 mentions in the entire session log**.
2. ⭐ **THE COSMETIC ANSWER IS THE GOOD ONE: THE SINGLE-NODE IDLE IS PLAYING, AND I DID NOT INFER IT FROM A PICTURE.** Zero fall-through log lines **plus** a live property readback on **both** commanders: `AnimationMode = AnimationSingleNode`, `AnimClass = None`. ⇒ **the manager's skeleton hypothesis HELD.** §3.
3. ✅ **COMPILE `Result: Succeeded` (0 errors, 0 warnings) and the SUITE IS 111/111, ZERO FAILURES** — the non-invalidation prediction landed exactly.

⛔ **AND THE STANDING ONE: THE MATRIX DID NOT IMPROVE.** Nothing in TASK-591 touches input injection ⇒ **the 11 unobserved rows are STILL UNOBSERVED** and are still inherited by TASK-571. §6.

---

## 1. ✅ THE COMPILE — `Result: Succeeded`, PARSED FROM THE LOG

```
Build.bat GitClaudeUnrealTestEditor Win64 Development -project=…GitClaudeUnrealTest.uproject -waitmutex
```

| fact | reading |
|---|---|
| ⭐ **the parsed verdict** | **`Result: Succeeded`** · **`Result: Failed` occurrences = 0** |
| ⛔ the exit code | **`0` — RECORDED AND NOT TRUSTED**, per the standing machine law (it reads `0` on a failed build too) |
| errors / warnings | ⭐ **0 / 0** — `error C`, `warning C`, `error LNK`, `warning LNK` all **absent** |
| wall clock | **12.23 s** (UBA local executor, 8 actions) |
| ⭐ the actions that matter | `[1/8] Compile CommanderNpc.cpp` · `[6/8] Link UnrealEditor-GitClaudeUnrealTest.lib` · `[7/8] Link UnrealEditor-GitClaudeUnrealTest.dll` |
| ⭐ **the relink is PROVEN, not assumed** | DLL **4,913,664 B @ 08-15 19:34** → **4,925,440 B @ 08-16 00:39:47**. **+11,776 B, timestamp moved.** |

⛔ **Smart App Control did NOT fire** — no ~2 s death, no `0x800711C7`. The Jonathan-only hazard was not in play and **no QA loop was spent.**

### ⚠️ THE ONE PROCEDURAL FACT THAT DECIDED THE WHOLE TASK — I MEASURED IT RATHER THAN ASSUMING THE EDITOR WAS USABLE

**The running editor held a PRE-FIX binary.** Source `CommanderNpc.cpp`/`.h` last written **08-16 00:15:40 / 00:16:44**; the loaded module DLL was **08-15 19:34:19** — **~4.7 hours older than the fix**.
⇒ ⛔ **A PIE run against that process would have re-observed the OLD defect and proved nothing.** ⭐ **Row (m) was structurally unobtainable without a rebuild, and the rebuild cannot link while the editor holds the DLL.** ⇒ the editor bounce was **mandatory**, not convenience. §7.

---

## 2. ✅ THE SUITE — **111 / 111, ZERO FAILURES**

```
UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests Siegebound" -unattended -nopause -nullrhi
  -testexit="Automation Test Queue Empty" -abslog=<scratchpad>\suite-593.log
  "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry.Entry"
```
Run from **PowerShell**, ⛔ never Git Bash.

> `LogAutomationCommandLine: Display: ...Automation Test Queue Empty **111 tests performed.**`

**Authoritative tally, grouped over every `Result={…}` line: total result lines = 111, `Success` = 111.** ⇒ ⭐ **111/111, zero non-Success. EVERY failure by name: there are none.** Suite wall-clock **20.43 s**.
📌 **111 is exactly the expected count** — TASK-591's non-invalidation grep predicted it and the machine agrees. ⛔ **No new test was owed and none was added.**
⛔ **The commandlet exit code read `0` and is likewise NOT trusted** — the verdict above is the parsed log.
✅ **The Zone-A pair (`MeasuredCharCount` / `TwoLaneByteEquality`) is inside the 111 and green** — the equality proof stands, re-confirmed rather than re-derived. ⛔ **No token figure is quoted or derived anywhere in this document** (`AS-§12g`).

🔒 **The `-ini:` override did its job COMPLETELY:** `L_Arena` mentions in the suite log = **0**, `LoadMap`/`Bringing World` = **0**, `/Game/Maps` saves = **0**. **The never-save map was never opened, not merely never written.**

---

## 3. ⭐⭐ ROW (m) — THE HEADLINE. **ZERO.** AND ROW (3) — **THE IDLE IS PLAYING.**

**Conditions:** ⭐ **fresh editor boot** (PID 15484, launched 00:41:27 after the graceful close), `L_Arena` opened by the startup-map setting, ⛔ **never saved**; PIE entered with ⛔ **no `startTransform` override**; PIE **07:48:32 → 07:51:06**.

### 3.1 ROW (m) — THE COUNT AND THE DURATION, ⛔ NEVER "LOOKED CLEAN"

| fact | measured |
|---|---|
| ⭐⭐ **`Blueprint Runtime Error`** | ⭐⭐ **0 — EXACTLY ZERO** |
| ⭐ **duration observed** | ⭐ **154 s (2 m 34 s)** — **3.1× the 49 s** that produced 1,806 |
| pre-PIE baseline | **0** (log boundary marked at 2,413 lines before `StartPIE`) |
| `TryGetPawnOwner` | **0** — the original error's signature string, absent |
| `ABP_Footman` | **0 mentions in the whole session log** |
| `Fatal` | **0** · `ensureAlways` **0** · `LogOutputDevice: Error` **0** |
| both commanders alive for the window | ✅ `BP_CommanderNpc_C_0` (Blue) + `BP_CommanderNpc_C_1` (Red) |

⚖️ **Why this is the strong form of the claim:** the original defect threw **2 errors per frame per NPC** with 2 NPCs alive — a continuous flood that needed only seconds to be visible. I ran **more than three times** the original window with **both** NPCs alive the whole time and the count never left zero. ⛔ **A quiet 5-second glance is not what happened here.**

**The 14 `Error:` lines in the session, classified — ⛔ NONE is row (m):**
- **13 × `LogLiveCoding: Error: Cannot enable module …/Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/…`** — ⭐ **exactly the 13 benign Live Coding variant notices TASK-569 recorded. Count matches to the unit.**
- **1 × `LogModelContextProtocol: Error: Unknown session id … client should reinitialize`** — ⚠️ **mine, not the game's**: my MCP client reconnecting to the freshly-booted editor at **07:48:08**, i.e. **before PIE started at 07:48:32**. Named rather than quietly folded into the benign bucket.

### 3.2 ⭐ ROW (3) — THE COSMETIC OBSERVATION. **THE SINGLE-NODE IDLE IS PLAYING.**

**Which of the handoff's four rows I actually saw: ROW 1 — *the commander idles*, the hypothesis HELD.**

⛔ **I did not claim this from a picture, and I did not claim it from an absence alone.** The discriminator the gate named is the log line, and it reads:

> **`CommanderNpc: idle sequence` → 0 lines.** (Neither the *unresolved sequence* line nor the *skeleton mismatch* line fired, on either commander.)

⭐ **And because "ref pose with no line at all" is the one ambiguous row QA told me to report, I closed it POSITIVELY with a live property readback on BOTH commanders rather than leaving it to inference:**

```
BP_CommanderNpc_C_0 .AvatarMesh   AnimationMode = AnimationSingleNode   AnimClass = None
BP_CommanderNpc_C_1 .AvatarMesh   AnimationMode = AnimationSingleNode   AnimClass = None
      SkeletalMeshAsset = /Game/Characters/SK_Sorcerer.SK_Sorcerer   bEnableAnimation = true
```

⇒ ⭐⭐ **Two independent facts fall out of one readback:**
1. **`AnimationMode = AnimationSingleNode`** is set **only** inside `ApplyAvatarAnimation` rung 2, immediately before `PlayAnimation`. ⇒ **the ladder RAN and REACHED PLAYBACK.** ⛔ **The fourth table row — *"ref pose with no log line"* — is RULED OUT by measurement, not by hope.** ⇒ combined with zero fall-through lines, **the skeleton identity test PASSED** ⇒ ⭐ **`A_Sorcerer_Idle` DOES share `SK_Sorcerer`'s skeleton. The hypothesis the manager could not verify is now confirmed at runtime.**
2. ⭐⭐ **`AnimClass = None` IS RUNG (a) — THE LOAD-BEARING HALF — CONFIRMED AT THE LIVE ACTOR.** The commander's mesh has **no AnimInstance class at all**, so `ABP_Footman`'s graph is **not instantiated on this actor** and structurally cannot be. **This is the fix, observed on the running object rather than argued from the diff.**

📌 **NIT-4 pre-empted:** `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` confirmed live — the pre-existing, out-of-diff setting. ⛔ Not a finding, recorded so it is not misfiled against row (3).
⛔ **Whether he *looks* right — silhouette, breathing amplitude, whether the idle reads at 9× castle scale — is an APPEARANCE claim and remains Jonathan's pixel check.** I claim only what the log and the property readback say.

---

## 4. 📌 THE FREE RE-CONFIRMATIONS — ⛔ REPORTED, ⛔ NOT RE-DERIVED. **ALL THREE MATCH; NO STOP.**

| check | TASK-569 | this run | verdict |
|---|---|---|---|
| nav verdict string | `CONFIRMED (nav settled: 0 pending)`, 0 pending, 0 culls, 6 mine paths, `[definitive: OnNavigationGenerationFinished]` | ⭐ **IDENTICAL, all fields** | ✅ **match** |
| 12 spawned torches are `BP_Torch_C` | 12, all `BP_Torch_C` | ⭐ **12, all `BP_Torch_C`** | ✅ **match** |
| both commanders spawn + stand in the hall | 2, Red at `(24535, 810, 174)` | ⭐ **2; `CommanderNpcInit team=Blue P=(-25465, 810, 174)` and `team=Red P=(24535, 810, 174)`, `interactRadius=400 revealCost=30`** | ✅ **match, incl. z=174 interior floor** |

**The nav line, verbatim:**
```
[2026.08.16-07.48.37:938][772]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0']
  Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending;
  Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s))
  [definitive: OnNavigationGenerationFinished].
```
⭐ **Settle latency ≈5.4 s after PIE start, and it is the `[definitive: …]` event-driven pass again — ⛔ not the settle-poll.** ✅ `WR-§9` rows 8 + 13 values (`interactRadius` 400, `revealCost` 30) confirmed live and **unchanged by this diff**.
⚠️ **`ASummonedUnit` did not regress into view:** zero new `Warning`s from the commander class, and **zero** `ABP_Footman` errors of any kind. ⛔ **I did NOT observe unit locomotion directly** (it needs a card play I have no lane for) — so *"unit locomotion still works"* is **NOT** claimed; what is claimed is that nothing in the log suggests this diff over-reached.

---

## 5. 🚩 FINDINGS — ⛔ NONE FIXED HERE

1. ⭐ **NO NEW DEFECT FOUND. The TASK-569 defect is CLOSED.** This is the first row (m) reading in the batch's history to come back zero.
2. 📌 **INSTRUMENT NOTE, worth a `TL-§` line — an asset sweep's TOTAL depends on its SCOPE, and the two are easy to confuse.** My whole-project sweep returned **7,074** assets where TASK-569 recorded **3,229**. ⛔ **Not a discrepancy and ⛔ not drift:** `find_assets("")` searches `/Game` **plus every project and engine plugin content root**, while `/Game`-scoped returns **3,229 found / 58 `__External*` excluded / 3,171 scanned — reconciling with TASK-569 to the unit.** ⇒ **Report the scope with the count, or a future agent will read a plugin-inclusive total as 3,845 assets of drift.**
3. ⚠️ **The QA report's three WARNs and R7 are UNADDRESSED BY DESIGN and are the manager's to board** — ⛔ none is commit-blocking and ⛔ none was touched here: **WARN-1** (`CommanderNpc.cpp:351-357` should name the runtime `USkeleton::IsCompatibleMesh` and why it was still declined — comment-only); **WARN-2** (the rung-1 escape hatch is guarded only by prose; the suggested `Warning` at rung 1 cannot fire on the shipped path); **R7** (`SummonedUnit.cpp:445-453` documents the *skeleton* contract but not the **owner-class** contract `SC-§35` item 1 now requires — the only remaining undocumented `TSoftClassPtr<UAnimInstance>` assignment, and `:455` is a composed path future authors will copy).
4. ⚠️ **PRE-EXISTING, ⛔ NOT THIS BATCH:** the 13 Live Coding `LlamaCpp` notices persist. ⛔ Not investigated, ⛔ not this task's.

---

## 6. ⛔⛔ THE MATRIX DID **NOT** IMPROVE

⛔ **Stated in the required words: THE MATRIX DID NOT IMPROVE, AND IT MAY NEVER BE REPORTED AS "PASSED."**
**Nothing in TASK-591 touches input injection.** The MCP surface still exposes **no keyboard injection, no mouse injection and no console-exec lane** — I re-read `EditorAppToolset`'s tool list this session and it gives `StartPIE` / `StopPIE` / `IsPIERunning` / captures / selection / camera **and nothing else**. ⇒ **The 11 unobserved rows — (b) (c) (d) (g) (h) (i) (j) (k) (l) (o) (q) — plus row (r), row (n)'s respawn half and row (e)'s *Play Again* respawn half are STILL UNOBSERVED and are STILL INHERITED BY TASK-571.**
⛔ **I did not re-attempt them and ⛔ I did not improvise a console route** (`W7-R5` proved none composes). ⛔ **Row (r) remains structurally blocked** on the TASK-569 §6 contradiction, which still needs a manager ruling.
✅ **What DID close is row (m) alone** — and it closed because a rebuild made the fixed binary observable, ⛔ not because the instrument surface grew.

---

## 7. 🔒 STATE LEDGER — MEASURED, ⛔ NOT ASSERTED

| item | state |
|---|---|
| 🔒 `Content/Maps/L_Arena.umap` | ✅ SHA256 **`b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268`** — **IDENTICAL at entry AND exit**, plus after the suite. **535,522 B, mtime still `2026-07-29T03:53:38`.** **Hash, ⛔ never mtime.** ⛔ Opened and played only, ⛔ **never saved.** 🔒 **The nav-save exception stays SPENT/EXPIRED; ⛔ no new one requested.** |
| ⭐ `Saving Package:` — the COMPLETE list | ⭐⭐ **NONE. ZERO package saves of any kind this session.** `/Game/Maps` saves = **0**. ⛔ **`save_assets` was never called, in any form.** |
| commits / pushes | ✅ **NONE.** `HEAD` = **`f205eb5`**, **`0 0`** vs `origin/main` |
| the git index | ✅ **UNCHANGED — 9 staged paths at entry, the SAME 9 at exit.** ⛔ **I ran NO `git add`, NO `git restore --staged`, NO commit, NO push.** |
| ⛔ `Content/UI/WBP_WarMap.uasset` | ✅ **STILL `AM`, UNTOUCHED — the index still holds the empty shell.** ⛔ **Left exactly as found; it is TASK-570's `git add` to fix.** |
| `git status` total | **88 entries at entry, 88 at exit — identical** |
| `.gen.cpp` / `Intermediate/` / `.umap` / `Saved/` in `git status` | ✅ **ZERO of each, at entry AND exit** — the `SC-§29b` STOP did not trip |
| C++ authored | ⛔ **NONE.** ⛔ No `Tools/**/*.py` added or edited ⇒ **no `SC-§27` gate owed** |
| 🔒 TASK-552's one-shot latch | ✅ **UNSPENT** — ⛔ the assistant console was **never opened**, ⛔ no sentence composed or submitted, ⛔ no `DumpAssistantPrompt` / `SpikeEval` / `SpikePrompt`. **Running PIE does not spend it, and it did not.** |
| 🔒 sealed holdout | ✅ untouched · ⛔ no CSV touched · ⛔ **no token figure quoted or derived** (`AS-§12g`) |
| `Content/` authoring | ⛔ **NONE** — no asset created, modified, imported or saved |

### THE EDITOR — `W7-R2`'s THREE LIMITS DISCHARGED
1. ✅ **DIRTINESS RE-MEASURED BY ME IMMEDIATELY BEFORE CLOSING** (`SC-§9` — ⛔ TASK-569's ledger was **not** trusted): `/Game` **3,229 found · 58 `__External*` excluded · 3,171 scanned ⇒ ⭐ `dirty_count: 0`**, and **0** across the plugin-inclusive 7,074 too. ⇒ ⛔ **Nothing was Jonathan's to pick.**
2. ✅ **GRACEFUL ONLY** — `CloseMainWindow()` accepted `True`, process exited within the timeout, `Get-Process` confirmed **no `UnrealEditor` process remained**. ⛔ **`Kill()` was NEVER called.**
3. ✅ **RE-OPENED FRESH** (PID **15484**) — the boot row (m) required — **and left RUNNING with MCP live** for TASK-570.
4. ✅ **CLOSING SWEEP — the editor is handed back CLEAN:** PIE stopped (`IsPIERunning = false`) and a second full sweep re-measured **3,229 / 58 / 3,171 ⇒ `dirty_count: 0`.** ⛔ **Nothing left dirty for TASK-570 to trip over.**

### THE COMPLETE SET OF FILES THIS TASK TOUCHED — ⛔ TWO, AND ⛔ NO MORE
```
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-593-buildmaster.md   (this file)
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                        (3 surgical - status: lines)
```
**The three board lines: TASK-592 → `done`, TASK-591 → `qa-passed / ready-for-integration`, TASK-593 → `done`.** ⛔ **Each located at its own `#### TASK-###` block and edited on its own `- status:` line — ⛔ NO bulk replace.**

---

## 8. ➡ WHAT TASK-570 INHERITS

- ⭐⭐ **THE COMMIT GATE IS NOW SATISFIED.** `qa/TASK-592.md` = PASS gated the **shape**; ⭐ **row (m) = 0 gates the RESULT.** ⇒ **TASK-591 is cleared to ship. ⛔ The commit-blocking condition on TASK-570 is DISCHARGED.**
- ⛔⭐ **THE COMMIT TRAP IS STILL LIVE AND I DELIBERATELY DID NOT DISTURB IT.** `Content/UI/WBP_WarMap.uasset` reads **`AM`**; the worktree holds the rooted asset and **the index still holds the empty shell**. ⛔ **`git add` that path first, or the commit ships the EMPTY widget while `git status` looks satisfied.** ⚠️ **Under LFS the staged blob is a POINTER — ⛔ do not "sanity check" it by expecting a `.uasset`-sized number.**
- ⛔ **TASK-570's commit list must include `Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h` and `CommanderNpc.cpp`** — the two files this task compiled and observed.
- 📌 **The build is warm and the binary on disk is the OBSERVED one** (DLL `08-16 00:39:47`). ⛔ **If anyone edits C++ before TASK-570 commits, row (m)'s reading no longer describes the committed tree** and the observation must be retaken.
- 🚩 **TASK-571 still inherits the 11 unobserved rows** plus row (r), row (n)'s respawn half and row (e)'s *Play Again* half. ⛔ **This may NEVER be reported as "the matrix passed."**
- 📌 **For the manager:** the three QA WARNs + R7 (§5 item 3) are comment-only follow-ups, and the §5 item 2 scope/instrument note is worth a `TL-§` line.

⛔ **AND THE STANDING ONE: NOTHING IN THIS DOCUMENT CLAIMS ANYTHING LOOKS RIGHT.** Every claim above is a parsed build verdict, a test tally, a log count, a live property readback, a file hash or a process fact. **Whether the commander's idle reads well on screen is Jonathan's pixel check.**
