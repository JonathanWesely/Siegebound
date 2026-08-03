# TASK-414 — [LLM-1d] Integration commit — the LLM assistant lane

**Agent:** build-master · **Date:** 2026-08-03 01:35–01:44 · **Gate in:** `qa/TASK-416.md` §R8 (PASS, 0 blockers)
**Verdict: ✅ COMMITTED `66c6854` on `main` · NOT PUSHED · ahead 7 · compile GREEN · 12/12 automation PASS**
**`L_Arena` never opened, never saved. No `Content/` path in the commit. No `.gguf`. `reset --hard` / `clean -fd` never used.**

---

## 1. The compile — `Result: Succeeded`

Judged on the log's `Result:` line, never the exit code (standing law; raw exit was 0 but that proves nothing).

```
[1/5] Compile [x64] SiegeAssistantGrammarTest.cpp
[2/5] Compile [x64] SiegeAssistantGrammar.cpp
[3/5] Link    [x64] UnrealEditor-GitClaudeUnrealTest.lib
[4/5] Link    [x64] UnrealEditor-GitClaudeUnrealTest.dll
[5/5] WriteMetadata GitClaudeUnrealTestEditor.target
Result: Succeeded
Total execution time: 4.70 seconds
```

- **Zero diagnostics.** `grep -inE "error|warning|unresolved|LNK[0-9]|fatal"` over the whole log = **0 rows**.
- **UBT rebuilt exactly the two files QA said had changed** — the comment-only `SiegeAssistantGrammar.cpp` fix and
  the WARN-3 shared-predicate change in `SiegeAssistantGrammarTest.cpp`. Nothing else in `Source/` was newer than
  the previous binary. **Neither had ever been through a compiler before this run.** The link was reached.
- Binary `UnrealEditor-GitClaudeUnrealTest.dll` → **3,380,224 B @ 01:36:37**.
- **No plugin hold-out was needed this time.** `Plugins/SiegeLlama/Binaries/Win64/UnrealEditor-SiegeLlama.dll`
  already exists (built 2026-08-02 23:46), so the plugin compiles and TASK-420's disable/restore dance is retired.
  Plugin DLL untouched by this build (no plugin source changed).

**Editor bounce:** graceful `CloseMainWindow()` only — **never a force-kill**. PIE was not running, zero asset
editors open, no dirty `Content/` before or after. Editor relaunched detached afterwards (PID 31332).

## 2. The automation suite — 12/12, and `RuleNameCharset` is the point

Exact command on record (`handoffs/TASK-420-buildmaster.md:56`), **no extra flags** — TASK-420's `-noshadercompile`
crash was not reproduced because the flag was not added.

```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound.Assistant"
                                -TestExit="Automation Test Queue Empty" -unattended -nullrhi
```

⚠️ **The real log is NOT on stdout** — stdout carried only the SDK-validation preamble. Results are in
`Saved/Logs/GitClaudeUnrealTest.log`. Repoint any parsing script.

```
Found 12 automation tests based on 'Siegebound.Assistant'
...Automation Test Queue Empty 12 tests performed.
**** TestExit: Automation Test Queue Empty ****
```

| # | Test | Result |
|---|---|---|
| 1 | `Command.NeverPartiallyFills` | Success |
| 2 | `Command.Rejection` | Success |
| 3 | `Command.RoundTrip` | Success |
| 4 | `Command.SelectionInvariants` | Success |
| 5 | `Grammar.CountRange` | Success |
| 6 | `Grammar.DegenerateInputs` | Success |
| 7 | `Grammar.Determinism` | Success |
| 8 | `Grammar.Grounding` | Success |
| 9 | `Grammar.Intents` | Success |
| 10 | **`Grammar.RuleNameCharset`** | **Success** ← the new 12th |
| 11 | `Grammar.SelectionCap` | Success |
| 12 | `Vocabulary.SynonymTable` | Success |

**Counts, mechanically:** `Test Completed` = 12 · `Result={Success}` = 12 · `Result={Fail}` = **0** ·
Skipped/NotRun/Cancelled = **0**. Registered name confirmed in full as
**`Siegebound.Assistant.Grammar.RuleNameCharset`**. No automation Error/Warning rows.

⇒ **This closes NIT-R1.** The comment committed here redirects the Shipping-build guarantee from the runtime
log/ensure (which compile out under `NO_LOGGING` / `DO_ENSURE`) to this test. That redirect was a documented
intention; it is now **evidenced**. Was 11 tests at TASK-420, is 12 now — the delta is exactly the new one.

## 3. The commit — `66c6854`, 24 paths, scope RE-DERIVED not assumed

⚠️ **The planned A/B/C split did not apply.** Jonathan's `e70f5ba` (2026-08-02 17:22) had already committed
**commit A's** plugin + vendored `LlamaCpp` and **commit B's** two eval CSVs. Scope was derived from `git status`.
Verified already-landed: `git ls-files …/ThirdParty` = 32 files incl. **22 `.dll`/`.lib`**; both
`Docs/Data/assistant_eval_{dev,holdout}.csv` tracked. `Docs/Data/` was **not** dirty ⇒ correctly not restaged.

**The 24 staged paths** (staged via `git add --pathspec-from-file`, an explicit auditable list — never `-A`,
never `-a`, never a bare `git commit`):

- **Plugin (5):** `SiegeLlamaSpike.cpp` (new) · `SiegeLlamaModule.h` · `SiegeLlamaSettings.h` ·
  `LlamaCpp.Build.cs` · `LlamaCpp/VERSION.md`
