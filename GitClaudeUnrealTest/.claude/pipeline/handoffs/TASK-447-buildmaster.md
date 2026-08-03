# TASK-447 — build-master handoff (W1-INT compile gate)

**Date:** 2026-08-03 · **Agent:** build-master
**STATUS: ✅ COMPILE GREEN · ✅ COMMITTED (3 commits) · ⚠️ AUDIT PARTIAL · ⛔ SMOKE TESTS NOT RUN · ⛔ TASK-438 NOT DONE**

> ✅ **`Result: Succeeded`** on the third attempt, **zero errors, zero warnings**, and ⭐ **the LINK
> STEP RAN FOR THE FIRST TIME** — both `UnrealEditor-GitClaudeUnrealTest.dll` and
> `UnrealEditor-SiegeLlama.dll` linked. **The 22-symbol seam prediction is now a MEASUREMENT.**
> ✅ **First PIE session in this repo's history ran on `L_Arena`. Zero gameplay errors, zero ensures.**
> ⛔ **BUT: `ReportFirstCapture` NEVER FIRED, so (0-ZONEB) IS STILL NOT MEASURED** — and no substitute
> is offered. ⛔ **No smoke test ran: this MCP surface has NO console-exec and NO input-injection tool.**
> **Commits: `cd5f4ed` · `97c7c8d` · (docs, this commit). ⛔ NOT PUSHED.**

---

## 1. THE COMPILE — THREE ATTEMPTS, AND THE EXIT CODE LIED TWICE

| # | `Result:` | exit | cause |
|---|---|---|---|
| 1 | `Failed (OtherCompilationError)` 6.44 s | **6** | ⛔ Live Coding mutex (editor held the module). Named no file |
| 2 | `Failed (OtherCompilationError)` 14.21 s | **6** | ⛔ **REAL** — 8 diagnostics, 2 TUs → QA loop 1 on TASK-444 + TASK-450 |
| 3 | ✅ **`Succeeded`** 5.38 s | **0** | ✅ green |

⛔ **`$LASTEXITCODE` WAS `6` FOR BOTH AN ENVIRONMENT BLOCK AND A GENUINE CODE FAILURE — THE SAME VALUE
FOR TWO DIFFERENT CAUSES — AND `0` FOR THE SUCCESS.** ⚖️ **On this run the exit code was not merely
unreliable, it was actively uninformative: it could not distinguish "your code is broken" from "close
the editor."** The log was the only authority, every time.

**Attempt 3, quoted:**
```
[1/7] Compile [x64] SiegeLlamaSubsystem.cpp
[2/7] Link [x64] UnrealEditor-SiegeLlama.lib
[3/7] Compile [x64] SiegeAssistantConsoleWidget.cpp
[4/7] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[5/7] Link [x64] UnrealEditor-SiegeLlama.dll
[6/7] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
Result: Succeeded
```
✅ **Binaries on disk, timestamps matching the build:** `UnrealEditor-GitClaudeUnrealTest.dll`
(3,785,728 B) and `UnrealEditor-SiegeLlama.dll` (419,840 B), both `14:54:53`.

⭐ **WHAT THE LINK DISCHARGES:** every "unresolved externals unproven" caveat in `qa/TASK-446.md`,
`qa/TASK-424.md`, `qa/TASK-461.md` and my own earlier handoff. **All 22 cross-task seams resolved.**
⚖️ **My pre-flight predicted this; the linker proved it. The prediction is now retired as a claim.**

---

## 2. ⭐ THE FIRST-EXECUTION AUDIT — WHAT ACTUALLY RAN, AND WHAT DID NOT

**PIE started on `/Game/Maps/L_Arena` (already the loaded level — I did not have to open it), standard
in-viewport PIE, 25 s warmup. `StartPIE` returned after 40.5 s. This is the first time any of this code
has executed.**

### ✅ MEASURED FOR THE FIRST TIME — REAL NUMBERS, QUOTED FROM THE LIVE LOG

| finding | value |
|---|---|
| ⭐ **`zoneA_chars`** | **3029** — *first ever shipped-builder Zone A measurement* |
| ⭐ **ggml backends ready (the synchronous step)** | **110 ms** |
| **Model load** | **1874 ms**, `tier=full`, `gpu_layers=-1`, 2375.9 MiB, 36 layers, RTX 5070 Laptop |
| **Tier decision** | `5156 MiB free >= 2703 (measured full-offload cost) + 768 (game reserve)` |
| **Settings subsystem** | slot `SiegeSettings`, **`bAssistantConfirmBeforeExecute=true`** |
| **Component** | `USiegeAssistantComponent ready on 'SiegePlayerController_0' (state Idle)` |

