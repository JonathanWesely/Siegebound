# QA Report — TASK-412 (LLM-QA1, the spike lane)

**Reviewer:** qa-reviewer · **Date:** 2026-08-02 · Pre-compile review, no code edited, no engine, no Git.

## Verdicts

| Task | Verdict | Blockers | Warns |
|---|---|---|---|
| **TASK-409** — plugin scaffold + vendored llama.cpp + licences + fetch tool | **PASS** | 0 | 4 |
| **TASK-410** — the spike harness | **⛔ FAIL** | **3** | 6 |
| **TASK-426** — the sealed corpus (structural re-verification) | **PASS** | 0 | 1 |
| TASK-411 — input probe | **NOT REVIEWED IN DEPTH** (see Scope) | — | — |

**Gate verdict: FAIL — TASK-413 must not run until BLOCKER-1..3 land.** Two of the three blockers make a
go/no-go number *flatter the result*, which is the specific failure this gate exists to catch.

---

## Scope of this pass, stated honestly

The dispatch scoped me to `Plugins/SiegeLlama/**` + `Docs/ThirdPartyNotices.md` + the corpus checks. The
board's TASK-412 entry says the gate covers **409 · 410 · 411 · 426**. I reviewed 409 and 410 in full, and
re-derived every §11 anti-self-grading check against the corpus files (426). **TASK-411
(`SiegeAssistantInputProbe.{h,cpp}`) got only a targeted confirmation of the mandatory items**, not a full
review: no `UFUNCTION(exec)` (the one `UFUNCTION()` at `:299` is the text-committed delegate binding),
`FAutoConsoleCommandWithWorld` registration at `:1096`/`:1101`, the M8 declaration verbatim at `:42`, and the
C4458 fix confirmed **on disk** at `SiegeAssistantInputProbe.cpp:420` (`Walker`, not `Cursor`). **Do not read
this report as a full gate on TASK-411.**

⚠️ **I opened `assistant_eval_holdout.csv`.** Board item (13) requires QA to verify its row count, header and
adversarial coverage, and ruling 14 binds the *prompt author*, not the reviewer. **No holdout sentence, expected
value or per-row content is quoted anywhere in this report**, and I make no prompt recommendation derived from it.

---

## ⛔ BLOCKERS (all three in `SiegeLlamaSpike.cpp` — TASK-410)

### BLOCKER-1 — Bar #3 measures a KV reuse the shipped path can never reach, and bar #2's warm TTFT inherits it
`SiegeLlamaSpike.cpp:2450-2492` (`RunBenchJob`) + `:391-427` (`AppendZoneC`)

Turn 1 and turn 2 are built from the **same Zone B and the same Zone C fixture**; `AppendZoneC` takes only the
utterance and an (always empty) pending line, so the two prompts differ **in the `order:` line alone** — which
sits at the very END of Zone C.

`RunGeneration`'s common-prefix walk (`:2081-2087`) therefore lands the reuse point *inside Zone C's tail*, not
at a zone boundary. Consequences, both in the flattering direction:

- **Turn-2 prefill ≈ the utterance (~20-25 tok) instead of the shipped Zone B+C (~265 tok).** Reported DROP will
  be **~97-98 %** against a bar of ~70 %. TASK-416's reference measurement for the *shipped* layout is
  **165/765 = 78 %** (CONVENTIONS §8), and §8 records 68 % as a MARGINAL FAIL. The spike will report a margin
  that does not exist.
- **Bar #2's warm-prefix TTFT is measured with ~245 tokens of prefill that the shipped path pays every turn.**
  On the CPU-only tier that is hundreds of milliseconds removed from the number Jonathan's tolerance is judged
  against. Bar #2 is the bar the whole feature lives or dies on.
- The file's own comment at `:2076-2080` states the expectation — *"the reuse point should land at the start of
  Zone C. If it does not, the LAYOUT is wrong"* — and the fixture violates it. The only guard
  (`DropPercent < 60.0`, `:2487`) fires on **too little** reuse and is silent on implausibly **too much**.

**Fix (all in this file):** give turn 2 a *second* Zone B+C fixture whose roster counts / stances / HP bands
differ, as they would after a few seconds of play, so the divergence lands at the start of Zone B (the shipped
worst case) — and keep the current constant-fixture run as the declared best case, reporting **both bounds**.
Then print **where the divergence landed** (token index against the Zone A / Zone B / Zone C boundaries) on the
`SPIKE_PREFILL` line, and warn when it lands past the start of Zone C. That turns bar #3 from a number into
evidence. The same applies to the per-row `prefill=`/`reused=` columns in `RunOneSplit` (accuracy is unaffected).

### BLOCKER-2 — The offload device is never pinned, and bar #4's VRAM is sampled from an *assumed* device
`SiegeLlamaSpike.cpp:2334-2343` (`EnsureModelLoaded`) + `:1913-1940` (`SampleMemory`)

`EnsureModelLoaded` sets only `n_gpu_layers` and `load_mode`. It never sets `devices`, `split_mode` or
`main_gpu`. The vendored header is explicit (`llama.h:307`): *"NULL-terminated list of devices to use for
offloading (**if NULL, all available devices are used**)"*, and `llama_split_mode` (`llama.h:198-203`) defaults
to a layer split across GPUs.

**This is not hypothetical on the machine TASK-413 will run on.** TASK-409's own captured link proof
(`VERSION.md:124-140`) enumerates **three devices: `Vulkan0 type=iGPU` (Intel Arc 140T), `Vulkan1 type=GPU`
(RTX 5070), `CPU`.** Meanwhile `SampleMemory` skips every device whose type is not
`GGML_BACKEND_DEVICE_TYPE_GPU` and takes **the first discrete GPU it finds** (`:1922-1934`) — an assumption,
not a measurement.

So on `tier=full`:
- if the weights split across the iGPU and the dGPU, the free-VRAM low-water mark on the dGPU understates the
  model's footprint — **and bar #4's only question is "does it fit the modal 8 GB card"**, so the error is in
  the direction that manufactures a PASS;
- bars #1 and #2 are then measured on an iGPU+dGPU split that no player has and the shipped design would not
  choose.

**Fix:** add a `gpu=<index>` argument; set `ModelParams.split_mode = LLAMA_SPLIT_MODE_NONE` and
`ModelParams.main_gpu` (or build an explicit one-entry `devices` array) so the tier means one named device; and
log **every** device's name/type/free/total before and after load, plus **which device the memory sampler is
reporting on**, so the number's subject is printed on the line rather than inferred.

### BLOCKER-3 — A model swap is silently ignored: `EnsureModelLoaded` never compares the model path
`SiegeLlamaSpike.cpp:2306-2313`

```cpp
if (GRunner.IsLoaded()
    && GRunner.LoadedOptions.GpuLayers == Options.GpuLayers
    && GRunner.LoadedOptions.ContextTokens == Options.ContextTokens
    && GRunner.LoadedOptions.UBatch == Options.UBatch
    && GRunner.LoadedOptions.Threads == Options.Threads)
{
    return true;
}
```

`GRunner.LoadedModelPath` is stored at `:2380` and **never read**. `Siege.Llama.SpikeBench model=<B>` after
`model=<A>` at the same tier keeps model **A** resident and prints A's numbers under B's command line — with
**no `SPIKE_LOAD` line at all**, because no load happened. `USiegeLlamaSettings.h:10-15` advertises the whole
override chain as *"swapping models never needs a rebuild"*, and TASK-413's ladder includes a bigger model.

**Fix:** one line — add `&& GRunner.LoadedModelPath == Options.ModelPathOverride` to the condition.
*(Acceptable alternative if the orchestrator prefers not to reopen the file: TASK-413 is instructed to run
`Siege.Llama.SpikeUnload` between every model swap. I do not recommend it — a procedural mitigation for a
silent-wrong-measurement is exactly the shape that has burned this project.)*

---

## ✅ What I verified rather than accepted — the measurement-integrity checks

Every item below was **re-derived from the artifact on disk**, per the relayed-diagnosis law. Where a relayed
claim was correct I say so; where I could not corroborate, I say that too.

**1. The sealed corpus — CONFIRMED CLEAN.**
- Loaded **by path at runtime** (`FFileHelper::LoadFileToStringArray`, `:1254`), default
  `FPaths::ProjectDir()/"Docs/Data/assistant_eval_dev.csv"` (`:3142`). **Never `#include`d**; the plugin gains
  no Siegebound dependency. Grepped the whole `Plugins/` tree: no corpus content is compiled in.
- **`holdout=` has no default** (`:3147`) and is the only route to the sealed file (`:2717-2727`). Nothing else
  in the plugin opens it, defaults to it, or prints it.
- **Two splits scored and reported separately** (`RunOneSplit` per split; `SPIKE_EVAL_SCORE split=dev` /
  `split=HOLDOUT`). **No blended number exists anywhere.**
- Row counts and headers: **dev = 25 rows, holdout = 15 rows, headers identical character-for-character** to the
  §11 pin. ✓
- Intent extraction: `ExtractIntentPrefix` (`:1197-1226`) implements `^intent=([a-z]+);`. I checked all
  **25/25 dev rows** carry the prefix, values all inside {7 intents, `none`, `unasserted`}. `unasserted` →
  intent skipped, `none` → asserted (`:1348`, and `:1525` correctly fails a command-shaped output on an
  `intent=none` row). ✓