- **Game code (8):** `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` ·
  `SiegeAssistantInputProbe.{h,cpp}` (new) · `Tests/SiegeAssistantGrammarTest.cpp` · `GitClaudeUnrealTest.Build.cs`
- **Project/tooling/docs (3):** `GitClaudeUnrealTest.uproject` · `Tools/fetch_llm_model.py` ·
  `Docs/ThirdPartyNotices.md`
- **Pipeline (8):** `CONVENTIONS.md` · `TASKBOARD.md` · `handoffs/TASK-410-programmer.md` ·
  `handoffs/TASK-411-programmer.md` · `handoffs/TASK-413-buildmaster.md` · `qa/TASK-411.md` · `qa/TASK-412.md` ·
  `qa/TASK-416.md`

**Inverse filter on the landed commit: EMPTY.** Nothing outside those path prefixes. No `Content/`, no `.umap`,
no `L_Arena`, no `.gguf`, no `Models/`.

## 4. The hard proofs

**⛔ The 2.5 GB model is NOT committed.**

```
$ git check-ignore -v GitClaudeUnrealTest/Models/Qwen3-4B-Q4_K_M.gguf
GitClaudeUnrealTest/.gitignore:132:/Models/	GitClaudeUnrealTest/Models/Qwen3-4B-Q4_K_M.gguf
```

Run **before** staging. On-disk size 2,497,280,256 B. `git ls-files` shows zero `Models/` paths. The single
`gguf`-matching tracked path is **`…/LlamaCpp/include/gguf.h`, an 11,355 B vendored C header** — not the model.

**⚠️ LFS pre-flight — pointers UNDISTURBED, nothing re-added.**

```
$ git check-attr filter -- …/bin/Win64/llama.dll   → filter: lfs
$ git check-attr filter -- …/bin/Win64/ggml.dll    → filter: lfs
$ git check-attr filter -- …/lib/Win64/llama.lib   → filter: lfs
$ git check-attr filter -- <every text file staged> → filter: unspecified
$ git status --porcelain -- …/LlamaCpp/bin/ …/LlamaCpp/lib/   → EMPTY (untouched)
```

`git lfs status` listed all 24 staged paths as `Git: → Git:` (plain blobs); **no LFS object entered this commit**.
Sample HEAD pointer verified intact: `llama.dll` = a **132 B** pointer for a 2,795,008 B payload ⇒ `e70f5ba`'s
repair of `20c8e48`'s 70.7 MB raw-blob sweep is holding.

**⚠️ `.uproject` — a real repair, and a false alarm ruled out.**
`e70f5ba` captured the `SiegeLlama` entry as **`"Enabled": false`** (a hold-out snapshot), which would leave the
plugin disabled for everyone. Restored to `true`. Disk file **859 B / sha256 `63058f3c…`** — byte-identical to
TASK-420's recorded good restore. `git lfs status` displayed a scary `ca5c2cf` (TASK-420's CRLF-corruption hash),
but the authoritative plumbing refutes it: **HEAD blob 806 B → staged blob 805 B (1 byte = `false`→`true`), and
`git diff --cached` shows exactly ONE changed line.** A line-ending change would have shown all 54. The repo has
always stored this file LF-normalised; 859 − 805 = 54 = the CRLF bytes on disk. **Not the TASK-420 defect.**

## 5. For TASK-434 (the next build-master commit) — READ THIS

1. **Left deliberately uncommitted, because they are yours, not mine:**
   `qa/TASK-433.md` (untracked) and a **one-line `TASKBOARD.md` change** (TASK-433's status), both written by the
   TASK-433 reviewer at 01:40–01:41, **during my staging window**. My commit captured the board one line stale;
   that is intentional, not a clobber — I never wrote the board before committing.
2. **Plus my own board edit** (TASK-414 → `done` with hash `66c6854`) and **this handoff**, also uncommitted.
3. ⚠️ **`TASK-433`'s audit raises 2 BLOCKERS against `SiegeAssistantSnapshot.cpp` — a file this commit ships.**
   They are NOT a defect in this commit: my gate was `qa/TASK-416.md`, which is green, and TASK-433's report is
   explicitly committable docs. **They are follow-up work, and the orchestrator/manager owns turning them into
   tasks.** Do not treat `66c6854` as needing a revert.
4. **The editor is back up (PID 31332) and its Git provider auto-stages saved assets.** Re-check
   `git status --porcelain` and rebuild the staged set explicitly. Do not trust the index.
5. **`main` is ahead 7 and UNPUSHED. Jonathan's call, and he is asleep. Do not push.**

## 6. What did NOT get proven here

- **No `llama_sampler_init_grammar` call was made in this task.** The grammar fix's runtime proof belongs to the
  spike run already recorded in `handoffs/TASK-413-buildmaster.md` §13a/§13b (the vendored `llama-completion.exe`
  parsed the renamed grammar clean and rejected the `at_least` original). I ran no inference here.
- **WARN-4's freebie was NOT taken** — I did not paste a digit-bearing rule name into `llama-completion.exe`. The
  binary was not out this pass and the compile+test gate was the commissioned work. **Still open, still cheap.**
- `USiegeAssistantSnapshot` remains **read, never run** (TASK-433's standing gap) — the 12 automation tests cover
  the grammar/command/vocabulary lanes, not the snapshot builder.