### ⭐ (0-BACKENDS) — THE OWED `N ms`, AND IT **CORRECTS THE BOARD'S PREMISE**

> `LLM_LOAD: ggml backends ready in **110 ms**. Starting the **ASYNC** model load on a
> TPri_BelowNormal worker -- Initialize() returns now and **match start never waits on it**.`

⇒ **The synchronous cost on the game thread is 110 ms — that is the number owed, and it is now
reported rather than asserted.** ⚠️ **The board and `qa/TASK-446.md` framed this as
*"`EnsureBackendsLoaded()` is synchronous ⇒ QUALIFIED-PASS ONLY."* **The measurement refines that:**
the **backend** step is synchronous and costs **110 ms**; the **2.4 GB model load** is genuinely
**asynchronous on a below-normal worker.**
⇒ ⚖️ **It remains a QUALIFIED pass, and I am not upgrading it** — 110 ms *is* a game-thread stall, and
⛔ **I did not measure it at match start under load; I read the subsystem's own startup line.** **The
slogan is still not a pass; the number is now on the record.**

### ✅ RUNTIME DISCHARGES — criteria closed by execution, not by reading

- ✅ **TASK-436's missing-slot path, EXECUTED:** *"No readable settings in slot 'SiegeSettings' (user 0)
  — falling back to C++ defaults."* **Degrades to defaults, logs once, does not crash — observed.**
- ✅ **Default is `true`, OBSERVED at runtime** (`bAssistantConfirmBeforeExecute=true`), not inferred.
- ✅ **The component constructs on the controller and sits in `Idle`** — TASK-440's subobject and
  TASK-442/443's FSM initialise cleanly.
- ✅ **`LogSiegeAssistant`, `LogSiegeLlama`, `LogSiegeSettings` are all registered** ⇒ both new modules
  loaded into the editor.
- ✅ **ZERO gameplay errors, ZERO ensures, ZERO script-stack warnings** across the whole session.

### ⛔ (0-ZONEB) — **STILL NOT MEASURED. NO SUBSTITUTE OFFERED. THE CONSTANT IS UNCHANGED AT 192.**

⛔ **`FIRST LIVE CAPTURE of USiegeAssistantSnapshot` returned ZERO matches.** Probed across **all**
categories, by the exact literal anchor.

**Why, mechanically:** `ReportFirstCapture` fires on a **snapshot capture**, which happens when the
assistant console is **opened / an utterance submitted**. That requires a **keypress**. ⛔ **This MCP
surface exposes no input-injection tool and no console-exec tool**, so nothing in this session could
open the console. ⇒ **`USiegeAssistantSnapshot` STILL has zero captures; `zoneB_chars` / `zoneC_chars`
remain unobserved; `MaxRosterKinds = 8` was never tested against a real board.**

⚖️ **I could have forced `bAssistantConsoleOpen` through `ObjectTools.set_properties`. I did not** —
it bypasses the single-writer guard, would not route through the capture path anyway, and is exactly
the *"synthesize a procedure at the gate"* that (0-DEADLINE)(c) forbids. ⛔ **An honest NOT MEASURED
is a result; a manufactured one is the defect this batch exists to catch.**
⇒ **`ZoneBCharReserve` stays 192, deliberately untouched. The reading is still owed and still cheap —
it needs one human keypress in PIE, which is TASK-448.**

### ⚠️ "UNITS ON THE FIELD" — NOT ESTABLISHED, AND MOOT
The board requires the audit on a populated board. I could not populate one (no input ⇒ no cards
played), and since **the capture never fired at all**, the population question was never reached.
**Stated rather than glossed.**

---

## 3. ⭐ THREE REAL FINDINGS THE FIRST EXECUTION SURFACED — NEW, NOT IN ANY REPORT

**These are the return on running the thing. All three are for the manager; none is a defect in this gate.**