- **Empty cell = SKIPPED, never matched** — `bKindsAsserted/bCountsAsserted/bWhereAsserted` are set from
  non-emptiness (`:1328-1331`) and the skip counters increment on the empty branches. ✓
- Place spellings: the seven §9a symbols appear character-for-character in `SpikePlaces` (`:365-374`) and in
  Zone A (`:236-242`). **`own_castle`, not `my_castle`**; `nearest_mine` present. Corroborated against the
  files that assert them: dev asserts `own_castle` and `nearest_mine`; the holdout asserts both as well. `hero`
  is asserted by **no** row in either file — CONVENTIONS §9a's note is correct.
- Adversarial coverage present in **both** splits: ambiguous quantity, unit-not-in-roster, the Sorcerer/Wizard
  collision (two rows per split, including one that names both cards in one sentence), selection-verb-mixed-
  with-army-wide, multi-kind including an over-cap row, deferred trigger, and `Clarify`/`Refuse` rows. ✓
- **The corpus files were not edited by TASK-410**: the runner reads them, and the only write path in the
  plugin is none.

**2. The leniency disclosure — ALL THREE NUMBERS INDEPENDENTLY RE-DERIVED AND CORRECT.**
I counted the dev file by hand rather than trusting the handoff:
- **13 Execute · 9 Clarify · 3 Refuse = 25.** A model answering `{"ask":…}` to everything passes exactly the
  non-Execute rows under the primary rule (verified against `ScoreRow`: `Refuse`→question passes;
  `Clarify`→`bOutcomeOk=true` and fields skipped). **Floor = 12/25 = 48 %. CONFIRMED.**
- **6 dev rows assert ≤ 1 field: DEV-04, DEV-06, DEV-08, DEV-09, DEV-10, DEV-25.** Exactly the six named.
  **CONFIRMED.**
- **DEV-08 asserts nothing** (`intent=unasserted`, no kinds/counts/where) and passes on any parseable output.
  **CONFIRMED.**
- The runner computes all three at run time (`DegenerateQuestionFloor` `:2582`, thin/zero-row lines
  `:2688-2700`), so they are printed, not promised. The strict score is printed on the same line as the primary
  (`:2667-2672`). **This is the disclosure that makes the accuracy number honest, and it is correct as built.**
- ⚠️ See WARN-9: **the holdout's floor is materially higher than 48 %.** Do not carry dev's floor across.

**3. Threading — RULED: the deviation is ACCEPTED, and bar #1 has an answer.**
The spec's "no `FRunnable`" sits inside an enumeration whose stated purpose is *"do not build TASK-423's
production subsystem"* — no worker class exists here. I checked the engine source rather than the claim:
`Engine/Source/Runtime/Core/Public/Async/Async.h:430-448` shows `AsyncThread(Callable, StackSize, ThreadPri, …)`
calling `FRunnableThread::Create(Runnable, Name, StackSize, ThreadPri)` — **a real dedicated OS thread at the
requested priority**, not a task-graph worker whose priority would be ignored. `TPri_BelowNormal` therefore
genuinely holds, which is the exact shape CONVENTIONS §8 pins for the shipped subsystem. A game-thread-
synchronous spike would report one multi-second frame and manufacture a NO-GO the shipped design never hits.
**The harness may keep the background thread; bar #1 is answerable.**

**4. Frame-time methodology — CORRECT.**
`BuildHistogram` (`:1740-1789`) sorts and reports **WORST** (`Values.Last()`) and **p99**; `LogHistogram`
(`:1802-1806`) prints `WORST=` and `p99=` first and the mean **last, labelled "NOT the bar"**. Buckets are
printed too. A **baseline control phase** is captured and every inference phase gets an explicit
`ATTRIBUTABLE worst_delta / p99_delta` line against it (`:2963-2967`), with a loud warning when the control
window came up short (`:2908-2913`). Sampling is on the game thread via `FTSTicker`, doing one
`FPlatformTime::Seconds()` and one append; RSS every 30th frame; **device VRAM is never queried on the game
thread** — I confirmed `SampleMemory` is called only from `EnsureModelLoaded` and `RunGeneration`, both worker.
Tiers are a command argument, so all three tiers are reportable from one build. ✓ (See WARN-3 for the one
caveat on the baseline.)

**5. Zone A parity — TRANSCRIPTION IS FAITHFUL. I diffed it, I did not read it.**
Against `USiegeAssistantSnapshot::BuildZoneA` (`SiegeAssistantSnapshot.cpp:538-674`),
`USiegeAssistantVocabulary::BuildSynonymTable` (`SiegeAssistantVocabulary.cpp:182-206`) and its constructor
defaults, and the `PlaceVocabulary` table (`SiegeAssistantSnapshot.cpp:52-61`):
- `[RULES]` / schema (command) / schema (question) / intents / places / rules / synonyms / examples — **same
  blocks, same order, same trailing newlines**.
- The seven place lines are `symbol = description` in `EPlaceSlot` order, **descriptions identical**.
- The synonym block reproduces `BuildSynonymTable`'s normalisation exactly: `SYNONYMS` header, `[units]`
  `[places]` `[intents]` `[notes]`, rows sorted by canonical, aliases lower-cased/de-duped/canonical-removed/
  sorted by string. I checked all 13 unit rows, 7 place rows, 7 intent rows and all 4 notes lines. **No drift.**
- The `ancient_ground_near <- ancient ground, …` alias that `qa/TASK-419.md` WARN-6 orders removed is present
  **because it is on disk today**, and the deviation is declared and is **pessimistic** for the author's own
  score. That is the right call and I endorse it.
- **Char arithmetic independently confirmed:** Zone B = **68 chars exactly** and Zone C = **887 chars exactly**
  for the default order line (I counted both from the source strings) ⇒ **B+C = 955 of 1440, headroom 485.**
  The handoff's figures are real, not estimated.
- **Zone A ~595 tok without the synonym table vs ~1100-1200 tok with it: the mechanism is confirmed** — the
  landed `BuildZoneA` emits `synonyms:\nnone\n` when `Vocabulary` is null (`:648-651`), which is exactly the
  state §8's "~600 tok as-built" was measured in, while the spike hardcodes the full 2,172-char table. So
  §8's figure and TASK-410's figure are both right about different inputs. **Bar #3's ratio does improve and
  turn-1 cold prefill does roughly double.** ⚠️ The token counts themselves are *not* verifiable pre-run —
  `Siege.Llama.SpikePrompt` must print them and TASK-413 must quote the printed numbers, not these.

**6. The Zone-A ↔ GBNF seam (§9c) — PASSES, shown by derivation not by eye.**
The spike's generator (`BuildSpikeGrammar`, `:529-693`) is a rule-for-rule mirror of
`USiegeAssistantGrammar::Build` (`SiegeAssistantGrammar.cpp:165-402`): same rule set and emission order
(`root, command, question, ask, intent, kind, where, count, at_least, item, selection, who, when`), same
`GbnfTerminal`/`GbnfJsonString`/`JsonObjectOpen`/`JsonNextKey` construction. The four sketch traps are all on
the landed side: key `who` (not `select`), per-pair key **`n`** (`count` is the rule name), army-wide ⇒
`"who":"none"` (not `[]`), and the `{"ask":ASK}` branch exists and Zone A teaches it. Ask codes and intent
symbols match `SiegeAssistantAskCodes` / `SiegeAssistantIntentSymbols` in order.

Running each Zone-A few-shot through the grammar:

| few-shot JSON | derivation | result |
|---|---|---|
| `{"intent":"send","who":[{"kind":"footman","n":10},{"kind":"sorcerer","n":1}],"where":"ancient_ground_near","when":"now"}` | `command` → intent`"send"` · who→selection(2 items) · kind∈13 · count 10∈1..30, 1∈1..30 · where∈7 · when`"now"` | **ACCEPTED** |
| `{"intent":"guard","who":[{"kind":"archer","n":"all"}],"where":"mid","when":"now"}` | selection(1 item) · count sentinel `"all"` present in the `count` rule | **ACCEPTED** |
| `{"intent":"charge","who":"none","where":"none","when":"now"}` | `who`→`"none"` · `where`→`"none"` | **ACCEPTED** |

All three contain **no whitespace between terminals**, which matters because GBNF concatenation is exact and the
command rule has no separator rule. ✓ `count` is **1..30 and not the live max** in both the spike (`:146-147`,
`:615`) and the landed grammar (`:290`). The 3-kind cap is a **bounded alternation in the grammar** in both, and
the spike's parser also rejects arity outside 1..3 (`:918-922`) rather than truncating.

**7. The central law — no multi-turn loop. VERIFIED BY ARITHMETIC, not by comment.**
Turn N leaves the cache holding prompt positions `0..P-1` plus generated positions `P..P+k-1`. Turn N+1 computes
`CommonPrefix = C ≤ P` and calls `llama_memory_seq_rm(mem, 0, C, -1)` (`:2102`), which the vendored header
(`llama.h:730-739`) documents as removing **all** positions in `[C, ∞)` — i.e. the divergent prompt tail **and
every generated token**. `GRunner.LastPromptTokens` holds prompt tokens only (`:2272`). **No model output is
ever fed back to a model.** What is reused is a KV cache of a byte-identical prefix — a cache, not a
conversation. The `seq_rm == false` path clears the whole cache and declares the turn's reuse `0 by
construction, not by measurement` (`:2102-2110`) — correct, and the honest way to report it.

**8. The comment trap — RE-RUN, PASS (0 findings).**
I did **not** take the reported PASS. I read the raw content of all ten first-party plugin files
(`SiegeLlama.uplugin`, both `Build.cs`, `SiegeLlamaLog.h`, `SiegeLlamaModule.{h,cpp}`,
`SiegeLlamaSettings.{h,cpp}`, `SiegeLlamaInfo.cpp`, `SiegeLlamaSpike.cpp` — all 3,275 lines) and checked every
block comment for a premature terminator. **`SiegeLlamaModule.h:38` is fixed** (`GATE EVERY llama_ AND ggml_
CALL ON THIS`) **and the fix does not reintroduce the trap** — the explanatory prose at `:39-40` describes the
sequence in words ("an asterisk immediately followed by a slash") without writing it. The same discipline holds
at `SiegeLlamaSpike.cpp:2777-2782` and `LlamaCpp.Build.cs:64-69`. The single-line `/*ParamName*/` idiom is used
throughout and is not the bug signature. **COMMENTSCAN: PASS (0).**
⚠️ **Tooling note for whoever re-runs this:** the Grep tool's *rendering* mangles `//` and `/`-adjacent
sequences in its output (it displayed `// Charge / Fallback / Rally` as `\ Charge \ Fallback \ Rally` in a file
that is perfectly valid on disk). **Comment-syntax analysis must be done on raw file reads, never on grep
output** — a scan done through grep would produce false positives *and* could mask a real one.

**9. The two API facts Wave 1 depends on — BOTH CONFIRMED against the vendored headers.**
- **`use_mmap` no longer exists.** `struct llama_model_params` (`llama.h:306-322`) has **no `use_mmap`**; it
  carries `enum llama_load_mode load_mode`, and `LLAMA_LOAD_MODE_MMAP = 1` (`llama.h:205-211`). The spike sets
  `ModelParams.load_mode = LLAMA_LOAD_MODE_MMAP` (`:2343`). **The requirement survives, the field name changed.**
  **TASK-423's spec must be corrected before dispatch** or it will go looking for a field that is gone.
- **`abort_callback` is documented CPU-only.** `llama.h:382-385`: *"Abort callback / if it returns true,
  execution of llama_decode() will be aborted / **currently works only with CPU execution**"*. So on the
  full/partial GPU tiers the 10 s hard deadline can only fire **between** graph submissions and **the token
  budget is what actually bounds a GPU run.** CONVENTIONS §8 calls `abort_callback` "the only correct
  mechanism" — on GPU it is **necessary but not sufficient**, and §8 wants that footnote.
- Bonus, also confirmed: `llama_decode` returns `0/1/2/-1` (`llama.h:964-978`) and the spike's
  `bAborted = (result == 2)` is right; `llama_batch_get_one` fixes seq 0 and auto-tracks positions
  (`llama.h:929-937`), so the sliced prefill resumes at the right position after `seq_rm`.

**10. `FAutoConsoleCommand` law — CLEAN.** Six `FAutoConsoleCommandWithWorldAndArgs` in
`SiegeLlamaSpike.cpp:3240-3274` and one `FAutoConsoleCommand` in `SiegeLlamaInfo.cpp:125`. **No `UFUNCTION(exec)`
anywhere in the plugin.** The plugin's only `UCLASS` is `USiegeLlamaSettings` (`UDeveloperSettings`, two
`UPROPERTY(config, EditAnywhere)`, zero `UFUNCTION`). `USiegeCheatManager` and `ASiegePlayerController` are
untouched.

**11. Licensing — PASS with the content-lock condition stated.** `Docs/ThirdPartyNotices.md` carries llama.cpp's
MIT text verbatim (§1) and `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/LICENSE` exists. §2 is an explicitly
marked **PLACEHOLDER owned by TASK-413** and is called a shipping blocker in the file itself. **xLAM-2 /
Hammer 2.1 / Arch-Function appear only on the banned list** — I grepped the whole repo, and every occurrence in
code, script or doc is a prohibition, never a default. The only model default anywhere is
`USiegeLlamaSettings::DefaultModelFileName = "Qwen3.5-4B-Instruct-Q4_K_M.gguf"` (cleared family, marked
PROVISIONAL). ⚠️ **The exact quant's card is still unverified and that is TASK-413's blocking duty — a
`…-GGUF` re-upload carries a different card from the publisher's.** `libomp140.x86_64.dll` is flagged for its
own entry; agreed, confirm at content lock.

**12. `VERSION.md` — PASS, all three required elements present.** Tag `b10235` + commit
`221f0f6356efe2260023208365705ec5d5a7c8f5` + artifact SHA-256; build flags (Vulkan+CPU **not CUDA**, Win64
Release, `/MD` verified by import table, defines); **both** toolsets (libs 14.38 / engine 14.50); the import-lib
regeneration recipe; and the warning is there in as many words: *"Do not 'fix' the import libs to match the
engine toolset."* The three-toolset clean link is the C-API-only ruling's immunity proven under a harder
condition than specified — correctly recorded.

**13. Remaining board checks.** `common/` neither vendored nor linked (`LlamaCpp.Build.cs` links only
`llama.lib`/`ggml.lib`/`ggml-base.lib`; `bin/` excludes `llama-common.dll`, `mtmd.dll`, `ggml-rpc.dll`) ✓ ·
**no `HTTP`, no `Sockets`** ✓ · Vulkan+CPU ✓ · DLL trap complete (`PublicDelayLoadDLLs` for the 3 imported DLLs,
`RuntimeDependencies` for all 19, explicit `GetDllHandle` in dependency order inside `Push/PopDllDirectory`,
`FreeDllHandle` in reverse, full-set-present precondition, **one Warning line then inert — never a crash, never
a blocked editor**) ✓ · no unit registry / actor cache / dirty flag ✓ · symbols only, no coordinate or timestamp
in any zone ✓ · every `FString::Printf` / `UE_LOG` / `Appendf` format string is a literal ✓ · SEH via a thunk
with no destructible locals, the correct MSVC pattern ✓ · complete-type includes present for `UWorld`,
`FTSTicker`, `FPlatformMemory`, `FPlatformMisc`, `FFileHelper`, `FParse`, `FCommandLine`, `IPluginManager` ✓ ·
**the plugin declares no replicated property, no replicated class, no relevancy tier**, stated verbatim in
`SiegeLlamaModule.h:21`, `SiegeLlamaSettings.h:26`, `SiegeLlamaInfo.cpp:11`, `SiegeLlamaSpike.cpp:21` and both
handoffs ✓.

**Ruling on TASK-409 flag (b):** `Projects` and `DeveloperSettings` are **ACCEPTED**. Both are mandated by the
spec's own requirements (`IPluginManager` for the plugin base dir; `UDeveloperSettings` for the override chain),
neither is `HTTP` nor `Sockets`, and the closed decision the prohibition protects — no sidecar, no socket, no
firewall prompt — is intact: this plugin makes no network call.

**Evidence note, in TASK-410's favour:** `SiegeLlamaSpike.cpp` has **already compiled and linked under UBT with
warnings-as-errors** (TASK-409 second pass, `[24/33] Compile [x64] SiegeLlamaSpike.cpp`, zero diagnostics; the
fourth pass found all outputs current, implying the sources have not changed since). Compile-class findings are
therefore largely settled by evidence, which is why this review concentrated on semantics. All three blockers
are semantic — they compile perfectly and produce the wrong number.

---

## ⚠️ WARNINGS