### 🚩 FINDING 1 — `DA_AssistantVocabulary` DOES NOT RESOLVE. THE SYNONYM TABLE IS EMPTY IN THE SHIPPED LANE.
> `Warning: DA_AssistantVocabulary did not resolve ('/Game/Data/DA_AssistantVocabulary.DA_AssistantVocabulary').
> Zone A's synonym block prints 'none', deterministically … logged ONCE and never retried, because a
> mid-session success would change Zone A.`

⚠️ **The degradation is well-engineered — deterministic, one-shot, KV-prefix-stable — but the asset is
simply absent, so `vocabulary=none` and `zoneA_chars=3029` is a measurement OF THE DEGRADED LANE.**
⛔ **THIS IS ACCURACY-RELEVANT AND IT TOUCHES THE SEAL:** Bar #5 sits at 20/25,
`Siegebound.Assistant.Vocabulary.SynonymTable` is one of the 33 tests, and 🔒 **the gen-2 holdout was
authored against the SPIKE's `t0` fixture** — if the spike lane had a vocabulary and the shipped lane
prints `none`, **the two lanes' Zone A differ**, which is precisely what
`Siegebound.Assistant.ZoneA.TwoLaneByteEquality` exists to detect. **Manager's call; I am not
diagnosing it further.**

### 🚩 FINDING 2 — THE §8 BUDGET GUARDRAIL IS **INERT AT MATCH START**, AND TASK-455's PREDICTION IS FALSIFIED *AT STARTUP*
> `Warning: LLM_BUDGET ASSERTION IS INCOMPLETE -- NO STATIC PREFIX HAS BEEN REGISTERED, so ZoneA_tokens
> is UNMEASURED and is being treated as 0. What IS checked: MaxSnapshotTokens=400 + MaxOutputTokens=96
> + SafetyMarginTokens=48 = 544 vs n_ctx_actual=2048. This is NOT the section 8 guardrail.`

⚠️ **TASK-455 predicted its `SetStaticPrefix` call would stop these warnings; it explicitly labelled
that *"a prediction about unexecuted code."*** **Measured: at match start the warning still fires**,
because registration happens later (first console open) — which **never happened this session**.
⇒ ⚖️ **The §8 guardrail is real but LATE: there is a window from match start until the first console
open in which the budget authority is not armed.** Whether that window matters is a manager ruling.
✅ **The prediction was correctly labelled — this is the labelling working, not a failure.**

### 🚩 FINDING 3 — THE SPIKE'S RETENTION HAS A MEASURED RUNTIME COST
> `Warning: THE SPIKE HARNESS IS STILL PRESENT IN THIS BUILD, so this process has TWO independent
> model-load paths. DO NOT drive Siege.Llama.Spike* and Siege.Llama.Test / the assistant in the SAME
> session -- that puts two ~2.5 GB models on one card.`

⚠️ 🔒 **§16 keeps the spike, correctly — but the cost is now measured, not theoretical: any future
smoke run must choose ONE lane per session.** With `tier=full` taking 2375.9 MiB of 5156 MiB free,
**two models genuinely will not fit.** ⇒ **This constrains how the owed smoke tests can ever be run,
and it should be written into their spec.**

### 📌 NIT — 13 `LogLiveCoding: Error: Cannot enable module … ggml-cpu-*.dll` lines
**Benign and pre-existing.** Only the CPU-matching ggml variant loads; Live Coding complains about the
other 12 vendored variants. **Not gameplay errors, not introduced by this batch.** Recorded so nobody
greps `Error:` and files a false blocker.

---

## 4. ⛔ THE SMOKE TEST — **NOT RUN**, AND THE REASON IS TOOLING, NOT SCHEDULE

⛔ **THIS MCP SURFACE HAS NO CONSOLE-EXEC TOOL AND NO INPUT-INJECTION TOOL.** I enumerated all 19
toolsets. `EditorAppToolset` offers `SearchCVars` but **no exec**; `ProgrammaticToolset` orchestrates
*registered tools only*, not arbitrary commands. ⇒ **`Siege.Llama.Status` / `Siege.Llama.Test` /
`Siege.Llama.Cancel` cannot be issued from this session at all.**