- **[WARN-1] `SiegeLlamaSpike.cpp:2445` — a bench utterance is byte-identical to a HOLDOUT sentence.**
  `SpikeBenchUtterances[4]` = `"everyone pull back to our castle"` is verbatim the `Sentence` of one sealed
  holdout row, while the array's own comment (`:2435-2437`) says *"Deliberately NOT corpus sentences — the bench
  must not become a second, unscored evaluation, and no corpus row may leak into a tuning surface."* The file
  contradicts itself. Bar #5 is unaffected (bench output is never scored), and the phrasing is a natural
  independent derivation from the vocabulary's own aliases (`fallback <- pull back`, `own_castle <- our
  castle`) — **I am not alleging the holdout was read.** But a bench run prints the model's raw answer to a
  sealed sentence *before* the holdout is opened, which is an incidental pre-exposure. **Fix: change that one
  string** (the file is being reopened for the blockers anyway). The other four bench utterances are clean
  against both files.
- **[WARN-2] `:2359`, `:2381`, `:2063` — requested params used where the header says to query the actual ones.**
  `llama.h:551-556` states plainly: *"After creating a llama_context, it is recommended to query the actual
  values using these functions … the requested values via llama_context_params may differ from the actual
  values used."* `GRunner.BatchSize` is set from the *requested* `n_batch` and the prompt-budget pre-check uses
  the *requested* `ContextTokens`. If either is clamped, a prefill slice larger than the effective `n_batch` is
  rejected outright and **every bar reads "inference failed"** — the exact failure class §5(f) of the handoff
  says it already fixed once. Two lines: `GRunner.BatchSize = (int32)llama_n_batch(ctx);` and the same for
  `llama_n_ctx`.
- **[WARN-3] `:2885-2906` — the baseline control window is captured *before* the model is loaded on the first
  run of a tier.** `StartJob` sets phase `Baseline`, the worker waits for the frames, and only then calls
  `EnsureModelLoaded`. The handoff claims the control is taken *"with the model already resident"*; that is true
  only from the second invocation at the same tier. Low impact on frame time, but the `load`-phase
  `ATTRIBUTABLE` delta is then computed against a baseline taken in a different memory state. **TASK-413: run
  each tier's bench twice and report the second.**
- **[WARN-4] `:1536-1546` — the per-row diff blames the wrong field.** `KindSetMatches` returns one bool for
  kinds *and* counts, and `bCountsOk = bKindsOk`, so a row where the model named the right unit but the wrong
  quantity prints `kinds=BAD counts=BAD`. Pass/fail is unaffected; the **remediation ladder is misdirected**
  ("the model can't name units" when it can't count). The per-row diff table is one of TASK-413's deliverables.
- **[WARN-5] `:347-362` — bar #5's accuracy is measured on a 13-kind roster the shipped snapshot truncates to
  8.** The deviation is declared and is the right call for the spike (honouring `MaxRosterKinds = 8` would have
  converted six *model* results into six *harness artefacts*). But it makes the number **conditional**: on a
  live board with more than 8 kinds alive, `USiegeAssistantSnapshot` prints 8 and collapses the tail while the
  grammar's `kind` alternatives stay uncapped, so the shipped accuracy on such boards will be **worse than
  measured.** ⇒ **The measured bar #5 is valid only if TASK-416's `MaxRosterKinds` seam is closed.** State that
  beside the number.
- **[WARN-6] `Tools/fetch_llm_model.py:59, 84` — the token-stripping host check is too loose.**
  `ALLOWED_AUTH_SUFFIXES` contains bare `"huggingface.co"` and `"hf.co"`, and `host.endswith(...)` therefore also
  matches `evilhuggingface.co` / `myhf.co`, so a redirect to a look-alike host would **keep the `Authorization:
  Bearer` header.** Low probability (the redirect comes from HF over TLS) but this is a secret, and the fix is
  one line: compare `host == "huggingface.co" or host.endswith(".huggingface.co")` (same for `hf.co`).
  **Everything else in the script is clean:** `HF_TOKEN` is env-only and never printed (`--check` reports
  presence only; the 401/403 message reports `SET`/`NOT SET`, never the value); every remote call has an
  explicit generous timeout (60 s metadata, 120 s download, 30 s probe); failures are surfaced with the HTTP
  code and actionable text, never swallowed; writes are confined to `--dest` (default `<ProjectRoot>/Models`)
  and `Path(filename).name` blocks a `../` traversal in the remote filename; resumable via HTTP Range with a
  216/416 handling path; sha256 verified against HF's LFS oid with a loud warning when unavailable; `--check`
  exists; exit codes 0/1/2/130 with explicit `CHECK_VERDICT:` / `FETCH_VERDICT:` lines. No banned model is a
  default.
- **[WARN-7] `.gitignore` now has TWO GGUF/Models blocks from two authors, and TASK-409's acceptance evidence is
  stale.** Lines 30-43 (`*.gguf`, `Models/*`, `!Models/.gitkeep`, `!Models/README.md`) sit above TASK-409's block
  at 120-132 (`!…/lib/**/*.lib`, `!…/bin/**/*.dll`, `*.gguf`, `/Models/`). **Last matching pattern wins, so
  `/Models/` at :132 kills the `!Models/README.md` and `!Models/.gitkeep` negations** — git will not descend
  into an ignored directory to honour a `!` inside it (the top block's own comment says so). No subject today
  (`Models/` does not exist on disk), so this is latent. Separately, the handoff's pasted `git check-ignore -v`
  output cites `.gitignore:112 / :113 / :117`; those are now **:127 / :128 / :132**. I re-derived the behaviour
  by hand and **it is still correct** for all three acceptance paths — but **TASK-414 must re-run
  `git check-ignore -v` before committing**, because the file changed after the evidence was captured. (Noted
  without judgement: the top block references commits `20c8e48` / `e70f5ba` that the board does not record, and
  the repo-root `.gitattributes` now **does** carry `*.dll` / `*.lib` LFS rules — TASK-409 flag (c) appears to
  have been actioned by someone. TASK-414 still owes the `git check-attr filter` + `git lfs status` proof.)
- **[WARN-8] `SiegeLlamaModule.cpp:166-176` vs `SiegeLlamaSpike.cpp:2802-2813` — shutdown ordering is
  asymmetric.** `ShutdownSpike` (on `OnEnginePreExit`) correctly **declines** to free the model while a job is
  in flight, but `FSiegeLlamaModule::ShutdownModule` then frees the delay-loaded DLLs unconditionally, so an
  editor exit during a run can unload `llama.dll` under a live worker. Editor-exit only, SEH-covered on the
  worker, and this is throwaway code — naming it so nobody rediscovers it as a mystery shutdown crash.
- **[WARN-9] The holdout's leniency floor is materially higher than dev's 48 %.** I re-derived it from the
  sealed file; I am deliberately not printing the figure here, and the runner computes and prints it on the
  holdout's `SPIKE_EVAL_SCORE` line at run time. ⚠️ **TASK-413: read that line before writing the headline, and
  do NOT carry dev's 48 % across as if it applied to the holdout.** The gap between the floor and the 85 % bar
  is the part of the score that is actually evidence of translation, and on the holdout it is **narrower** than
  dev's numbers suggest. Reassuring counterpart: **no holdout row asserts zero fields**, so nothing there passes
  for free the way DEV-08 does.
- **[WARN-10] `:2207-2262` — the `grammar=0 tokens=60` "genuine fixed-length control" is not fixed-length.** The
  decode loop still breaks on `llama_vocab_is_eog`, so an unconstrained run may terminate well before 60 tokens
  and the "control" becomes another extrapolation. Say so on the line, or ignore EOG when `grammar=0`.

---

## NITs

- **[NIT-1]** `:2010` `Needed = -llama_tokenize(...)` — `llama.h:1151` documents `INT32_MIN` as the overflow
  return, and negating it is signed-overflow UB. Unreachable at these prompt sizes; noted for TASK-423, which
  inherits the idiom.
- **[NIT-2]** `:1904-1911` `SpikeAbortCallback` reads `GRunner.AbortDeadlineSeconds` (a plain `double`) from
  ggml's compute threads while the worker writes it. Benign on x64, but the handoff's §7 claim that "the only
  cross-thread state is two `FThreadSafeCounter`s" is not quite true — this is a third.
- **[NIT-3]** `:1913-1940` `SampleMemory` returns `VramFreeBytes = 0` when no discrete-GPU device exists, and
  the bench then prints `vram_free_lowwater=0.0 MiB`, which reads as a measurement rather than "n/a".
- **[NIT-4]** *(game lane, informational — not a finding against 409/410)* the landed `BuildZoneA`'s few-shots
  name `footman` / `sorcerer` / `archer` unconditionally, while the landed grammar's `kind` alternatives come
  from the live roster. On a board where one of those kinds is absent, Zone A teaches a symbol the sampler
  physically forbids — the §9c seam one level down, adjacent to WARN-5. The spike's 13-kind fixture masks it.
- **[NIT-5]** `.gitignore:43` `!Models/README.md` has no subject: `Models/` does not exist on disk, so TASK-409
  flag (d)(2) is still open in practice.

---

## Notes for build-master (TASK-413) — read before the run, and after the blockers land

1. **Do not run until BLOCKER-1..3 land.** Bar #3 and bar #2 are wrong in the flattering direction until
   BLOCKER-1 is fixed, and bar #4's subject is unnamed until BLOCKER-2 is.
2. **Report the HOLDOUT number against the 85 % bar. The dev number is a fitted number by construction.**
   Print both, and print the **strict** score and the **leniency floor** for **each split** beside them — they
   are on the same log line and they are what make the headline honest. **Dev's floor is 48 %; the holdout's is
   higher — quote the runner's own line, not dev's.**
3. **Subtract DEV-08 before believing the dev headline** (it asserts nothing and passes on any parseable
   output), and treat DEV-04, DEV-06, DEV-09, DEV-10, DEV-25 as ≤ 1-assertion rows. The runner prints the IDs.
4. **`Siege.Llama.SpikePrompt` before anything else** — quote its printed token counts when correcting
   `MaxSnapshotChars`; the char counts I verified (Zone B 68, Zone C 887, B+C 955 of 1440) are exact, the token
   counts are not knowable until the model is resident.
5. **`Siege.Llama.SpikeGrammar` and diff it against `USiegeAssistantGrammar::Build`** — I verified the
   generators match by construction, but the runtime dump is the artifact to paste.
6. **`Siege.Llama.SpikeLoad tier=cpu` first** to print the real `n_layer`, then pass `gpulayers=<n_layer/2>`
   explicitly for the partial tier; `tier=partial` guesses 18 before any load and says so.
7. **PIE on `L_Arena` with an army fielded on both sides.** The runner warns when the world is not a game world
   and records the map on the `SPIKE_RUN START` line — an empty-map number would be self-evident in the log,
   but it would still be worse than no number.
8. **Judge on the `Result:` line, not the exit code** — `Build.bat` returned exit 0 on four failed builds and
   exit 1 on a success on 2026-08-02.
9. **The model licence:** verify **the exact quant's** card, record repo id + exact filename + quant + the
   licence line verbatim into `Docs/ThirdPartyNotices.md` §2. **xLAM-2 / Hammer 2.1 / Arch-Function are banned.**
10. **Carry these two corrections into TASK-423's spec before it is dispatched:** `use_mmap` is gone
    (`load_mode = LLAMA_LOAD_MODE_MMAP`), and `abort_callback` is CPU-only, so the **token budget** is what
    bounds a GPU run — CONVENTIONS §8's "the only correct mechanism" needs that footnote.
11. **Bar #5's number is conditional on TASK-416's `MaxRosterKinds` seam** (WARN-5). Print the condition beside
    the number.

## Notes for gameplay-programmer (the fix loop)

All three blockers are in `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp`, which you own
exclusively. Estimated: BLOCKER-3 is one line, BLOCKER-2 is ~15 lines plus a log line, BLOCKER-1 is a second
Zone B+C fixture plus a divergence-position log line. **WARN-1, WARN-2 and WARN-4 are cheap and are in the same
file — please take them in the same pass.** WARN-6 is in `Tools/fetch_llm_model.py` (TASK-409's file, not
yours). Nothing in this report asks you to change the corpus, Zone A's bytes, the GBNF, the few-shots, or the
scoring rule — **the Clarify-rule judgement call (§4 of your handoff) is RULED CORRECT AS IMPLEMENTED**: keep
the lenient rule as PRIMARY, keep STRICT and the computed floor printed beside it. That is the right answer and
the disclosure is what makes it defensible.

---

## ⚠️ Board update — I have no Edit tool; exact replacement text follows

`.claude/pipeline/TASKBOARD.md` line 4423, under `#### TASK-412`, replace:

```
- status: backlog
```

with:

```
- status: **qa-failed** (2026-08-02 — `qa/TASK-412.md`). **TASK-409 PASS · TASK-410 FAIL (3 blockers) · TASK-426 PASS · TASK-411 not reviewed in depth (dispatch scope).** All three blockers are semantic, in `SiegeLlamaSpike.cpp`, and all three compile clean — **two of them make a go/no-go number FLATTER the result.** **B1: bar #3 + bar #2's warm TTFT** — turn 1 and turn 2 share one Zone B+C fixture, so the KV reuse point lands at the `order:` line instead of a zone boundary; DROP will read ~97-98 % against a shipped ~78-82 % (§8's own 165/765), and warm prefill is measured ~245 tokens short of what ships. **B2: the offload device is never pinned** — `devices`/`split_mode`/`main_gpu` are all left at llama defaults ("if NULL, all available devices are used", `llama.h:307`) on a machine whose own link proof enumerates **two** Vulkan devices, while `SampleMemory` reports "the first discrete GPU" by assumption ⇒ bar #4's 8 GB answer is not attributable. **B3: `EnsureModelLoaded` never compares the model path**, so `model=<B>` after `model=<A>` silently reports A's numbers with no `SPIKE_LOAD` line. 10 WARNs incl. a bench utterance byte-identical to a HOLDOUT sentence, and an HF-token host check that also matches look-alike hosts. ✅ **VERIFIED, not accepted:** the leniency floor **12/25 = 48 %**, the **6** thin dev rows and **DEV-08 asserting nothing** all re-derived by hand and CORRECT · corpus loaded by path, holdout has no default and is opened only by explicit act, splits scored separately · Zone A + the synonym table + the 7 place symbols diffed line-for-line against the landed builders (Zone B = **68** chars and Zone C = **887** chars confirmed exactly) · all 3 few-shots **parse under the landed GBNF**, all four §9c traps correct · few-shots literally disjoint from BOTH files · **no multi-turn loop** (proved from the `seq_rm` arithmetic) · comment scan **PASS (0)** re-run on raw file reads · **`use_mmap` IS GONE (`load_mode = LLAMA_LOAD_MODE_MMAP`) and `abort_callback` IS CPU-ONLY — both confirmed in the vendored headers; TASK-423's spec must be corrected before dispatch.** **RULED: the `AsyncThread` / `TPri_BelowNormal` deviation is ACCEPTED** (engine source shows it creates a real dedicated thread at the requested priority) — **bar #1 has an answer.** **RULED: the Clarify-row scoring rule is CORRECT AS IMPLEMENTED — keep lenient as PRIMARY with STRICT + the computed floor printed beside it.** **RULED: `Projects` + `DeveloperSettings` deps ACCEPTED** (flag (b) closed).
```

Also update `#### TASK-410` line 4266 `- status: ready-for-qa` → `- status: **qa-failed** (loop 1 of 3 — `qa/TASK-412.md`, 3 blockers, all in `SiegeLlamaSpike.cpp`)`
and `#### TASK-409` line 4168 `- status: **ready-for-qa** …` → `- status: **qa-passed** (2026-08-02 — `qa/TASK-412.md`, 0 blockers, 4 warns; still **NOT COMMITTED**, TASK-414 owns the commit)`.

---
---

# Re-gate — fix loop 1

**Reviewer:** qa-reviewer · **Date:** 2026-08-02 · **Verdict: ✅ PASS — 0 BLOCKERS.**
**Scope:** targeted re-gate of TASK-410 only. `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp`
(the only file changed; now **4,172 lines**, was 3,260) + `handoffs/TASK-410-programmer.md` §"Fix loop 1".
TASK-409 and TASK-426 remain **PASSED** and were not re-opened. Nothing compiled, no engine, no Git — correct
at this stage.

| Item | Verdict |
|---|---|
| **BLOCKER-1** — bar #3 / bar #2 measured a reuse the shipped path cannot reach | ✅ **CLOSED** |
| **BLOCKER-2** — offload device never pinned, bar #4's VRAM unattributed | ✅ **CLOSED** |
| **BLOCKER-3** — model swap silently ignored | ✅ **CLOSED** |
| WARN-1 (holdout collision) · WARN-2 (actuals) · WARN-4 (kinds/counts) · WARN-10 (EOG) · NIT-3 (`n/a`) | ✅ **all closed** |
| New findings | 0 BLOCKER · **2 WARN** · **4 NIT** |

---

## ⚠️ THE ONE CHECK THAT CARRIES THE FIX — where the first divergent byte actually lands

**I derived it myself from the fixture data and the serializer. It does NOT land at the very start of Zone B.
It lands 23 characters in — and that is the earliest divergence the shipped layout physically admits, so the
fixture is optimal and the residual error is ~0.6 percentage points in the flattering direction.**

`AppendZoneB` (`:529-536`) emits, unconditionally and in fixed order:

```cpp
Out += TEXT("[MATCH]\n");
Out.Appendf(TEXT("own_castle_hp: %d%%\n"), Fixture.OwnCastleHpBand);
Out.Appendf(TEXT("enemy_castle_hp: %d%%\n"), Fixture.EnemyCastleHpBand);
Out.Appendf(TEXT("mid: %s\n"), Fixture.MidOwner);
Out.Appendf(TEXT("gold: %d\n"), Fixture.GoldBand);
```

t0 = `80, 60, "ours", 120` (`:474-481`) · t1 = `70, 45, "neutral", 165` (`:491-498`).

| char offset in Zone B | bytes | t0 | t1 | same? |
|---|---|---|---|---|
| 0–7 | `[MATCH]\n` | literal | literal | **identical** |
| 8–22 | `own_castle_hp: ` | format-string prefix | format-string prefix | **identical** |
| **23** | first value byte | `8` | `7` | ⛔ **FIRST DIVERGENCE** |

**So the reuse point extends 23 characters past the Zone B boundary — the `[MATCH]` header line plus the
`own_castle_hp: ` label.** In token space that is **~8–10 tokens** (the common-prefix walk is over tokens, and
greedy tokenization truncates to the last boundary at or before char 23, so it is an upper bound).

**Is that a defect?** No — and I checked the stronger claim rather than the stated one. Those 23 bytes are a
structural constant of the layout: `[MATCH]\n` is a literal and `own_castle_hp: ` is the fixed prefix of the
first `Appendf`. **No two Zone B states the shipped serializer can produce are capable of differing before
char 23.** t1 therefore achieves the *true* earliest-possible divergence, which is exactly what a worst-case
bound must do. The code comment at `:484-489` states this precisely and correctly — *"the FIRST differing token
is inside the first Zone B line — there is no arrangement of a live board that diverges EARLIER than this"*.
Only the handoff's prose shorthand ("at the start of Zone B") is loose; the artifact is right.

**How much does it move the number?** With Zone A ≈ 1,200 tok and Zone B+C ≈ 265 tok (turn-1 total ≈ 1,465):