| check | status |
|---|---|
| `Siege.Llama.Info` · one parseable JSON · **queue depth 1** · absent-model path | ⛔ **NOT RUN** — no console-exec tool |
| *async load never blocks match start* | ⚠️ **QUALIFIED — measured: 110 ms synchronous (backends); model load async.** Reported as a number, never as a slogan |
| *cancel mid-prefill* | ⛔ **NOT RUN** — and ⚠️ **not exercisable in this session anyway: the tier resolved to `full`, and mid-graph cancel is `tier=cpu` only** |
| **partial-tier frame-time hitch re-measure** | ⛔ **NOT MEASURED — STRUCTURALLY IMPOSSIBLE from the subsystem** (sampler exists only in the spike). **Recorded, not dropped** |
| **deadline exercise** | ⛔ **NOT EXERCISED** — nothing ran long enough, and nothing ran at all |

⚠️ **CARRIED FORWARD UNCHANGED — the trap is still armed for whoever runs these:** ⛔ **the absence of
a `HARD TIMEOUT` line is NOT "no timeout."** On `tier=cpu` the abort exits via `abort_callback` ⇒
`llama_decode` returns **2** ⇒ a branch with **no elapsed field**. Disambiguator: **`code 2` +
`bCancelled == false` ⇒ deadline or shutdown, never a player cancel.** ⛔ **A spike-printed figure is
CONFIGURED, not measured.** Quote the overshoot as **`printed − 10.0`** (~50 ms `%.1f` floor).

⚠️ **QA-461 WARN-1 labels, corrected form:** *"worst TTFT — **an upper bound on prefill** — 434 / 1706
/ 2914 ms"* and *"**partial's worst total wall** 8926 of 10000."* ⛔ **Not "prefill", not "DECODE wall."**

---

## 5. ⛔ TASK-438 — NOT DONE. THE SESSION IS HOSTED AND READY; THE WORK IS THE ART-DIRECTOR'S.

⚖️ **A declared boundary, not an omission.** The board assigns **TASK-438 to `art-director`**, and
TASK-447's role is to **host** it. My own role bars authoring UI. ⇒ **I did the hosting half and
stopped at the boundary rather than quietly doing another agent's task.**

✅ **Everything the art-director needs is live right now:**
- **Editor running, PID 9828**, on the **freshly compiled** module — `USettingsMenuWidget` **will**
  appear in the `CreateWidget` class picker (it compiled and linked; the class registered).
- **MCP up at `http://127.0.0.1:8000/mcp`** with `BlueprintTools` fully available — `find_nodes`,
  `create_node`, `connect_pins`, `add_event`, `set_pin_value`, `read_graph_dsl`, `compile_blueprint`.
  ⚠️ **`write_graph_dsl` exists and TASK-438's spec forbids it — granular ops only.**
- ⚠️ **The MCP server is a meta-gateway:** call `call_tool` with `toolset_name` + an **unprefixed**
  `tool_name`. A working PowerShell client is at `scratchpad/mcp.ps1`.
- 🔒 **`L_Arena` must not be saved. Save `/Game/UI/WBP_MainMenu` ONLY.**

---

## 6. ✅ THE COMMITS — REAL HASHES, READ BACK FROM `git log`. ⛔ NOT PUSHED.

| # | hash | contents |
|---|---|---|
| 1 | **`cd5f4ed`** | **Code** — 23 files, **12872 insertions / 2614 deletions**. Per-task attribution in the message (TASK-434 §3 precedent) for the multi-owner files |
| 2 | **`97c7c8d`** | **TASK-445 assets** — `IA_AssistantConsole.uasset` (new) + `IMC_Hero.uasset`. **5 insertions / 2 deletions ⇒ LFS POINTERS, not binaries** |
| 3 | *(this commit)* | **Pipeline record** — CONVENTIONS, TASKBOARD, 18 handoffs, 5 QA reports |

### ⛔ THE HOSTILE INDEX WAS DEFEATED BY METHOD, NOT BY LUCK
`IA_AssistantConsole.uasset` was **pre-staged by GitHub Desktop / the editor's Git provider**. A bare
`git commit` would have swept it into the **code** commit. ⇒ ✅ **Every commit used
`git commit -F <msg> -- <explicit pathspec>`, which commits ONLY the named paths regardless of what
else sits in the index.** ⛔ **No `-a`, no `add -A`, no bare `git commit`.** Index re-read after every
`add`.

### ✅ INVERSE-FILTER VERIFICATION — BOTH COMMITS, EMPTY RESIDUE
`git show --name-only <sha> | grep -v -E '<intended paths>'` ⇒ **empty for `cd5f4ed` and `97c7c8d`.**
**Nothing foreign rode along.**

### ✅ THE OTHER GIT GATES
- **LFS, per-KIND rule applied** (the relayed blanket rule was wrong and is corrected on the board):
  `IA_AssistantConsole.uasset (LFS: 06b1b21)` · `IMC_Hero.uasset (LFS: 45d09c7 -> LFS: 25fb340)`.
  ✅ **Both `(LFS: …)`, which is CORRECT** — `git check-attr filter` ⇒ `lfs` for `*.uasset`. **All
  source objects are `(Git: …)`.** ⚖️ **A `.uasset` tagged `(Git: …)` would have been the defect.**
- ✅ **`Models/*.gguf` — acceptance, not assertion:** `git check-ignore -v Models/*.gguf` ⇒
  `GitClaudeUnrealTest/.gitignore:132:/Models/	Models/Qwen3-4B-Q4_K_M.gguf`. **No `.gguf` in any commit.**
- ✅ **`git reset --hard` / `git clean -fd` were never typed.** One git command per call, output never suppressed.
- ⛔ **NOT PUSHED.** `main` was **13 ahead** before this gate; these three commits make it **16**. **The push is Jonathan's.**
- 🔒 **`L_Arena` — SHA256 AT FOUR CHECKPOINTS, NEVER MTIME:** pre-flight · post-kill · post-build ·
  **post-PIE** — all `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268`, **535,522 B**.
  ✅ **PIE did not dirty it. It was never saved.**

---

## 7. ⛔ WHAT I COULD NOT VERIFY — STATED, NOT OMITTED

1. ⛔ **NO AUTOMATION TEST HAS EVER RUN.** 33 exist. ✅ They **compile**; ⛔ **compiling is not passing.**
2. ⛔ **WARN-5 / `Siegebound.Assistant.ZoneA.*` — STILL INSTRUMENTED, NOT DISCHARGED.** Discharge needs
   a **green run**, claimed against that run. **This gate does not move it**, and ⚠️ **Finding 1 gives
   real reason to think `TwoLaneByteEquality` may not pass on the first try.**
3. ⛔ **(0-ZONEB) NOT MEASURED** — see §2. **No substitute offered.**
4. ⛔ **EVERY SMOKE ROW NOT RUN** — tooling, see §4.
5. ⛔ **NO ON-SCREEN CLAIM WHATSOEVER.** Nothing was rendered or looked at. **TASK-438 not placed.**
   ⚠️ When it lands it must be reported **PLACED, never VERIFIED-LOOKING** — a readback cannot see geometry.
6. ⛔ **THE PLAYABLE CHECKS (step 4) — NONE RUN.** Settings-opens, backdrop-swallows-clicks,
   toggle-survives-relaunch, AwaitConfirm + ghosts, Cancel, non-orderable refusal, and ⛔ **the §2
   keyboard-identity check** are **all** open. **They need a human. That is TASK-448.**
7. ⚠️ **The 110 ms was read from the subsystem's own startup line, not independently timed at match
   start under load.**

---

## 8. BOARD FLIPS OWED (orchestrator applies — I do not edit the board)

- **TASK-436 · 437 · 440 · 441 · 442 · 443 · 444 · 445 · 449 · 450 · 451 · 453 · 454 · 455 · 456 ·
  457 · 459 · 423** → ✅ **`done`** (compiled, linked, committed in `cd5f4ed` / `97c7c8d`).
- **TASK-438** → ⛔ **stays open — dispatch `art-director`. The editor session is hosted and waiting (PID 9828).**
- **TASK-447** → ⚠️ **substantially done, but NOT fully:** compile ✅, commit ✅, audit ⚠️ **partial**,
  smoke ⛔, TASK-438 ⛔. **The manager should decide whether to close it with the residue recorded or
  keep it open for the owed readings.**
- **TASK-448** → ✅ **UNBLOCKED — this is the gate Jonathan's playtest now runs against**, and it
  additionally now owes **the one keypress that takes the Zone B reading.**

**New tasks the manager should consider:** Finding 1 (missing `DA_AssistantVocabulary`) · Finding 2
(the §8 guardrail's arming window) · Finding 3 (one-lane-per-session, written into the smoke spec) ·
the owed automation-test run.