- ideal (divergence at Zone B char 0): turn-2 prefill 265 ⇒ **DROP = 81.9 %**
- as built (divergence at Zone B char 23): turn-2 prefill ≈ 256 ⇒ **DROP = 82.5 %**

**≈ 0.6 percentage points optimistic, against a 70 % bar with ~12 points of margin. Immaterial, and it does not
change the sign of any answer.** Critically it is also **auditable at run time**: every `SPIKE_PREFILL` /
`SPIKE_LATENCY` / `SPIKE_EVAL_ROW` line prints `reused=N … zoneB_start~=M`, so TASK-413 can compute `N − M`
itself and read the ~8–10-token overshoot straight off the log. That is the "evidence, not a number" property
the fix was asked for.

⚠️ **TASK-413 must actually do that subtraction once** and record it: if `reused − zoneB_start~` ever comes back
much larger than ~10 tokens, the two fixtures stopped differing where they are supposed to and the bound has
silently drifted back toward the best case. See NIT-R1.

### t0's bytes are unchanged — Zone B = 68, Zone C = 887, re-derived by hand against the new `Appendf` forms

The refactor from literals to `Appendf` is byte-identical. I counted the **output**, not the format strings:

**Zone B = 68** · `[MATCH]\n` 8 + `own_castle_hp: 80%\n` 19 + `enemy_castle_hp: 60%\n` 21 + `mid: ours\n` 10 +
`gold: 120\n` 10 = **68** ✓ (the `%%` in the format emits one `%`, which is what the 19/21 counts assume).

**Zone C = 887** ·
`[FORCES]\n` 9 + `places: …\n` 99 (8 + the seven symbols 78 + six `, ` separators 12 + 1) + `roster:\n` 8 +
**13 roster rows 595** + `other_kinds: none\n` 18 + `stances: free 20, following 12, holding 8, ambushing 4\n` 55 +
`hero: alive\n` 12 + `pending: none\n` 14 + `[ORDER]\n` 8 + `order: <61 chars>\n` 69 = **887** ✓

The roster arithmetic, since it is the bulk: the format `- %s: %d total, %d orderable, %d followable\n` has
**36 fixed output chars**; every t0 count is a single digit, so each row = 36 + len(kind) + 3. Kind lengths sum
to **88** over the 13 rows ⇒ 13 × 39 + 88 = **595**. Spot-checked literally: `- footman: 8 total, 8 orderable,
8 followable\n` = **46** chars = 36 + 7 + 3 ✓. The 61-char order line is the default utterance at `:4012`,
`send 10 footmen with a sorcerer to the nearest ancient ground` — I counted it (51 word chars + 10 spaces).

**⇒ B + C = 955 of 1,440, headroom 485 — unchanged from my first pass. The corpus's premises are intact and
bar #5's accuracy number still rests on the board the rows were authored against.** The eval is hard-pinned to
`SpikeFixtureT0` at `:3422` with the reason stated in the code, and `RunOneSplit` prints a per-split line saying
its `prefill=`/`reused=` columns are a constant-fixture best case and are **not** bar #3 (`:3527-3529`). Correct.
There is also a run-time tripwire in `CmdSpikePrompt` (`:4050-4055`) that fires only for t0 **and** the default
order line — correctly scoped, so an `order=` override cannot produce a false alarm.

### The new "reuse reached Zone C" guard fires in the right direction

`:3210-3215`, gated on `bExpectZoneBDivergence && Divergence.bResolved && Divergence.bLandedInZoneCTail`:

- `BEST_CASE` (t0→t0) passes `false` — landing in Zone C is expected there, no warning. ✓
- `SHIPPED_WORST_CASE` (t0→t1) passes `true` — a reuse point at or past `zoneC_start` warns that
  *"DROP=…% overstates the reuse a live board would get"*. ✓

**That is the exact gap my original finding named: the old `DropPercent < 60.0` guard fired only on too LITTLE
reuse and was silent on implausibly too MUCH. Both guards are now present and they point in opposite
directions.** `ClassifyDivergence` (`:2523-2551`) is sound: `< zoneB` → ZONE_A, `< zoneC` → ZONE_B, else ZONE_C,
with `bResolved=false` printing `landed_in=unknown` rather than a confident wrong number. Zone offsets are
computed **structurally** (`:2387-2399`, `:2440-2452`) and never by searching for `[FORCES]`, which also appears
inside Zone A's schema block — the trap is correctly avoided and the code says why.

### The rest of B1

- **Two bounds, both printed** (`:3266-3281`), with `BEST_CASE` explicitly labelled *"do NOT quote it"* and
  `SHIPPED_WORST_CASE` labelled *"THE NUMBER"*. Both bounds share the **same** cold turn 1 (t0, utterance[0]),
  so the two DROPs are directly comparable — a good property that was not required.
- **Warm iterations alternate** `(Iteration % 2) == 0 ? t0 : t1` (`:3301`), so every warm turn re-prefills the
  full Zone B + Zone C. Bar #2 is now the shipped shape; the best-case warm TTFT is preserved on the
  `BAR#3 BOUNDS` line so nothing was lost.
- `VerifyFixtureKindParity` (`:506-526`) + a `static_assert` on the row count (`:417`) enforce that t1 carries
  the same kind set the GBNF is generated from — the §9c seam cannot open through the new fixture. Checked at
  run time by both the bench (`:3244`) and `SpikePrompt` (`:4058`), not merely asserted in a comment.

---

## BLOCKER-2 — CLOSED, and the header claims verified rather than assumed

**`LLAMA_SPLIT_MODE_NONE` is the correct enumerator in b10235.** `llama.h:198-203` declares
`enum llama_split_mode { LLAMA_SPLIT_MODE_NONE = 0, …LAYER = 1, …ROW = 2, …TENSOR = 3 }`. ✓ Set at `:2983`.

**`main_gpu = 0` with a one-entry `devices` list — SAFE, though the header does not fully document the
indexing.** `llama.h:317-318` says only *"the GPU that is used for the entire model when split_mode is
LLAMA_SPLIT_MODE_NONE"*; it does **not** state that the index is into `params.devices`. Upstream llama.cpp does
index `model->devices` (which is populated from `params.devices` when non-NULL) and range-checks against its
size — but the vendored drop ships headers + binaries only, so **I could not confirm that from the artifact and
I am not going to claim I did.** What makes it safe regardless:

1. With a **one-entry** list, `0` is the only in-range index under the documented reading — anything else would
   be rejected.
2. `llama_model_default_params()` already returns `main_gpu = 0`, so the assignment at `:2984` is a **no-op
   relative to the default**. Under any interpretation the code cannot be worse than not setting it.

⇒ **the pin is correct and carries no risk.** The array is `ggml_backend_dev_t PinnedDeviceList[2] =
{ PinnedDevice, nullptr }` (`:2979`) — NUL-terminated as `llama.h:307` requires, declared in the function scope
that outlives `llama_model_load_from_file`. ✓ It is only installed when `PinnedDevice != nullptr`; otherwise a
loud `SPIKE_WARN` says every VRAM figure is unattributed (`:2986-2993`). ✓

**"n/a never 0" (my NIT-3) — CONFIRMED.** `FormatVram(Bytes, bHasDevice)` (`:2200-2203`) returns the literal
`n/a` when no device was queried, and `FSpikeMemorySnapshot::bHasDevice` (`:2051`) is set only inside the
`Device != nullptr` branch of `SampleMemory` (`:2334-2343`). Every VRAM figure on the `model_load` (`:3085-3093`)
and `bench_end` (`:3359-3364`) lines goes through it, and the low-water mark additionally guards on
`VramLowWater != MAX_uint64`. ✓

**Subject named everywhere.** `SampleMemory` now reports on `GRunner.OffloadDevice` (`:2319`), every VRAM line
carries `vram_dev=`, `LogDeviceInventory` prints **every** device before *and* after the load with the pinned one
marked `<== PINNED, AND THE SUBJECT OF EVERY vram_ FIGURE` (`:2212-2231`, called at `:2961` and `:3077`), and the
before/after samples are **both taken after the pin is installed** (`GRunner.OffloadDevice` is assigned at
`:2949`, `BeforeLoad` sampled at `:2963`) — so the model-footprint delta is two samples of the *same* card,
which is the whole point. If nothing was pinned or the device was merely assumed, the bench says so at the end
and tells the reader to re-run with an explicit index (`:3366-3371`). ✓

**`enum ggml_backend_dev_type` — the elaborated keyword is REAL and REQUIRED, not a typo.** Confirmed in the
vendored header: the enum is declared at `ggml-backend.h:134` and a **function of exactly the same name** at
`ggml-backend.h:182` (`GGML_API enum ggml_backend_dev_type ggml_backend_dev_type(ggml_backend_dev_t device);`).
In C++ the function name hides the type name in that scope, so the unelaborated spelling names the function and
does not compile; the elaborated-type-specifier `enum ggml_backend_dev_type` performs type-only lookup and is
correct, standard C++. **The header itself does the same at `:182`, `:242` and `:248`.** Both spike uses
(`:2136` parameter, `:2156` member) are correct. **No compile risk.**

---

## BLOCKER-3 — CLOSED

`:2909-2918` — the reload key is now:

```cpp
GRunner.IsLoaded()
&& GRunner.LoadedModelPath   == Options.ModelPathOverride
&& GRunner.LoadedOptions.GpuDeviceSpec == Options.GpuDeviceSpec
&& GRunner.LoadedOptions.GpuLayers … ContextTokens … UBatch … Threads
```

**Both** the path and the device spec are covered, as claimed. `LoadedModelPath` is written at `:3038` from the
same `Options.ModelPathOverride` it is later compared against, and `UnloadModel()` clears it (`:2885`), so a
failed load cannot leave a stale key. The path is fully resolved on the **game thread** in `StartJob`
(`:3728-3731`) before the worker ever sees it, so both sides of the comparison have the same basis. ✓
Comparing the device *spec string* rather than the resolved pointer means `gpu=1` and `gpu=Vulkan1` cost one
redundant reload — the safe direction, and the code says so. The procedural `SpikeUnload` alternative was
correctly declined; I did not recommend it either.

---

## The four warns — all closed, one of them with a check only I could run

**WARN-1 ✅ — I checked the new string against the sealed holdout. NO COLLISION.**
`SpikeBenchUtterances[4]` is now `"whole army disengage from the middle and regroup at the home keep"`
(`:3153`). **It matches no row in `assistant_eval_holdout.csv`, and it is not a near-paraphrase of one** — the
closest holdout row by *shape* (army-wide fallback to the home place) shares **zero** content words with it.
I re-checked all five bench utterances against **both** files: none is byte-identical to any corpus sentence in
either split. The old string, which was verbatim a holdout `Sentence`, is gone. ✓
**The programmer's refusal to open the holdout to verify this was correct and I endorse it** — that check was
mine to run, and it passes. *(No holdout content is quoted here.)*
NIT-R3 below notes the one residual surface overlap, which is unchanged from my first pass and not a finding.

**WARN-2 ✅ — the actuals are queried AND used everywhere, including the path that mattered.**
`llama_n_batch` / `llama_n_ctx` / `llama_n_ubatch` are all queried after context creation (`:3047-3049`); all
three return `uint32_t` (`llama.h:554-557`) so the `static_cast<int32>` is right. The consumers:

| use | line | reads |
|---|---|---|
| **prompt-budget pre-check** (the path that would have made every bar read "inference failed") | `:2618` | `GRunner.ContextSize` ✅ |
| context-full check inside the decode loop | `:2812` | `GRunner.ContextSize` ✅ |
| prefill slice size | `:2691` | `GRunner.BatchSize` ✅ |

`Options.ContextTokens` survives **only** as a printed "requested" value (`:2625`, `:3060`, `:3067`) and in the
clamp comparison — it is never used as a budget again. A clamp raises a loud `SPIKE_WARN` naming requested vs
actual for all three (`:3054-3062`), and `SPIKE_LOAD` prints `ctx=N(req M)` / `ubatch=N(req M)`. ✓

**WARN-4 ✅ — independent scoring, and the "provably unchanged" claim holds where it can be proved.**
`:1698` / `:1704` are now two independent `KindSetMatches` calls. **I verified the load-bearing implication from
the function body rather than the comment:** `KindSetMatches(counts=true)` returns false only on (a) an arity
mismatch, (b) an unmatched kind, or (c) a count mismatch; `counts=false` returns false only on (a) or (b).
⇒ **`K_true ⇒ K_false`**, so for a row asserting both, `bKindsOk && bCountsOk = K_false && K_true = K_true` —
**bit-identical to the single value the old code produced. Pass/fail cannot move on those rows.** ✓
For a row asserting kinds but **not** counts (DEV-02 is the one such dev row), the new code yields
`K_false && true = K_false`, which is the correct scoring — the kind set must match and the count is genuinely
not asserted. I **cannot** diff that branch against the old code, which no longer exists on disk and was never
committed; I record that honestly. **The new behaviour is correct on its own terms, which is what the gate
needs, and bar #5's row verdicts are safe.** Supporting checks: `bCountsAsserted` is dropped when kinds and
counts are index-misaligned (`:1484-1490`), counts are compared at `Parsed.Counts[FoundIndex]` so the pairing
stays order-insensitive (`:1614-1624`), and `"all"` maps to `0` in both the parser (`:962-976`) and the corpus.

**WARN-10 ✅ — the fixed-length control is genuinely fixed length, and only for the control.**
`bIgnoreEogForFixedLength` is **derived, never parsed**: the single assignment in the file is `:3232`
(`RunBenchJob`, on a **local copy** of the options, `= !InOptions.bUseGrammar`). `RunEvalJob` never sets it and
`ParseOptions` cannot, so **no bar #5 number moves.** ✅ Under the flag the loop records `EogTokenIndex`, keeps
decoding to the budget, **drops the pieces after EOG from the text** (`:2798`) so filler never reaches the
answer, and prints `FIXED_LENGTH_CONTROL(grammar=0: EOG IGNORED at out token N …)` on the line. The EOG token
is not counted when the loop breaks normally (`break` precedes `++Out.OutputTokens`). ✓

**NIT-3 ✅** closed as a side effect of B2 (see above).

---

## The comment audit — RE-RUN INDEPENDENTLY ON RAW READS. PASS (0).

I did **not** take the reported 78/78. I read **all 4,172 lines of `SiegeLlamaSpike.cpp` raw** (eight `Read`
calls, no Grep — per my own tooling note, Grep's rendering mangles `/`-adjacent sequences and would produce
false positives *and* could mask a real one) and checked every block comment for a premature terminator.

**Result: no block comment closes early anywhere in the file. COMMENTSCAN: PASS (0).**

The `/*ParamName*/` idiom appears in the new code at `:1698`, `:1704` (`/*bCompareCounts*/`) and `:3269`,
`:3276` (`/*bExpectZoneBDivergence*/`), plus the pre-existing `/*UserData*/`, `/*Args*/`, `/*World*/`,
`/*UnusedDelta*/` — all benign single-line forms, not the bug signature. The one place that *discusses* the trap
(`:3627-3631`) correctly describes the two prefixes in prose without writing the glob pair, and the new comment
at `:2132-2135` explaining the elaborated `enum` keyword does the same. The discipline held through a
900-line edit.

---

## New findings from this pass — 0 BLOCKER, 2 WARN, 4 NIT

- **[WARN-R1] `SiegeLlamaSpike.cpp:2630-2635` — a stale comment now asserts the exact expectation BLOCKER-1
  overturned.** `RunGeneration`'s comment still reads *"Zone A and Zone B are byte-identical between turns by
  construction, so the reuse point should land at the start of Zone C. If it does not, the LAYOUT is wrong and
  must be fixed before anything else is built."* **That is now false for the shipped bound and for every warm
  iteration** — t0/t1 alternation makes Zone B deliberately *not* identical, and a reuse point at the start of
  Zone C is precisely the failure the new guard exists to catch. This is the same comment I quoted as evidence
  in BLOCKER-1. No behaviour is affected and the bench's own comments and log lines say the right thing, so it
  is not a blocker — but **TASK-423 inherits this file's comments as its design brief**, and this one points the
  wrong way. **Fix: rewrite it to say the reuse point lands at the start of Zone B when the snapshot moved, at
  the start of Zone C only when it did not, and that `landed_in=` reports which.**
- **[WARN-R2] `:3192-3204` — `RunPrefillBound` does not surface `bDecodeFailed` / `bAborted` on the
  `SPIKE_PREFILL` line, so a failed turn 1 reports as a layout defect.** If turn 1 fails, `Turn1.PrefillTokens`
  is 0, `DropPercent` computes to `0.0`, and the `< 60 %` guard fires with *"THE PROMPT LAYOUT IS WRONG AND MUST
  BE FIXED BEFORE ANYTHING ELSE IS BUILT"* — a misattribution at the loudest possible volume. `RunGeneration`
  does log the real cause immediately above, so the log is recoverable, but the two lines contradict each other.
  The per-iteration `SPIKE_LATENCY` line prints `ABORTED` / `DECODE_FAILED`; the bound line should too.
  **TASK-413: if you see `DROP=0.0%` plus a layout warning, read the lines above it before believing it.**
- **[NIT-R1]** The Zone-C guard catches reuse that reaches Zone C, but nothing catches reuse landing *deep
  inside* Zone B (e.g. if a future fixture edit left the four Zone B keys equal and moved only the roster).
  Unreachable with t1 as authored, and `reused` vs `zoneB_start~` are both printed so it is checkable by hand —
  but a `reused − zoneB_start~ > ~16 tokens` warning would close the class rather than the instance.
- **[NIT-R2]** `SampleMemory`'s pre-load fallback (`:2320-2332`) picks the first discrete GPU when nothing is
  pinned and sets `bHasDevice = true`, while `GRunner.OffloadDeviceLabel` is still `<none>` — so a line could
  read `vram_dev=<none> vram_free_before=NNNN MiB`. Reachable only by explicitly naming a CPU/ACCEL device with
  `gpu=`, which `ResolveOffloadDevice` refuses with a loud reason line first (`:2264-2270`). Not on TASK-413's
  path.
- **[NIT-R3]** `SpikeBenchUtterances[0]` shares a long literal run with one holdout row (`send 12 footmen and …
  to the far ancient ground`) without being byte-identical, so §11's literal disjointness test passes. Unchanged
  from my first pass, where I cleared it. Recording it only for completeness: the bench is unscored and the
  sentence is not a sealed one, so the pre-exposure concern WARN-1 raised does not apply.
- **[NIT-R4]** With `grammar=0`, if the model never emits EOG inside the token budget, `bEogIgnored` stays false
  and the `FIXED_LENGTH_CONTROL(...)` note does not print — so the reader of a genuinely-60-token control sees no
  line saying it was one. Cosmetic.

**Carried forward unchanged and still open (not re-checked this pass, none of them TASK-410's file):**
WARN-3 (baseline captured before the first load — remains a TASK-413 procedure item, correctly declined),
WARN-5 / WARN-9 / NIT-4 (rulings on how to read bar #5), WARN-6 (`Tools/fetch_llm_model.py`, TASK-409's),
WARN-7 (`.gitignore`, TASK-414's), WARN-8 (`SiegeLlamaModule.cpp`), NIT-1 / NIT-2 (recorded for TASK-423).

---

## Notes for build-master (TASK-413)

**The eleven notes in the original report all stand, with these amendments:**

1. **Note 1 is superseded: BLOCKER-1..3 have landed and the code lane is clear to run.** BLOCKER-1 of the
   *handoff* (the `SiegeLlamaModule.h:38` comment trap) is a different item and was already fixed — I re-confirmed
   it on a raw read.
2. **Bar #3: quote `SHIPPED_WORST_CASE`. Never `BEST_CASE`.** Both are printed on the `SPIKE_PREFILL` lines and
   summarised on the `BAR#3 BOUNDS` line. The gap between them is itself a reportable number.
3. **Do the subtraction once:** on the `SHIPPED_WORST_CASE` line, compute `reused − zoneB_start~`. Expect
   **~8–10 tokens** (the `[MATCH]` header + the `own_castle_hp: ` label, which no live board can ever change).
   **If it is materially larger, the fixtures have drifted and the bound is optimistic — say so instead of
   quoting the number.**
4. **Bar #2's `mean_ttft_ms` / `WORST_wall_ms` will be WORSE than the previous build's, and that is the fix
   working** — the warm iterations now alternate t0/t1 and pay the full Zone B+C re-prefill. `mean_prefill_tok`
   is printed beside them; quote it so the basis is visible.
5. **Runtime cost: the bounds phase now runs 4 generations, not 2** (two cold turn-1s + two turn-2s), on top of
   `iters` warm iterations — **9 generations per bench at the default `iters=5`.** On the CPU tier budget the
   extra minutes accordingly; it is not a correctness concern.
6. **Pass `gpu=<index>` explicitly on every tier.** Without it the harness picks the first discrete GPU, labels
   every VRAM figure `ASSUMED`, and prints a warning at the end telling you to re-run. Take the index off the
   `SPIKE_MEM stage=devices_before_load` lines — on this machine that is the **RTX 5070**, not `Vulkan0`
   (the Arc iGPU). Bar #4's 8 GB question is about the discrete card.
7. **`Siege.Llama.SpikePrompt fixture=t1`** is new and worth one run: it dumps t1's bytes and the zone
   boundaries in token space, which is how you check the `landed_in=` classifier itself rather than trusting it.
   `fixture=t0` (the default) must still print **Zone B = 68, Zone C = 887** — there is a tripwire, but read the
   numbers anyway.
8. **Bar #5 is unaffected by this whole fix loop.** The eval is pinned to t0, EOG is never ignored on an eval
   run, and kinds/counts scoring is bit-identical for every row that asserts both. The holdout discipline
   (notes 2, 3 and 9 in the original report) is unchanged and still governs.

---

## Would I stake the go/no-go on these numbers?

**Yes — with two sentences of framing that must travel with them.**

The three defects that made this harness flatter its own result are closed, and the two that mattered most are
closed in the way that matters: **bar #3 and bar #4 now print their own subject and their own error bar** — where
the reuse landed, against which zone boundary, on which named card — so Jonathan is not being asked to trust a
number, he is being handed one he can check. The residual ~0.6-point optimism in bar #3 is structural, is the
best any fixture can do, and is visible in the log. Bar #5 was never in question and did not move.

The two framings that must go to Jonathan *beside* the numbers, because they are conditions on the answer and
not defects in it: **(1) bar #5's holdout score must be read against the holdout's own printed leniency floor,
not dev's 48 %** (WARN-9), **and it is valid only while TASK-416's `MaxRosterKinds` seam stays open as measured**
(WARN-5); **(2) bar #1's hitch numbers are only attributable if the tier is run twice and the second run is
reported** (WARN-3), because the first run's baseline is captured before the model is resident.

**Run TASK-413. If the numbers come back clean under those two conditions, I would put the go/no-go on them.**

---

## ⚠️ Board update — I have no Edit tool; exact replacement text follows

`.claude/pipeline/TASKBOARD.md` **line 4266**, under `#### TASK-410`, replace the whole `- status:` line with:

```
- status: **qa-passed** (2026-08-02 — re-gate in `qa/TASK-412.md` §"Re-gate — fix loop 1"; fix loop 1 of 3, **0 blockers**, 2 new WARN, 4 new NIT). **ALL THREE BLOCKERS CLOSED, RE-DERIVED NOT ACCEPTED.** **B1:** I derived the first divergent byte between t0 and t1 myself from the fixture data + `AppendZoneB` — it lands at **char 23 of Zone B** (after the `[MATCH]\n` header and the `own_castle_hp: ` label), i.e. **~8–10 tokens**, not at byte 0. **That is the earliest divergence the shipped serializer can produce** (both are structural constants), so t1 is a genuine worst case; the residual optimism is **≈0.6 percentage points** on a bar with ~12 points of margin, and it is **auditable at run time** because every line prints `reused=N … zoneB_start~=M`. Two bounds print, `BEST_CASE` is labelled do-not-quote, the warm iterations alternate t0/t1 (bar #2 is now the shipped shape), and **the new Zone-C guard fires on too MUCH reuse** — the exact direction the old `DROP<60%` guard was blind to. **t0's bytes are unchanged: Zone B = 68 and Zone C = 887 re-derived BY HAND against the new `Appendf` forms** (roster = 13×39 + 88 = 595; default order line = 61 chars), so the sealed corpus's premises and bar #5 are intact; eval is hard-pinned to t0. **B2:** `LLAMA_SPLIT_MODE_NONE` confirmed at `llama.h:199`; the one-entry NUL-terminated `devices` array is correct per `llama.h:307`; **`main_gpu = 0` is safe — the header does NOT document the indexing basis (`llama.h:317`) and the vendored drop is headers-only so I could not confirm it from the artifact, but `0` is the only in-range index for a one-entry list AND is already the default, so the assignment cannot be worse than omitting it.** Every device logged before + after load with the pinned one marked; `vram_dev=` on every VRAM line; **`n/a` never `0`** (my NIT-3) confirmed via `FormatVram`/`bHasDevice`; before/after samples are the SAME card. **`enum ggml_backend_dev_type`'s elaborated keyword is REAL AND REQUIRED — the enum at `ggml-backend.h:134` is hidden in C++ by a function of the same name at `:182`; the vendored header spells it the same way. Not a typo, no compile risk.** **B3:** reload key genuinely covers **both** model path and `gpu=` spec; `LoadedModelPath` written from the same value it is compared against and cleared by `UnloadModel`; the procedural alternative correctly declined. **WARN-1 CLOSED — I checked the new utterance against the SEALED HOLDOUT (only QA may): NO COLLISION, and not a near-paraphrase; the programmer's refusal to open it was correct and is endorsed.** **WARN-2:** actuals used in the prompt-budget pre-check, the context-full check and the prefill slicing — the requested values survive only as printed `(req N)`. **WARN-4:** pass/fail **proved** unchanged for every row asserting both fields (`KindSetMatches(counts=true) ⇒ KindSetMatches(counts=false)`, derived from the body); the kinds-only branch cannot be diffed against code that was never committed, but is correct as built. **WARN-10:** `bIgnoreEogForFixedLength` has exactly ONE assignment, in the bench's local options copy — no bar #5 number can move. **COMMENTSCAN: PASS (0), re-run independently on RAW READS of all 4,172 lines (never Grep).** ⚠️ **NEW: [WARN-R1] the stale comment at `:2630-2635` still asserts "the reuse point should land at the start of Zone C … if it does not, the LAYOUT is wrong" — the exact expectation B1 overturned, and TASK-423 inherits these comments as its brief. [WARN-R2] `RunPrefillBound` omits `bDecodeFailed`/`bAborted`, so a failed turn 1 reports `DROP=0.0%` + "THE PROMPT LAYOUT IS WRONG".** ⚠️ **Bar #3 now costs 4 generations instead of 2 → 9 per bench at `iters=5`.** **NOT COMPILED, NOT COMMITTED — build-master gates it.**
```

Also update `#### TASK-412`'s status line: `- status: **qa-failed** …` → keep the existing text and append:
`· **RE-GATE 2026-08-02: TASK-410 now qa-passed (fix loop 1, 0 blockers) — see §"Re-gate — fix loop 1". TASK-409 and TASK-426 remain PASSED. The gate is GREEN and TASK-413 may run.**`
