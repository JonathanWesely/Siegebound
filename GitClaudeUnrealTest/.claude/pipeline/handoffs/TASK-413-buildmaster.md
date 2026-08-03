# TASK-413 — THE SPIKE RUN (build-master)

**Date:** 2026-08-02 · **Author:** build-master · **Gate in:** GREEN (TASK-409 qa-passed · TASK-410 qa-passed after
fix loop 1 · TASK-426 passed, `qa/TASK-412.md` + §"Re-gate — fix loop 1").

> # ⛔ HEADLINE: THE SPIKE DID NOT RUN. FIVE OF THE SIX BARS ARE **NOT MEASURED**, AND I AM NOT GIVING A VERDICT.
>
> **The pinned model does not exist under the pinned name, and the model that *does* exist is not the
> architecture CONVENTIONS §8 was designed around.** No GGUF was downloaded. Nothing was committed. Nothing
> was pushed. `L_Arena` was never saved. The working tree is byte-identical to how I found it.
>
> **There is no GO, no GO-WITH-RESCOPE and no NO-GO in this report, because a verdict needs numbers and I have
> none to give.** Manufacturing one from a substituted model is precisely the failure this task was written to
> prevent.

---

## 1. What DID land

| # | Item | Result |
|---|---|---|
| 0 | **Compile — game module + `SiegeLlama` plugin** | ✅ **GREEN** |
| — | `Siege.Llama.SpikeGrammar` runtime GBNF dump | ✅ **captured + diffed** |
| — | `Siege.Llama.SpikePrompt` **char** counts | ✅ **measured, and they match QA's hand count exactly** |
| — | Model existence + licence investigation | ✅ **done — and it is what stopped the run** |
| 6 | Input probe (TASK-411) | ⛔ **NOT ANSWERED — two environments, two distinct obstacles (§6)** |
| 1,2,3,4,5 | The five model-dependent bars | ⛔ **NOT MEASURED — no model** |

---

## 2. Step 0 — the compile. GREEN.

`SiegeLlamaSpike.cpp` had never been compiled since the fix loop. It compiles and links clean.

```
[1/4] Compile [x64] SiegeLlamaSpike.cpp
[2/4] Link [x64] UnrealEditor-SiegeLlama.lib
[3/4] Link [x64] UnrealEditor-SiegeLlama.dll
[4/4] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Result: Succeeded
```

- **Judged on the `Result:` line, never the exit code**, per the standing law. (For the record the exit code
  was also 0 this time — but it was not what I judged on.)
- **Zero diagnostics.** The whole 30-line log contains no `warning`, no `error`, no `failed`. TASK-409's
  warnings-as-errors setting is in force, so a warning would have been a failure.
- `SiegeAssistantInputProbe.cpp` (TASK-411, game module) was already current from an earlier build — **it too
  is compile-clean.**
- Both `[Adaptive Build]` lines confirm the two lanes are in **separate UBT modules**, as ruling B intended.
- The editor was closed **gracefully** (WM_CLOSE, never a kill) and it exited with **no save prompt at all** —
  I verified `/Game/Maps/L_Arena` was not dirty *before* closing, and the clean exit confirms it.
  **`L_Arena.umap` was never saved. The one-time exception was not spent.**

---

## 3. ⛔ Step 1 — THE MODEL. THIS IS WHY THERE ARE NO NUMBERS.

Everything below was read from Hugging Face's own model API, live. It is not recalled and not inferred.

### 3a. The pinned filename does not exist — anywhere

`USiegeLlamaSettings::DefaultModelFileName` and the board both pin
**`Qwen3.5-4B-Instruct-Q4_K_M.gguf`**. That file does not exist in any repo, because
**Qwen publishes no `-Instruct` variant of Qwen3.5-4B at all.**

| repo id probed | result |
|---|---|
| `Qwen/Qwen3.5-4B-Instruct-GGUF` | **404** |
| `Qwen/Qwen3.5-4B-Instruct` | **404** |
| `Qwen/Qwen3.5-4B-GGUF` | **404** (Qwen ships **no official GGUF** for this model) |
| `Qwen/Qwen3.5-4B` | **EXISTS** — the post-trained model, `license: apache-2.0` |
| `Qwen/Qwen3.5-4B-Base` | **EXISTS** — the base model, separately named |

⚠️ **My first probe pass concluded the whole family was missing. That was wrong and I corrected it before
reporting** — a keyword search returns `Qwen/Qwen3.5-9B`, `-4B`, `-2B`, `-27B`, `-0.8B`, `-35B-A3B`,
`-122B-A10B`. The family is real; **only the `-Instruct` infix is fictitious.** Recording the correction
because the first answer would have been a confidently wrong "the model does not exist".

**Reading it:** since `-Base` is separately named, `Qwen/Qwen3.5-4B` **is** the instruct/post-trained variant.
Note CONVENTIONS §7's cleared name is literally **`Qwen3.5-4B (Apache 2.0)` — with no `-Instruct`.** So the
suffix looks like drafting drift in the settings default and the board prose, not a real artifact.
**On its own this would be a naming reconciliation, not a substitution.** §3b is why I still stopped.

### 3b. ⚠️ THE ONE THAT ACTUALLY BLOCKS: `Qwen3.5-4B` IS NOT THE MODEL §8 WAS DESIGNED AROUND

From `Qwen/Qwen3.5-4B`'s own `config.json` and model card:

- `"architectures": ["Qwen3_5ForConditionalGeneration"]`, `pipeline_tag: image-text-to-text`, a `vision_config`
  block, `image_token_id` / `video_token_id` ⇒ **it is a multimodal vision-language model.**
- **`"layer_types"`: 32 layers, of which only 8 are `full_attention` — the other 24 are `linear_attention`**
  (`full_attention_interval: 4`). Plus `linear_conv_kernel_dim`, `linear_num_key_heads`, `mamba_ssm_dtype`.
  Qwen's own card calls it *"Gated Delta Networks combined with sparse Mixture-of-Experts"*.

**Why this stops the run rather than being a footnote:**

**Bar #3 is `llama_memory_seq_rm` KV-prefix reuse, and that mechanism is architecture-dependent.**
Linear-attention / SSM layers carry a **rolling recurrent state**, not a per-token KV cache, so rewinding to an
arbitrary prefix position is not the same operation it is on a dense transformer. The harness is honest about
this by construction — QA verified that when `seq_rm` returns false the runner clears the cache and declares
that turn's reuse **"0 by construction, not by measurement"** (`SiegeLlamaSpike.cpp:2102-2110`). So it will not
lie to us.

⚠️ **But `RunPrefillBound` would then print `DROP=0.0%` together with "THE PROMPT LAYOUT IS WRONG AND MUST BE
FIXED BEFORE ANYTHING ELSE IS BUILT"** (WARN-R2) — a maximum-volume misattribution blaming §8's prompt layout
for a property of the model class. §8's layout law would get "fixed" to chase a number it does not control.

**This is the "a different model invalidates the plan's premises" scenario, not a spelling fix.** §8 is written
throughout for a dense-transformer KV cache. Whether the assistant should be built on a hybrid linear-attention
multimodal 4B is a design decision with real consequences, and **it is not build-master's to take.**

I am **also** flagging, without deciding: the GGUF repos ship the language model and the vision tower as
**separate files** (`mmproj-*.gguf`), so bar #4's VRAM would not be inflated by the vision encoder — the
multimodality is a *fitness* question, not a footprint one.

### 3c. Licence record — VERIFIED, but NOT written to `Docs/ThirdPartyNotices.md`

**I deliberately left §2 as its marked placeholder.** Recording a licence for a model that was never selected,
never downloaded and never used would put a false record into the file that exists to prevent exactly that.
**§2 gets filled by whoever runs the spike with the ruled model.** Evidence gathered so far, for that task:

| repo | licence (card front-matter, verbatim) | `license_link` | gated | `Q4_K_M` filename |
|---|---|---|---|---|
| `Qwen/Qwen3.5-4B` (upstream) | `license: apache-2.0` | `https://huggingface.co/Qwen/Qwen3.5-4B/blob/main/LICENSE` | False | *(no official GGUF)* |
| `unsloth/Qwen3.5-4B-GGUF` | `license: apache-2.0` | → Qwen's own LICENSE | False | `Qwen3.5-4B-Q4_K_M.gguf` |
| `lmstudio-community/Qwen3.5-4B-GGUF` | `license: apache-2.0` | — | False | `Qwen3.5-4B-Q4_K_M.gguf` |
| `bartowski/Qwen_Qwen3.5-4B-GGUF` | `license: apache-2.0` | → Qwen's own LICENSE | False | `Qwen_Qwen3.5-4B-Q4_K_M.gguf` |
| `Mungert/Qwen3.5-4B-GGUF` | `license: apache-2.0` | → Qwen's own LICENSE | False | `Qwen3.5-4B-q4_k_m.gguf` |

✅ **§7's banned list was honoured absolutely: `xLAM-2`, `Hammer 2.1` and `Arch-Function` were never probed,
never considered and never downloaded.**
✅ **`HF_TOKEN` was read from the environment only.** It never appeared on a command line, in a log, in a
report or in a Slack post. Every probe ran **bare over TLS** — verification was never disabled.
⚠️ **The re-upload cards are read but NOT content-locked.** §7 requires the *exact quant's* card, and a
re-upload can be re-licensed at any time. **Whoever downloads re-reads the card at download time.**

### 3d. The rest of the cleared list actually exists, if the ruling goes that way

All verified live, all with a real `Q4_K_M`: `unsloth/Phi-4-mini-instruct-GGUF` (**MIT**) ·
`ggml-org/SmolLM3-3B-GGUF` and `unsloth/SmolLM3-3B-GGUF` (**apache-2.0**) ·
`ibm-granite/granite-4.0-h-tiny-GGUF` and `granite-4.0-micro-GGUF` (**apache-2.0**) ·
`Qwen/Qwen3-4B-GGUF` (**apache-2.0**, `Qwen3-4B-Q4_K_M.gguf`, official Qwen GGUF, dense transformer).
⚠️ Gemma is the awkward one: `google/*` GGUF repos are **`license: gemma` and `gated: manual`** — not
Apache-2.0 as §7 records, and gated behind a manual acceptance Jonathan would have to click.

---

## 4. Artifact — `Siege.Llama.SpikeGrammar`, and the diff QA asked for

Runtime dump, `SPIKE_RUN ---- GBNF (1418 chars) ----`:

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
kind ::= "\"footman\"" | "\"archer\"" | "\"knight\"" | "\"miner\"" | "\"militiamob\"" | "\"pikeman\"" | "\"sapper\"" | "\"cavalry\"" | "\"longbowman\"" | "\"cleric\"" | "\"ogre\"" | "\"wizard\"" | "\"sorcerer\""
where ::= "\"own_castle\"" | "\"enemy_castle\"" | "\"mid\"" | "\"ancient_ground_near\"" | "\"ancient_ground_far\"" | "\"nearest_mine\"" | "\"hero\"" | "\"none\""
count ::= "1" | ... | "30" | "\"all\""
at_least ::= "1" | ... | "30"
item ::= "{\"kind\":" kind ",\"n\":" count "}"
selection ::= "[" item "]" | "[" item "," item "]" | "[" item "," item "," item "]"
who ::= selection | "\"all\"" | "\"none\""
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at_least "}"
```

**Diffed against `USiegeAssistantGrammar::Build` — the landed generator emits, in order:**
`root · command · question · ask · intent · kind · where · count · at_least · item · selection · who · when`
(`SiegeAssistantGrammar.cpp:186, 204, 214, 229, 247, 262, 279, 296, 312, 327, 359, 375, 398`).
**Rule-for-rule identical, in the same emission order.** QA's by-construction derivation is now confirmed by
the running artifact. ✅ **§9c seam holds at runtime.**

All four sketch traps confirmed **on the live dump**: key is **`who`** not `select` · per-pair key is **`n`**
not `count` · army-wide is **`"who":"none"`** not `[]` · the **`{"ask":ASK}`** branch exists with the five ask
codes in order. `count` is **1..30** (`GrammarCountMax`), *not* the live roster max. The 3-kind cap is a
**bounded alternation** in `selection`. Places: **`own_castle`, never `my_castle`**; `nearest_mine` and `hero`
both present.

---

## 5. Artifact — `Siege.Llama.SpikePrompt`, and what it says about the §8 token figures

```
SPIKE_TOKENS ---- ZONE A (4314 chars) ----
SPIKE_TOKENS ---- ZONE B (fixture=t0, 68 chars) ----
SPIKE_TOKENS ---- ZONE C (fixture=t0, 887 chars) ----
SPIKE_TOKENS fixture=t0 zoneA_chars=4314 zoneB_chars=68 zoneC_chars=887 zoneBC_chars=955
             MaxSnapshotChars=1440 headroom=485
SPIKE_TOKENS: TOKEN counts need a resident model and no job in flight -- run 'Siege.Llama.SpikeLoad' first.
             Character counts above are exact regardless.
```

✅ **Zone B = 68 · Zone C = 887 · B+C = 955 of 1440 · headroom 485 — measured at runtime, matching QA's hand
derivation character-for-character.** The `Appendf` refactor is byte-identical for t0; the sealed corpus's
premises are intact. Zone A = **4,314 chars**, confirming TASK-410's finding that §8's "~600 tok as-built" was
measured with **no vocabulary attached**.

### ⛔ THE §8 / §10 TOKEN CORRECTION CANNOT BE MADE, AND MUST NOT BE FAKED

The single job §8 🔢 TOKEN PROVENANCE assigned to this task — replace every `DERIVED — PENDING TASK-413`
figure with `Siege.Llama.SpikePrompt`'s printed counts, in one pass — **cannot be done**, for a reason that is
stronger than "the model didn't download":

**A token count is a property of a specific tokenizer.** Qwen3.5's vocab is **248,320** tokens; Phi-4-mini's,
SmolLM3's and Granite's are all different. **There is no model-independent chars/token ratio to measure.** So
the correction is not merely blocked on *a* model — it is blocked on **the model that ships**, and re-running
it after a model change would be a second correction, which is exactly what "never guessed twice" forbids.

⇒ **Everything in §8 and §10 stays labelled `DERIVED — PENDING TASK-413`. `MaxSnapshotChars` stays at 1440.**
⇒ **The worst-case (smallest) chars/token figure §8's RESOLUTION bullet demands is NOT MEASURED.** I will not
estimate it, and I especially will not quote 3.6 or 5.8 as though this run had confirmed either.
⇒ **§8's TRIGGER — "if the printed worst-case ratio comes back BELOW 3.6, lower `MaxSnapshotChars` once" — did
not fire, because no ratio was printed.** It is still armed.
⇒ **Bar #3's reference figure `165/765 = 78 %` remains UNVERIFIED**, exactly as §8 records. No PASS or FAIL may
be declared against it.

---

## 6. Bar #6 — the input probe. **NOT ANSWERED.** Two environments, two distinct obstacles.

The probe is fully automatic (it synthesizes its own keystrokes through `FSlateApplication::ProcessKeyDownEvent`
— no human at the keyboard), so this should have been the one bar a missing model could not block. It was not.

### Attempt 1 — PIE on `L_Arena`. ⛔ **THE PROBE TERMINATES ITS OWN PIE SESSION.**

```
[04.09.32:518][686] InputProbe: starting 3 passes (CONTROL, MODE A GameAndUI+focus, MODE B UIOnly+focus).
[04.09.32:518][686] InputProbe: pass 1/3 — CONTROL (GameAndUI, NO focus)
[04.09.45:195][724] InputProbe: finished — the probe widget was destroyed mid-run.
[04.09.45:198][724] LogWorld: BeginTearingDown for /Game/Maps/UEDPIE_0_L_Arena
[04.09.45:200][724] LogPlayLevel: Display: Shutting down PIE online subsystems
```

**Zero rows produced** — `Siege.Assistant.InputProbeReport` then answered *"No probe has run in this session."*

**Root cause, derived from the artifact and not from the message:** every pass injects **`EKeys::Escape`** after
typing (`SiegeAssistantInputProbe.cpp:617-625` — Escape deliberately precedes Enter). **Pass 1 is CONTROL,
which by design leaves keyboard focus on the viewport.** An Escape that reaches a PIE viewport ends the play
session. The probe's abort line and `BeginTearingDown` land in **the same frame (724)**.

Two supporting facts I checked rather than assumed:
- The 12.7 s pass-1 duration looked too long for 17 keys at 0.2 s. It is not: frames **686 → 724 in 12.7 s is
  ~3 FPS**, because PIE was still doing `FlushAsyncLoading` and *"Waiting for skinned assets (SK_Miner)"*. The
  per-key timers quantize to frames, so ~0.2 s becomes ~0.66 s. **The arithmetic fits Escape-at-end-of-pass-1
  precisely.**
- Passes 2 and 3 would have survived — the later `-game` run shows `Escape -> PlayerInput: NO — swallowed` when
  the box has focus. **Only the CONTROL pass kills PIE, and it runs first, so no pass ever completes.**

⇒ **As built, `Siege.Assistant.InputProbe` can never produce a row in PIE** — which is the only environment its
own error message tells you to use (`"no game world — run this from PIE on L_Arena"`), and the environment the
board mandates. ⚠️ **QA explicitly did not review TASK-411 in depth** (`qa/TASK-412.md` §Scope), so this is new.

### Attempt 2 — standalone `-game`, where Escape does not end the session. ⛔ **CONTROL-FAILED.**

All three passes completed. The probe then **refused to report an answer**, correctly:

```
---- CONTROL (GameAndUI, NO focus) ----
  VERDICT              : CONTROL-FAILED
  why                  : the hero did NOT move with no widget focused — the injection never reached the
                         input stack, so nothing below is evidence of anything
  PATH length (cm)     : 0.00
  W/A/S/D on PlayerInput: no — Slate absorbed them
---- MODE A  (GameAndUI + SetKeyboardFocus) ----
  VERDICT              : INCONCLUSIVE     why: the CONTROL pass failed, so a zero delta here proves nothing
  text landed in box   : YES  ('wasd send footmen')
  focus held (typing)  : 644 / 644 frames
---- MODE B  (UIOnly + SetKeyboardFocus) ----
  VERDICT              : INCONCLUSIVE     why: the CONTROL pass failed, so a zero delta here proves nothing
```

**Cause: `-ExecCmds` fires on frame 0.** The `starting 3 passes` line carries frame counter **`[  0]`** — the
probe ran before Enhanced Input was live. UE 5.8 has **no delay facility for `-ExecCmds`**
(`UnrealEngine.cpp:2552` queues them once, unconditionally; I checked the engine source).

I tried to issue the command late into the warm `-game` window's own console and **Windows refused the
foreground grab**. My script had a guard that aborted rather than type blind, and it fired:
`COULD NOT FOCUS GAME WINDOW — aborting, not typing blind`. **I did not retry with a foreground-lock bypass** —
typing an unverified keystroke sequence into whatever window actually had focus is not a measurement technique.

### ⇒ Bar #6 is NOT ANSWERED, and that is the honest result

**MODE A and MODE B both showed `PATH length = 0.00` — and that number means nothing**, because the control
that exists to prove the keystrokes reached the input stack at all did not pass. Quoting "the hero didn't move,
therefore `FInputModeGameAndUI` is safe" off these rows would be **exactly the false green the probe's
three-pass design was built to prevent. The probe worked. It refused to let me report an answer it had not
earned.**

⚠️ **One thing the run does establish, independent of the control:** in **MODE B (UIOnly)**,
`RMB -> PlayerInput: NO — swallowed` and `Enter -> PlayerInput: NO — absorbed by the box`, whereas in **MODE A
(GameAndUI)** both still fire. That is the §8 note about UIOnly killing the shipped Escape/RMB cancel routes,
**observed rather than predicted** — and it holds regardless of the control, because it is about event routing,
not about hero movement.

---

## 7. The two conditions QA requires travelling beside the numbers

There are no numbers yet, so these travel forward **unspent**. Both are still binding on the re-run:

1. **Bar #5 must be read against the HOLDOUT's own printed leniency floor — never dev's 48 %** (WARN-9), and
   **it is valid only while TASK-416's `MaxRosterKinds = 8` seam stays open as measured** (WARN-5): the harness
   scores a 13-kind roster that the shipped snapshot truncates to 8, so shipped accuracy on a >8-kind board
   will be **worse than measured**. Also: **DEV-08 asserts nothing** and six dev rows assert ≤ 1 field.
   ⚠️ **`assistant_eval_holdout.csv` WAS NOT OPENED BY THIS TASK. The seal is intact and the one-shot is
   unspent** — it is still available to whoever actually runs bar #5.
2. **Bar #1's hitch numbers are attributable only if each tier is run twice and the SECOND run reported**
   (WARN-3), because the first run's baseline control window is captured before the model is resident.

**A third, added by this run:** ⚠️ **PIE on `L_Arena` runs at ~3 FPS for the first ~25 s** while nav builds and
`SK_Miner` streams in synchronously. **The bench must not be started until that settles**, or the baseline
control window is captured against a ~330 ms frame time and every `ATTRIBUTABLE` delta is measured from the
wrong floor. This is a *procedure* item for the re-run, on top of the run-twice rule.

---

## 8. The verdict

### **NO VERDICT. THE SPIKE DID NOT RUN.**

Ruling 10 offers **GO / GO-WITH-RESCOPE / NO-GO**, and **none of the three is available to me**, because all
three are readings of measurements and I have five bars unmeasured and the sixth unanswered. **A NO-GO would be
just as false as a GO** — nothing here says the feature cannot work; it says nobody has tested it yet.

**What is genuinely settled:** the code lane is **real**. It compiles clean under warnings-as-errors, the
plugin loads its vendored llama.cpp (`4 modules`), the GBNF the model will be constrained by is **runtime-
verified identical to the shipped generator**, and the prompt's char budget is **measured and comfortable**
(955 of 1440). **The harness is ready. It has nothing to measure.**

**The decision Jonathan actually faces is not go/no-go. It is: which model?** And it is a real design question,
not a typo fix, because the model the plan names has a **hybrid linear-attention** architecture that bar #3's
whole mechanism may not apply to.

**My recommendation, offered as measurement input and not as a decision:**

1. **Rule on the model first (manager reads, Jonathan decides).** The two coherent options:
   - **`Qwen3.5-4B`** (`unsloth/Qwen3.5-4B-GGUF` → `Qwen3.5-4B-Q4_K_M.gguf`, apache-2.0) — what the plan
     *names*. Accept that **bar #3 is now genuinely open**, and read a low DROP as an architecture result, not
     a §8 layout defect.
   - **`Qwen3-4B`** (`Qwen/Qwen3-4B-GGUF` → `Qwen3-4B-Q4_K_M.gguf`, apache-2.0, **official Qwen GGUF**) — a
     dense transformer, i.e. the architecture §8 was actually written against, so bar #3 measures the thing §8
     claims. ⚠️ It is a **hybrid-reasoning** model that can emit `<think>` blocks, which would materially
     affect bar #2's TTFT and bar #5's grammar-constrained output. That needs its own eye.
   **I am not choosing between these.** Both are Apache-2.0 and neither is on the banned list.
2. **Fix `USiegeLlamaSettings::DefaultModelFileName`** — it points at a file that cannot exist. That is a
   TASK-409 file under single-owner law, so it is not mine to edit.
3. **TASK-411 owes a fix before bar #6 can be measured in PIE**: the CONTROL pass's Escape ends the session.
   Simplest shapes, for the programmer to choose: make the Escape/Enter/RMB observations opt-in via an
   argument, or run CONTROL last so the earlier passes bank their rows first.
4. **Re-run TASK-413 in full once (1) is ruled.** Nothing measured here needs redoing except the compile, which
   is cheap. **The re-run is one dispatch, not a re-decomposition.**

---

## 9. Machine state I changed, and what I left behind

**Repo: nothing.** `git status --porcelain` is **byte-identical** to session start (9 modified, 4 untracked —
all pre-existing). **HEAD is unmoved at `ffe3fdd`. No commit. No push. No `reset`/`clean`. `L_Arena` never
saved. `Docs/ThirdPartyNotices.md` untouched** (§2 left as its marked placeholder — see §3c).

**Machine, disclosed in full:**
- **The Unreal Editor was closed and reopened** (graceful WM_CLOSE both times, never a force-kill) and **is
  running now**, on `L_Arena`, as I found it.
- ⚠️ **I enabled Python remote execution** so console commands could be driven into PIE — MCP has no
  console-command tool, and this is Epic's own supported channel. It is **six lines appended to
  `Saved/Config/WindowsEditor/Engine.ini`**, which is **gitignored** (`.gitignore:91`, verified with
  `git check-ignore -v`), clearly commented as TASK-413 temporary, bound to **127.0.0.1 with multicast TTL 0**
  (loopback only). **A verbatim backup of the original file is at**
  `…/scratchpad/Engine.ini.TASK413.bak`.
  **I left it in place on purpose** — the re-run needs exactly this channel, and rebuilding it costs a session.
  **If the LLM lane is killed, delete that block.** This is the only residue.
- The `-game` probe process was closed gracefully; its log was **moved out of the engine's `Binaries/Win64`
  directory** into the scratchpad. **No residue under `C:\Program Files`.**

**Evidence files (scratchpad):** `build_task413.log` (the compile) · `TASK413_game_probe.log` (bar #6, all
three passes) · `probe_hf*.py` (the model probes) · `ue_exec.py` (the remote-execution driver — reusable by the
re-run) · `Engine.ini.TASK413.bak`.
The PIE evidence is in the live editor log at
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\Logs\GitClaudeUnrealTest.log`.

---
---

# PART 2 — THE SPIKE RAN. FOUR BARS MEASURED, TWO BLOCKED BY ONE CHARACTER.

**Date:** 2026-08-03 · **Author:** build-master · **Gate in:** GREEN (`qa/TASK-412.md` + re-gate; `qa/TASK-411.md` qa-passed).

> # HEADLINE: **GO-WITH-CAVEATS — but bars #2 and #5 are NOT MEASURED, because the GBNF has never once parsed.**
>
> The model is real, resident and fast. **Bar #3 PASSES at 77.1 %, bar #4 PASSES at 2.64 GiB, bar #6 PASSES
> outright.** But `llama_sampler_init_grammar` returned NULL on **every single generation of all six bench
> runs** — the grammar the whole design rests on **fails to parse**, in the spike *and* in the landed shipped
> generator. Every latency and accuracy number in this run is therefore **unconstrained** and cannot be
> reported against bars #2 or #5.
>
> **The cause is one character.** `at_least` is not a legal GBNF rule name — llama.cpp's parser reads it as
> `at` followed by `_least`. Renaming the *rule* to `at-least` (the JSON key `"at_least"` is untouched) makes
> it parse, verified against the same b10235 parser. **No commit, no push, `L_Arena` never saved, HEAD unmoved
> at `ffe3fdd`.**

---

## 10. Step 0 — the compile. GREEN.

```
[1/10] Compile [x64] SiegeLlamaSettings.cpp        [5/10] Compile [x64] SiegeAssistantInputProbe.cpp
[3/10] Compile [x64] SiegeLlamaSpike.cpp           [6/10] Link    [x64] UnrealEditor-SiegeLlama.dll
[9/10] Link    [x64] UnrealEditor-GitClaudeUnrealTest.dll
Result: Succeeded
```

- **Judged on the `Result:` line, never the exit code.** Zero `error`, zero `warning`, zero `failed` in the
  whole log, under warnings-as-errors. Both lanes rebuilt — plugin **and** game module.
- Editor closed gracefully (in-engine `quit_editor()` after `DIRTY_COUNT: 0`), exited with **no save prompt**.
  **`L_Arena` was never saved. The one-time exception is still unspent.** Log: `scratchpad/build_task413_p2.log`.

---

## 11. TWO ENVIRONMENT CORRECTIONS — AND THE FIRST ONE INVALIDATES MY OWN PART-1 FINDING

### 11a. The "~3 FPS for the first ~25 s startup window" I reported in part 1 **DOES NOT EXIST**

I reported it as nav-build + `SK_Miner` streaming, and the orchestrator carried it forward as a third re-run
condition. **It is wrong, and I am correcting it before it becomes law.**

It is the editor's **"Use Less CPU when in Background"** idle. `EditorEngine.cpp:5327-5375`:

```cpp
const bool bIsForeground = FPlatformApplicationMisc::IsThisApplicationForeground();
if (!bIsForeground && !FApp::HasFocus() && !IsRunningCommandlet() && !GIsAutomationTesting && !FApp::IsBenchmarking())
{
    bShouldThrottle = Settings->bThrottleCPUWhenNotForeground;   // default true
```

**A headless-driven PIE session is ALWAYS background, so it idles at exactly 3.0 FPS forever — not for 25 s.**
I measured it directly (`get_frame_count()` deltas against wall clock): 3.0 FPS held for **11 straight minutes**
across two full editor sessions, including after the spawn burst. A Windows `SetForegroundWindow` grab was
**refused** and changed nothing.

**Why this matters more than a footnote:** every part-1 frame-time figure, and any bench started this way, takes
its **baseline control against a ~333 ms floor**. Bar #1's entire `ATTRIBUTABLE` mechanism is a subtraction
against that baseline. **A bar #1 measured under the throttle is not conservative — it is meaningless**, and it
would have read as a spectacular PASS (inference adds nothing next to a 333 ms frame).

**Fix, and it is NOT where I first put it:** the setting is `config=EditorSettings`, i.e. the **user-global**
`%LOCALAPPDATA%/UnrealEngine/5.8/Saved/Config/WindowsEditor/EditorSettings.ini` — **not** the project's
`Saved/Config`. My first edit went to `EditorPerProjectUserSettings.ini`, had no effect, and has been reverted.
With `bThrottleCPUWhenNotForeground=False` in the right file, **PIE runs at 59.9 FPS.** Every number below was
taken at a **measured >=58 FPS**, verified immediately before each bench and again after each spawn burst.

=> **The real startup ramp is ~3 seconds** (24.8 FPS -> 59.7 FPS on the next poll), not 25.

### 11b. The device order is the **opposite** of the TASK-409 link proof

QA note 6 said the RTX 5070 would be `Vulkan1` and warned that `Vulkan0` was the Arc iGPU. Live inventory:

```
dev=0/Vulkan0/GPU  free=5311.7 total=7891.0 MiB  desc="NVIDIA GeForce RTX 5070 Laptop GPU"   <== PINNED
dev=1/Vulkan1/iGPU free=47690.3 total=37020.8 MiB desc="Intel(R) Arc(TM) 140T GPU (32GB)"
dev=2/CPU/CPU                                     desc="Intel(R) Core(TM) Ultra 9 285H"
```

I took the index off the live line as instructed => **every run below is pinned `gpu=0`**, and I verified the
`desc=` string reads *RTX 5070* on every run rather than trusting the index. **Do not hard-code either index.**
Note also `Vulkan1` reports **free (47690 MiB) > total (37020 MiB)**, which is incoherent — another reason the
iGPU must never be the subject of a VRAM figure.

**The dGPU total is 7891 MiB — this machine IS the modal 8 GB card bar #4 asks about.**

---

## 12. Step 1 — THE TOKEN COUNTS. Section 8's TRIGGER HAS FIRED.

```
SPIKE_TOKENS fixture=t0 zoneA_chars=4314 zoneB_chars=68 zoneC_chars=887 zoneBC_chars=955 MaxSnapshotChars=1440 headroom=485
SPIKE_TOKENS fixture=t0 zoneA_tok~=1139 zoneB_tok~=32 zoneC_tok~=320 zoneBC_tok~=352 (<=400 cap) assembled_total_tok=1504 chars_per_token=3.56
SPIKE_TOKENS fixture=t0 assembled_prompt_chars=5349 zoneB_start_char=4361 zoneC_start_char=4429 zoneB_start_tok~=1147 zoneC_start_tok~=1179
```

`fixture=t1`: `zoneB_chars=71 zoneC_chars=886`, **identical token counts and identical zone boundaries**
(`zoneB_start_tok~=1147 zoneC_start_tok~=1179`).

### THE WORST CHARS/TOKEN IS **2.13**, NOT 3.6 — AND THE CAP THAT MATTERS MEASURES **2.71**

Every figure below is `printed chars / printed tokens`. **Both operands are measurements.**

| zone | chars | tokens | **chars/token** |
|---|---|---|---|
| Zone A | 4314 | 1139 | 3.79 |
| **Zone B** | 68 | 32 | **2.13 <- WORST** |
| Zone C | 887 | 320 | 2.77 |
| **Zone B+C — the region `MaxSnapshotChars` actually caps** | 955 | 352 | **2.71** |
| assembled prompt | 5349 | 1504 | 3.56 |

**Section 8 assumes 3.6 and calls it conservative. It is not conservative — it is optimistic on every single
reading, including the whole-prompt average (3.56).**

**`MaxSnapshotChars = 1440` OVER-ADMITS, and by a lot.** Section 8 derived 1440 as `400 tok x 3.6`. At the
measured snapshot ratio of **2.71**, 1440 chars admits **1440 / 2.71 = 531 tokens** against a 400-token
budget — **33 % over.** To bind at 400 tokens the cap must be **400 x 2.71 = ~1085 chars** (or ~850 at Zone
B's 2.13 if section 8 wants the strict worst-zone reading).

⚠️ **Today's board does not breach it** — the live snapshot is 955 chars = **352 tok**, inside 400. **The
breach is latent**: the cap permits a snapshot 33 % over budget as soon as the roster or order line grows.
**Section 8's RESOLUTION trigger — "if the printed worst-case ratio comes back BELOW 3.6, lower
`MaxSnapshotChars` once" — HAS FIRED. This is the one-pass correction, from measurement. I am not making the
edit myself (CONVENTIONS is not build-master's file), but the number is no longer pending.**

Two further corrections this measurement settles:
- **Zone A is 1139 tokens, not section 8's "~600 as-built".** QA's mechanism was right: the landed builder
  emits `synonyms:\nnone` when `Vocabulary` is null; with the real table attached it is nearly double.
- **The assembled prompt is 1504 tok of a 2048 context.** With the 96-token output budget that is 1600/2048
  (78 %). If the snapshot ever reached the current 1440-char cap (531 tok) it would be ~1779/2048 (87 %).
  **The context headroom is thinner than section 8 implies and should be stated in TASK-423's spec.**

---

## 13. THE BLOCKER — THE GBNF HAS NEVER PARSED, IN THE SPIKE **OR** IN THE SHIPPED GENERATOR

Every generation of all six bench runs logged, at `Error` level:

```
SPIKE_WARN: llama_sampler_init_grammar returned NULL -- the GBNF FAILED TO PARSE.
Every number from this run is unconstrained and must not be reported as a constrained result.
```

**The harness was right and it said so at the top of its voice. I am reporting it, not working around it.**

### 13a. Root cause, from llama.cpp's own parser — not from reading the grammar

I extracted the runtime dump and fed it to `llama-completion.exe` from **the same b10235 build the plugin
links**:

```
parse: error parsing grammar: expecting ::= at _least ::= "1" | "2" | "3" | ...
```

**`at_least` is not a legal GBNF rule name.** llama.cpp parses a rule name as letters/digits/hyphens, so it
reads `at`, then demands `::=` and finds `_least`. llama.cpp's own grammars are kebab-case throughout.

### 13b. It is ONE identifier, and the fix is verified — not proposed

Renaming **only the rule identifier** to `at-least`, leaving the JSON key literal `"at_least"` untouched:

```gbnf
at-least ::= "1" | "2" | ... | "30"
when     ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
```

=> **parses clean, no diagnostics.** That is the entire defect. Nothing else in the grammar is wrong.

### 13c. THIS IS NOT A SPIKE BUG — THE LANDED SHIPPED GENERATOR HAS IT TOO

`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp`:
- **`:312`** `AppendRule(Grammar, TEXT("at_least"), ...)` — the rule name
- **`:392`** `Parts.Add(TEXT("at_least"));` — the reference

**CONVENTIONS section 9c names the landed grammar as THE AUTHORITY, and the authority emits a grammar
llama.cpp cannot load.** QA verified the two generators mirror each other rule-for-rule — and they do. **What
no by-derivation review could catch is that they are identically unparseable.** The spike's own `SpikeGrammar`
dump was diffed and blessed in part 1; a dump is not a parse.

=> **Both files need the rename, and `USiegeAssistantGrammar` is the one that actually ships.**

### 13d. The hybrid-reasoning risk: REAL, and FULLY NEUTRALISED by a working grammar — measured A/B

Same model, same prompt (real Zone A+B+C), `--temp 0`, only the grammar differs:

| | output |
|---|---|
| **Unconstrained** | `<think>` / `Okay, let's tackle this. The user's order is "pull the archers back to the middle." First, I need to parse the intent...` |
| **Constrained (fixed GBNF)** | `{"intent":"fallback","who":"none","where":"mid","when":"now"}` |

**Verified, not assumed: the GBNF does structurally forbid `<think>` from token 0** — `root` is
`command | question` and both begin with a `{` literal, so no thinking token is reachable. **And in-engine,
with the grammar dead, the pathology is total:** every one of the 5 warm iterations on the CPU tier produced
`output=<think>` and burned the entire budget to the 10 s abort. `total_out_tokens=206...480` of pure
reasoning prose, zero JSON, on every tier.

⚠️ **So bar #2's and bar #5's numbers in this run are not "slightly off" — they are measurements of a model
free-associating.** They are not reported against their bars.

⚠️ **One qualitative note, offered as an anecdote and explicitly NOT a score:** the single constrained
generation above answered `"who":"none"` where `[{"kind":"archer","n":"all"}]` was the better answer — it got
intent and place right and dropped the unit selection. **One row is not evidence.** It is a reason to expect
bar #5 to be genuinely contested rather than a formality.

---

## 14. THE SIX BARS

**Every bench: PIE on `L_Arena`, ~40 units fielded on both teams via `SummonTestUnit` (the shipping
`SpawnUnitSwarm` path), FPS verified >=58 immediately before each run, `gpu=0` pinned to the RTX 5070, each
tier run TWICE and the SECOND reported (QA WARN-3).** Units fight and die, so the field was topped up before
every run; the count decays from ~40 during a run, which is real contention, not a static scene.

| # | Bar | Measured (2nd run) | Verdict |
|---|---|---|---|
| 1 | No frame > 33 ms attributable to inference, **partial** tier | p99 delta **+2.02 ms** (prefill) / **+2.32 ms** (decode); worst delta +24.68 / **+33.04 ms**; **2 frames > 33 ms out of 5,297** | **PASS on p99, NOT CLEANLY SCOREABLE on worst-frame** — see 14a |
| 2 | TTFT + wall for ~60 **constrained** tokens: <=2 s partial, <=6 s CPU | **NOT MEASURED — no run was constrained.** TTFT half (grammar-independent): partial **1744 ms**, cpu **2636 ms**, full **417 ms** | ⛔ **NOT MEASURED** (TTFT half would pass) |
| 3 | KV-prefix reuse ~70 % drop | **DROP = 77.1 %**, identical on all 3 tiers and all 6 runs | ✅ **PASS** |
| 4 | Peak VRAM + RSS fits 8 GB with room | Model-attributable **`vram_delta = 2702.8 MiB`** (2.64 GiB) of a **7891 MiB** card, full offload | ✅ **PASS** — see 14b |
| 5 | >= 85 % exact match on the HOLDOUT | **NOT MEASURED. Holdout NOT opened — the one-shot is STILL UNSPENT** | ⛔ **NOT MEASURED** |
| 6 | Does a focused `UEditableTextBox` starve Enhanced Input of WASD? | **CONTROL-OK / MODE A PASS / MODE B PASS** | ✅ **PASS — ships on `FInputModeGameAndUI`** |

### 14a. Bar #1 — why I will not force this into a PASS or a FAIL

Partial tier, run 2, the reported run:

```
phase=baseline WORST=  19.32ms p99= 18.79ms  over33ms=0
phase=prefill  WORST=  44.00ms p99= 20.80ms  over33ms=1   ATTRIBUTABLE worst_delta=+24.68ms p99_delta=+2.02ms
phase=decode   WORST=  52.36ms p99= 21.11ms  over33ms=1   ATTRIBUTABLE worst_delta=+33.04ms p99_delta=+2.32ms
```

**By the bar's literal words this is a FAIL: 2 frames exceeded 33 ms during inference and 0 did during the
baseline.** Run 1 of the same tier shows the same shape (+21.36 / +32.56 ms, 1 frame each).

**But the environment itself produces those frames.** On the **full** tier's run 2 the **baseline** — no
inference running at all — recorded `WORST=42.98ms over33ms=1`. **A 40-50 ms outlier occurs in this PIE
session with the model idle.** So attributing the partial tier's single 44 ms and single 52 ms frame to
inference is **not established by this data**, and claiming a FAIL from 2 frames in 5,297 would be as
unfounded as claiming a PASS by ignoring them.

**What IS solid, and points to PASS:** the p99 attributable delta is **+2.0 to +4.6 ms on every tier**, and the
median frame never moves (16.6-16.8 ms throughout). The systematic cost of a `TPri_BelowNormal` background
thread is **~2 ms at p99** — which is the shape section 8 predicted and is comfortably inside budget.

⚠️ **And this measurement is CONSERVATIVE:** with the grammar dead, decode ran ~93 output tokens per iteration
instead of the short JSON that ships, i.e. **more** inference work over **more** frames than the real feature
will ever do. Confirmation needs a re-run once the grammar parses — the honest statement today is
**"~+2 ms at p99, with a worst-frame question the environment's own noise floor cannot resolve."**

### 14b. Bar #4 — the subject is named, and the confound is named too

```
tier=full  gpulayers=-1/36  SPIKE_MEM stage=model_load vram_dev=dev=0/Vulkan0/GPU
           vram_free_before=3320.2 MiB  vram_free_after=617.4 MiB  vram_delta=2702.8 MiB  rss_delta=2446 MiB
```

| tier | gpu layers | model-attributable `vram_delta` |
|---|---|---|
| cpu | 0/36 | 366.7 MiB *(Vulkan scratch; weights live in RAM)* |
| partial | 18/36 | 793.5 MiB |
| **full** | **36/36** | **2702.8 MiB** |

**The fully-offloaded model + KV cache at ctx=2048 costs 2.64 GiB — 34 % of a 7891 MiB card, leaving ~5.1 GB
for the game. That is "fits 8 GB with room", and it is a delta between two samples of the same named card.**

⚠️ **The `vram_free_lowwater=607.0 MiB` figure on the `bench_end` line must NOT be quoted as bar #4.** It
includes the **Unreal Editor itself**, which was rendering PIE on the same card and is not present in a
shipped title. The *delta* is the model's footprint; the *low-water* is an artefact of measuring inside the
editor. **A standalone `-game` run would tighten this, and is the one thing PIE genuinely cannot answer.**

RSS peaked at **7818 MiB** across all runs — irrelevant on a 64 GB machine, but worth recording.

### 14c. Bar #3 — the strongest result in the run, and the subtraction QA demanded

```
SPIKE_PREFILL tier=... bound=SHIPPED_WORST_CASE fixtures=t0->t1
  turn1_prompt=1517 turn1_prefill=1517 | turn2_prompt=1503 turn2_prefill=347
  reused=1156 landed_in=ZONE_B zoneB_start~=1147 zoneC_start~=1179 | DROP=77.1%
```

- **`reused - zoneB_start~ = 1156 - 1147 = 9 tokens`** — **exactly inside QA's predicted 8-10 band**
  (`[MATCH]\n` + `own_castle_hp: `, the 23 structural bytes no live board can change). **The fixtures have
  NOT drifted and the bound is not silently optimistic.** Identical on all 6 runs across all 3 tiers.
- `landed_in=ZONE_B` on every shipped-bound run — the classifier agrees with the token boundaries
  `SpikePrompt` printed independently (`zoneB_start_tok~=1147`, `zoneC_start_tok~=1179`).
- **77.1 % vs section 8's `165/765 = 78 %` reference — within 0.9 pp. That reference figure is now
  CORROBORATED BY MEASUREMENT and is no longer UNVERIFIED.**
- `BEST_CASE` is **not** quoted, per instruction. On the full/partial tiers it read `DROP=98.7 %`,
  `landed_in=ZONE_C` — the paused board, as designed.

⚠️ **WARN-R2 sprang exactly as QA predicted, and I checked before believing it.** The CPU tier's *first* run
printed `bound=BEST_CASE ... reused=0 ... DROP=0.9%` followed by **"THE PROMPT LAYOUT IS WRONG AND MUST BE
FIXED BEFORE ANYTHING ELSE IS BUILT."** The line immediately above it was
`SPIKE_WARN: decode llama_decode returned 2 at token 0` — **turn 1 aborted before it cached anything.** It is
a decode abort, **not** a layout defect; the same tier's `SHIPPED_WORST_CASE` bound was healthy at 77.1 % in
the same run, and the partial/full tiers' `BEST_CASE` bounds ran normally at 98.7 %. **The prompt layout is
fine. Do not let that line be quoted.**

### 14d. Bar #6 — de-provisionalised, and the control passed this time

Inline banked rows (preferred over the re-printed table, per QA):

```
BANKED row 1/3 — CONTROL (GameAndUI, focus on the GAME VIEWPORT) | CONTROL-OK | path=523.25cm net=48.48cm wasd=YES focus=0/208
BANKED row 2/3 — MODE A  (GameAndUI + SetKeyboardFocus)          | PASS       | path=0.00cm net=0.00cm wasd=no  focus=212/212
BANKED row 3/3 — MODE B  (UIOnly + SetKeyboardFocus)             | PASS       | path=0.00cm net=0.00cm wasd=no  focus=208/208
```

**The CONTROL passed on attempt 1 of 4** — *"the synthetic keystrokes DO reach Enhanced Input when the game
viewport holds focus, so the two focused passes below are meaningful."* That is the thing part 1 could never
get, and it is what makes the two zeros below it evidence rather than absence.

=> **ANSWER: a focused `UEditableTextBox` DOES starve Enhanced Input of WASD — the hero moved 523.25 cm with
the viewport focused and 0.00 cm with the box focused, holding focus every one of 212 frames.**
=> **MODE A PASS => the console ships on `FInputModeGameAndUI` and the camera stays live. The
`FInputModeUIOnly` fallback is NOT needed.**

**Run conditions QA required:**
- **Cursor parked over the viewport**, DPI-aware (the window is **3940x2320 physical** at 250 % scaling; my
  first attempt set a virtualized coordinate and the probe correctly reported `RMB cursor was at (2739, 1097)`
  — outside the window, so **that run's RMB rows were discarded as meaningless**). Re-run with the cursor at
  physical (1656, 883):

  | pass | RMB -> PlayerInput | `RMB cursor was at` |
  |---|---|---|
  | CONTROL | NO | (2139, 1650) |
  | **MODE A (GameAndUI)** | **YES — reached UPlayerInput** | **(1198, 1043)** |
  | **MODE B (UIOnly)** | **NO — never reached UPlayerInput** | **(1154, 1105)** |

  => **RMB is the real, measured cost of the UIOnly fallback** — it survives GameAndUI and dies under UIOnly.
  Since MODE A passed, **that cost is not incurred**. ⚠️ MODE A's RMB row is self-flagged
  *"box focused at press: NO — contaminated"* (Enter's `ClearKeyboardFocusOnCommit` dropped focus first), so
  read it as *input-mode* routing, not *focused-box* routing — both modes were sampled in the same
  unfocused-box state, which is what makes the A/B clean.
- **The keyboard was untouched for the entire run.** Every keystroke was synthetic
  (`FSlateApplication::ProcessKeyDownEvent`); the session was driven headlessly over Python remote execution
  with no human at the machine. No stray W/A/S/D could have forced the expensive FAIL direction.
- **Enter is NOT reported as an input-mode cost**, per QA's engine-source ruling. For the record both modes
  printed `Enter -> PlayerInput: NO`, which is consistent with that ruling.
- **Escape was NOT INJECTED** (`Escape policy: auto (not injected in PIE)`) — the probe now declines it in a
  PIE world rather than ending the session. ⚠️ **The Escape matrix is therefore still unmeasured and needs a
  standalone `-game` run**; it is not needed for bar #6's binary answer.

---

## 15. THE TWO CONDITIONS QA REQUIRES TRAVELLING BESIDE THE NUMBERS

1. **Bar #5's holdout condition — UNSPENT AND STILL BINDING.** `assistant_eval_holdout.csv` **was not opened
   by this task either.** I deliberately did **not** run `Siege.Llama.SpikeEval` with `holdout=`: with the
   grammar dead every row would score against `<think>` prose, and **spending a one-shot sealed holdout to
   measure a broken sampler would destroy the only clean shot at bar #5 for a number that means nothing.**
   When it is finally run: read it against **the holdout's own printed leniency floor, never dev's 48 %**
   (WARN-9); **DEV-08 asserts nothing** and six dev rows assert <=1 field; and **the number is valid only
   while TASK-416's `MaxRosterKinds = 8` seam stays open as measured** (WARN-5) — the harness scores a
   13-kind roster the shipped snapshot truncates to 8, so shipped accuracy on a >8-kind board will be
   **worse than measured**.
2. **Bar #1's run-twice condition — HONOURED.** Every tier was run twice and the **second** is reported;
   partial and full were run twice back-to-back with the model already resident, and the CPU tier got a third
   warm-up run so its reported run also had the model resident. ⚠️ **Superseded in importance by 11a: the
   run-twice rule is necessary but was never sufficient — under the background throttle both runs would have
   been equally worthless.**

**Part 1's third condition (the "~25 s startup window") is WITHDRAWN and replaced** by 11a: disable
`bThrottleCPUWhenNotForeground` in the user-global `EditorSettings.ini`, and **verify measured FPS >=58
immediately before each bench** — which is what I did, rather than trusting the setting.

---

## 16. THE VERDICT

### **GO-WITH-CAVEATS.** Not a clean GO, and emphatically not a NO-GO.

**What the measurements actually establish:**

- **The technical premise holds.** The prompt layout is right (**bar #3: 77.1 %**, corroborating section 8's
  own reference to within 0.9 pp, with the reuse point landing 9 tokens into Zone B exactly as QA derived).
  The footprint is affordable (**bar #4: 2.64 GiB on a 7891 MiB card**). The background-thread design costs
  **~2 ms at p99** (**bar #1**). The input question is answered and it went the **cheap** way
  (**bar #6: `FInputModeGameAndUI`**, no UIOnly fallback, no lost RMB/camera).
- **The two bars that could kill the feature — #2 latency and #5 accuracy — are still unmeasured**, because
  the sampler was never constrained. **That is not a NO-GO signal; it is a "not yet measured" signal**, and
  the distinction is the whole point of this report.

**Why it is not a clean GO:** bars #2 and #5 are the two Jonathan's tolerance actually lives on, and **one is
now known to be at risk in a specific way**: this model *will* emit `<think>` the instant the grammar is not
holding it, so **the grammar is not a nicety, it is load-bearing**, and until today it had never executed. The
one constrained generation I did obtain was schema-perfect but dropped the unit selection — **one row, not a
score, and I will not extrapolate it.**

**Why it is not a NO-GO:** nothing measured says the feature cannot work. Everything measured says it can, and
the single defect standing between here and a full six-bar answer is **one identifier**.

**The shortest path to a complete answer — and it is short:**

1. **`gameplay-programmer` renames the GBNF rule `at_least` -> `at-least` in BOTH files** —
   `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp:312` and `:392` (**the shipped
   authority**, and the more important of the two) and the mirrored sites in
   `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp`. **The JSON key literal `"at_least"`
   must NOT change** — it is the wire format and TASK-426's corpus asserts it. Fix verified against b10235.
   ⚠️ **Worth a sweep for any other underscored rule name before it recurs; `at_least` was the only one today.**
2. **Re-run TASK-413's bars #2 and #5 only.** Bars #1, #3, #4 and #6 do not need redoing — none depends on the
   sampler — though bar #1 is worth re-reading once decode runs at its true (shorter) length.
3. **CONVENTIONS section 8: lower `MaxSnapshotChars` from 1440 to ~1085** on the measured 2.71 chars/token,
   and replace the `DERIVED — PENDING TASK-413` figures with section 12's table. **The trigger fired; the
   guess is over.**

⚠️ **I am not giving a GO on bars #2 and #5 by inference from the other four. They are unmeasured, and a
verdict that quietly promoted four passes into six would be exactly the failure this task exists to prevent.**

---

## 17. MACHINE STATE — WHAT I CHANGED AND WHAT I LEFT

**Repo: nothing.** No commit, no push, no `reset`, no `clean`. **HEAD unmoved at `ffe3fdd`.** `git status` is
as I found it (12 modified, 6 untracked — all pre-existing; `qa/TASK-411.md` appeared from QA's own run).
**`L_Arena` was never saved** — dirty state verified `0` before every editor close, and every close was
in-engine `quit_editor()`, never a force-kill. **`Docs/ThirdPartyNotices.md` was already filled in before
part 2 began and I did not touch it.**

**Machine residue — two config blocks, both outside the repo, both with verbatim backups:**

1. ⚠️ **`%LOCALAPPDATA%/UnrealEngine/5.8/Saved/Config/WindowsEditor/EditorSettings.ini`** —
   `[/Script/UnrealEd.EditorPerformanceSettings] bThrottleCPUWhenNotForeground=False`.
   **This is a USER-GLOBAL UE 5.8 setting, not a project one — it affects every project Jonathan opens** (the
   editor will no longer idle in the background, which costs CPU/battery on a laptop).
   **LEFT IN PLACE ON PURPOSE: the re-run for bars #2/#5 is worthless without it** (11a).
   **Backup: `scratchpad/EditorSettings.TASK413.bak`. To restore: copy it back, or delete the block marked
   `---- TASK-413 TEMPORARY ----` at the end of the file.**
2. **`Saved/Config/WindowsEditor/Engine.ini`** — the Python remote-execution block (loopback only, multicast
   TTL 0). **GITIGNORED** (`.gitignore:91`, re-verified with `git check-ignore -v`).
   ⚠️ **The editor REWRITES this file on every shutdown and silently drops the block** — it must be
   re-appended after each editor close, which is why part 1's "left in place" did not survive. Backup:
   `scratchpad/Engine.ini.TASK413.bak`.
   *(My incorrect first edit to `EditorPerProjectUserSettings.ini` has been fully reverted from backup.)*

**The editor is running, on `L_Arena`, PIE ended, exactly as I found it.**

**Norton:** ⚠️ `scratchpad/vulkan/llama-cli.exe` was quarantined mid-session (`IDP.Generic`). **It affected
nothing.** It is not ours, nothing in the project needs it, and `llama-completion.exe` from the same release
served the identical purpose. **Verified after the fact, not assumed: all 19 vendored DLLs present in
`Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/`, `Models/Qwen3-4B-Q4_K_M.gguf` intact at
2,497,280,256 bytes, and zero DLL-load / access-denied errors in any of the six bench logs.**
⚠️ **To be unambiguous: the GBNF parse failure is NOT a Norton symptom.** It is deterministic, reproduced in
6/6 in-engine runs *and* offline on two separate binaries, it names a grammar token rather than a file, and a
one-character rename fixes it. **It is a real defect and must not be laundered into "antivirus interfered".**

**Evidence (all in `scratchpad/`):** `build_task413_p2.log` (compile) · `t413_bench_{cpu,partial,full}_r*.log`
(six bench runs) · `t413_prompt_t0.log`, `t413_prompt_t1.log` (token counts) · `t413_gbnf_raw.log`,
`spike.gbnf` (as-built, fails), `spike_fixed.gbnf` (renamed, parses) · `cons_out.txt` / `unc_out.txt` (the
constrained/unconstrained A/B) · `t413_probe.log`, `t413_probe2.log` (bar #6) · `spike.py`, `pie_driver.py`,
`ue_exec.py` (the harness — reusable by the re-run) · `EditorSettings.TASK413.bak`, `Engine.ini.TASK413.bak`.

---
---

# PART 3 — ALL SIX BARS ARE NOW MEASURED. THE GRAMMAR PARSES. BAR #5 FAILS.

**Date:** 2026-08-03 · **Author:** build-master · **Gate in:** GREEN (grammar fix landed in both generators; measurement re-run of bars #2 and #5 only).

> # HEADLINE: **GO-WITH-CAVEATS — and the caveat is now a NUMBER, not an unknown.**
>
> **The grammar loads. Proven three ways, not inferred from silence.** With it live, **bar #1 improves,
> bar #3 reproduces exactly at 77.1 %, and bar #2 splits on a wording ambiguity I will not resolve in my own
> favour.** And the one that matters: **bar #5 is MEASURED and it FAILS — HOLDOUT 10/15 = 66.7 % against an
> 85 % bar, with the runner's own printed floor at 60.0 %.**
>
> **No commit, no push, `L_Arena` never saved, HEAD unmoved at `ffe3fdd`.**

---

## 18. Step 0 — the compile. GREEN.

```
[1/13] Compile [x64] SiegeAssistantGrammarTest.cpp     [5/13] Compile [x64] SiegeLlamaSpike.cpp
[2/13] Compile [x64] SiegeAssistantGrammar.cpp         [7/13] Link    [x64] UnrealEditor-SiegeLlama.dll
[3/13] Compile [x64] SiegeAssistantSnapshot.cpp        [12/13] Link   [x64] UnrealEditor-GitClaudeUnrealTest.dll
Result: Succeeded
```

- **Judged on the `Result:` line, never the exit code.** (Exit code was also 0; it was not what I judged on.)
- **Zero `error`, zero `warning`, zero `failed`** in the whole log, under warnings-as-errors.
- **Both lanes rebuilt** — plugin **and** game module, as required. Log: `scratchpad/build_task413_p3.log`.
- Editor closed **gracefully** before the build (in-engine `quit_editor()` after `DIRTY_COUNT: 0`), exited with
  **no save prompt**. **`L_Arena` never saved; the one-time exception is STILL unspent.**

⚠️ **Environment re-verified rather than assumed, and it was half-right:**
- `EditorSettings.ini` `bThrottleCPUWhenNotForeground=False` — **SURVIVED** the editor's shutdown rewrite. ✅
- The gitignored `Engine.ini` python-remote-exec block — **GONE AGAIN**, exactly as part 2 predicted. Re-appended
  before relaunch. **This confirms the part-2 warning as a standing procedure item, not a one-off.**
- **Measured FPS before benching: 59.7-59.8**, and re-settled to >=58 after the spawn burst. 37 units fielded on
  both sides via the shipping `SummonTestUnit` path. **The ~3 FPS pathology did not recur.**

---

## 19. THE GRAMMAR PARSES. STATED AS A FIRST-CLASS RESULT.

**The spike logs ONLY on failure — there is no success line.** So "no NULL error" is *absence of evidence*, and
I will not report it as the proof. I built a positive one, three independent ways:

### 19a. In-engine A/B — the decisive one

Same build, same model, same prompt, same fixtures; **only `grammar=` differs**:

| run | result |
|---|---|
| **`grammar=0`** (negative control) | `output=<think>` on **2 / 2** iterations |
| **`grammar=1`** | schema-perfect JSON on **10 / 10** iterations across two runs |

```
grammar=0 : SPIKE_LATENCY   iter=0 output=<think>
grammar=1 : SPIKE_LATENCY   iter=0 output={"intent":"send","who":[{"kind":"footman","n":12},
            {"kind":"archer","n":2},{"kind":"sorcerer","n":1}],"where":"ancient_ground_far",
            "when":{"kind":"knight","at_least":4}}
```

=> **The grammar flag demonstrably changes the output distribution.** A NULL sampler cannot do that. **The
sampler is non-NULL AND constraining** — that is the proof, and it is a measurement, not an inference.

### 19b. Aggregate sweep — every part-3 log

| log | `init_grammar returned NULL` | `<think>` | truncation |
|---|---|---|---|
| **all 12 part-3 logs** | **0** | **0 — except the grammar=0 control (2)** | **0** |

**`<think>` appears in exactly one place: the negative control.** Nowhere else, across 30 constrained bench
generations + 40 constrained eval rows.

### 19c. Offline parse of today's *runtime* bytes — the §9c "a dump is not a parse" answer

Extracted the live 1418-char dump and fed it to **`llama-completion.exe` from the same b10235 build the plugin
links** — the same binary that produced part 2's `expecting ::= at _least` error:

```
(zero parse diagnostics)
output: {"ask":"how_many"}      <- a legal `question` production, grammar-constrained
```

**13 rules, all legal identifiers**, `at-least` the only hyphenated one, and the JSON key `"at_least"` intact on
the `when` rule. Byte-identical to the fix verified in part 2.

### 19d. The hybrid-reasoning question — now FALSIFIABLE, and answered

Part 2 could not test this because every run was unconstrained. Now it can be:
**Qwen3-4B emits `<think>` 100 % of the time unconstrained and 0 % of the time constrained.** Combined with the
structural argument (`root` -> both branches open with a `{` literal, so no thinking token is reachable from
token 0), **the `<think>` pathology is closed. `parse_failures=0` on all 40 eval rows corroborates it.**
**No thinking-token pathology appeared in bar #2 or bar #5. This is a clean negative finding, not a footnote.**

---

## 20. BAR #2 — THE BAR'S OWN WORDING HAS TWO READINGS, AND THEY DISAGREE. I AM NOT PICKING ONE.

The board (line 4080) states the measure as **"TTFT + wall-clock for ~60 constrained tokens, warm prefix"**
against **"<= 2 s partial · <= 6 s CPU-only"** — one threshold for two quantities. In part 2 only TTFT was
available, so the ambiguity was invisible. **With constrained decoding it is decisive**, so it must be Jonathan's
call and not mine:

**Every tier run TWICE, the SECOND reported. `gpu=0` pinned, `desc=` verified as the RTX 5070 on every load.**

| tier | mean TTFT | worst TTFT | **mean wall** | worst wall | out tokens | aborts |
|---|---|---|---|---|---|---|
| **partial (18/36)** — *bar's tier* | **1655.4 ms** | 1705.6 ms | **6472.0 ms** | 8926.3 ms | 19-57 (194 tot) | 0 / 5 |
| **cpu (0/36)** — *bar's tier* | **2716.1 ms** | 2913.8 ms | **8729.6 ms** | 10000.6 ms | 19-43 (173 tot) | **2 / 5** |
| full (36/36) — *context, not the bar's tier* | 417.3 ms | 434.1 ms | 2106.1 ms | 2937.5 ms | 19-56 (193 tot) | 0 / 5 |

```
partial r2: SUMMARY iters=5 mean_ttft_ms=1655.4 mean_wall_ms=6472.0 WORST_wall_ms=8926.3 total_out_tokens=194
cpu     r2: SUMMARY iters=5 mean_ttft_ms=2716.1 mean_wall_ms=8729.6 WORST_wall_ms=10000.6 total_out_tokens=173
full    r2: SUMMARY iters=5 mean_ttft_ms=417.3  mean_wall_ms=2106.1 WORST_wall_ms=2937.5 total_out_tokens=193
```

### The two readings

| reading | partial (<=2000 ms) | cpu (<=6000 ms) | verdict |
|---|---|---|---|
| **Bar governs TTFT** | 1655.4 PASS | 2716.1 PASS | **PASS both** |
| **Bar governs wall-clock** | 6472.0 FAIL (3.2x) | 8729.6 FAIL (1.5x) | **FAIL both** |

**I am not choosing.** For the record, the plain-English reading of a player-facing latency budget is the
**wall clock** — the time until the order is actually understood — and under that reading **bar #2 fails on both
named tiers.** But the board's own harness prints the bar string on the line carrying *both* means, so the
ambiguity is real and predates this run.

### Three things that matter more than the arithmetic

1. ⚠️ **"~60 constrained tokens" is a hypothetical this feature never reaches.** Real constrained output is
   **19-57 tokens** (mean ~39) — it stops at EOS after valid JSON. **The bar is therefore measured against a
   longer response than the feature emits**, i.e. the wall figures above are already generous.
   ⚠️ **`wall60_equiv_ms` (8.3-16.3 s) is an EXTRAPOLATION printed by the harness. I am NOT reporting it as a
   measurement and it must not be quoted as one.**
2. ⛔ **The CPU tier did not finish 2 of 5 generations.** `iter=0` and `iter=3` hit the harness's 10 s ceiling
   and were logged `ABORTED` with **truncated JSON** (`..."where":"ancient_ground_far` / `..."where":"enemy`).
   **A truncated command is not a slow command — it is an unusable one.** Under *any* reading of the bar, the
   CPU tier as configured does not reliably produce a parseable order. **This is the single hardest fact in
   bar #2** and it is invisible if you only read the means.
3. **Decode, not prefill, is the whole cost.** ms/token: **full ~36-49 · partial ~117-131 · cpu ~156-182.**
   TTFT is nearly flat within a tier; the spread is all decode. Jonathan's ruling (i) requires a CPU fallback —
   **the CPU fallback is where this bar actually hurts.**

---

## 21. BAR #5 — THE HOLDOUT. THE ONE-SHOT IS SPENT. **66.7 % vs an 85 % BAR. FAIL.**

**Run ONCE, on `tier=full gpu=0`** (the only tier with zero aborts — a truncated generation would have scored as
a wrong answer and corrupted the number). **Reported exactly as printed. No second run, no cherry-pick.**

```
SPIKE_EVAL_SCORE split=HOLDOUT rows=15 PRIMARY(lenient)=10/15 = 66.7% | STRICT=10/15 = 66.7%
                 | parse_failures=0 | clarify_rows_passed_by_question=0 | assertions_compared=35
SPIKE_EVAL_SCORE split=HOLDOUT LENIENCY FLOOR: a degenerate model that answers {"ask":...} to EVERY sentence
                 would score 9/15 = 60.0% under the PRIMARY rule (it fails every Execute row).
SPIKE_EVAL_SCORE split=HOLDOUT 5 row(s) assert at most ONE field: HOLD-05 HOLD-07 HOLD-11 HOLD-13 HOLD-14
SPIKE_EVAL_SCORE split=HOLDOUT deferred-trigger agreement (NOT part of bar #5's four fields) = 0/1
```

- **STRICT AND LENIENT ARE IDENTICAL — 10/15 both.** `clarify_rows_passed_by_question=0`. **No part of this
  score is a leniency artefact**, which removes the usual objection in both directions.
- ⚠️ **THE HOLDOUT'S OWN PRINTED FLOOR IS 60.0 %, NOT DEV'S 48 %** (WARN-9 honoured). **The measured 66.7 % is
  only 6.7 pp — one single row — above what a model with zero translation ability scores on this split.**
  **On this split the headline number has very little discriminating power, and that cuts against the result,
  not for it.**
- **`parse_failures=0` on all 15 rows** — every output was grammar-legal JSON. **The failure is semantic, not
  structural. The grammar is doing its job; the model is not.**
- **Dev split, for context only and NOT the bar: 18/25 = 72.0 %** (strict identical), floor **12/25 = 48.0 %**,
  6 thin rows, **DEV-08 asserts nothing at all**. **The dev number is not the go/no-go number.**
- ✅ **TRUNCATION CHECKED ON EVERY ROW, NOT JUST THE FIRST** (the 6-char margin): **zero** `Snapshot roster
  degraded` / `TRUNCATED` lines across all 40 eval rows and all 12 part-3 logs. `SPIKE_TOKENS` confirms
  `zoneC_chars=887` against an 893 budget and `zoneBC_tok~=352` inside the 400 cap. **The prompt was NOT
  truncated at any point, so the number stands as a measurement.**
- ✅ `MaxSnapshotChars=1085 headroom=130` printed as expected — **the corrected value, not a regression.**

### 21a. The five failures — and they are a coherent story, not noise

| row | expect | sentence | model emitted | broke |
|---|---|---|---|---|
| HOLD-01 | Execute | "send 12 footmen and 4 **bowmen** to the far ancient ground" | `longbowman` x4 | kinds, counts |
| HOLD-07 | **Refuse** | "send the **trebuchets** at them" | `send sapper x1 -> enemy_castle` | intent |
| HOLD-08 | Execute | "send **the wizard**" | `{"kind":"wizard","n":"all"}` | counts |
| HOLD-10 | Clarify | "attack with the archers" | `{"intent":"charge","who":"none","where":"none"}` | kinds, counts |
| HOLD-13 | **Refuse** | "**spend my gold** on another ogre" | `send miner x1 -> nearest_mine` | intent |

**Four distinct mechanisms, all actionable, none requiring a bigger model as the first move:**

1. ⛔ **REFUSAL IS THE WORST FAILURE, AND IT IS SAFETY-SHAPED. 2 of 3 Refuse rows became executable orders.**
   HOLD-07 invents a `sapper` for a unit type (*trebuchet*) **that does not exist**; HOLD-13 turns
   *"spend my gold"* — **which Jonathan's DECIDED ruling (iii) says the AI must never do** — into a mining order.
   **The grammar cannot prevent either**: it admits all 13 kinds and all 7 intents regardless of board state or
   policy, so an out-of-scope request always has a plausible-looking legal production. **`{"ask":"unsupported"}`
   exists in the grammar and the model simply does not route to it.** HOLD-14 shows it *can* (it emitted
   `got=question` and passed). ⚠️ **This is the one failure class that is not merely an accuracy point — a
   silently-executed refused order is worse than no assistant.**
2. **Unit selection silently dropped** (HOLD-10 -> `"who":"none"`). ⚠️ **This is exactly the pathology I flagged
   in PART 2 §13d as a single anecdote and explicitly refused to extrapolate. It has now reproduced on the
   holdout. The anecdote was real.**
3. **Vocabulary/synonym gap** (HOLD-01: *"bowmen"* -> `longbowman`, expected `archer`). Cheapest possible fix —
   a synonym-table entry, which is `DA_AssistantVocabulary` data, not code.
4. **Determiner->quantity** (HOLD-08: *"the wizard"* singular -> `n:"all"`).

⚠️ **THE SEAL IS NOW SPENT AND THESE FIVE ROWS ARE BURNED AS TUNING SIGNAL.** The runner said it itself:
*"DO NOT TUNE ANYTHING AFTER THIS POINT."* **Any fix informed by §21a MUST be validated against a FRESH holdout.
Re-scoring this file after tuning would be measuring the training set, and the resulting number would be
worthless.** I am reporting the taxonomy because Jonathan needs it to decide — not as a to-do list to tune against.

---

## 22. BAR #3 — RE-CONFIRMED. NOTHING MOVED.

**7 `SHIPPED_WORST_CASE` bounds across all 3 tiers and 6 bench runs — `DROP=77.1%` on every single one.**

```
turn1_prompt=1517 turn1_prefill=1517 | turn2_prompt=1503 turn2_prefill=347
reused=1156 landed_in=ZONE_B zoneB_start~=1147 zoneC_start~=1179 | DROP=77.1%
```

- **`reused - zoneB_start~ = 1156 - 1147 = 9 tokens`** — identical to part 2, still inside QA's predicted 8-10
  band. **No fixture and no zone boundary moved**, exactly as expected. ✅
- §8's `165/765 = 78 %` reference stays corroborated to within 0.9 pp.
- ⚠️ **WARN-R2 SPRANG AGAIN ON THE CPU TIER AND I CHECKED BEFORE BELIEVING IT — AGAIN.** `bound=BEST_CASE
  reused=0 DROP=0.9%` + *"THE PROMPT LAYOUT IS WRONG AND MUST BE FIXED BEFORE ANYTHING ELSE IS BUILT."* The two
  lines immediately above it: `prefill llama_decode returned 2 at offset 1024` and `decode llama_decode returned
  2 at token 4` — **turn 1 aborted before caching anything.** The same run's SHIPPED bound is healthy at 77.1 %.
  **Decode abort, NOT a layout defect. The prompt layout is fine. Do not let that line be quoted.**

---

## 23. BAR #1 — RE-READ AT THE TRUE DECODE LENGTH, AND IT IMPROVED

Part 2 flagged its own bar #1 as *conservative* because the broken grammar ran ~93 output tokens per iteration.
With the real short JSON it is materially better. Partial tier (the bar's tier), run 2:

```
phase=baseline WORST= 18.80ms p99= 18.77ms median=16.70ms over33ms=0
phase=prefill  WORST= 49.27ms p99= 19.59ms median=16.67ms over33ms=1  ATTRIBUTABLE worst_delta=+30.48ms p99_delta=+0.82ms
phase=decode   WORST= 22.87ms p99= 19.20ms median=16.64ms over33ms=0  ATTRIBUTABLE worst_delta= +4.07ms p99_delta=+0.43ms
```

- **p99 attributable is now +0.82 ms (prefill) / +0.43 ms (decode)** — down from part 2's +2.02/+2.32.
- **Decode now has ZERO frames over 33 ms** (part 2 had 1). Median never moves (16.6-16.7 ms).
- **The FULL tier's run 2 has ZERO frames over 33 ms in EVERY phase** (baseline 0, prefill 0, decode 0).
- ⚠️ **The worst-frame ambiguity is unchanged and is still the environment's:** **full tier run 1's BASELINE —
  model completely idle — recorded `WORST=41.37ms over33ms=1`.** A >33 ms frame occurs in this PIE session with
  no inference at all, so the single 49.27 ms prefill frame in 3,959 samples **cannot be cleanly attributed to
  inference.** Same honest position as part 2, now on better numbers.

**Bar #4 also re-confirmed incidentally:** full-offload `vram_delta = 2701.8 MiB` vs part 2's 2702.8 MiB — **1 MiB
apart on a 7891 MiB card.** (Partial read 1264.0 MiB this session vs 793.5 in part 2 — the partial tier's delta
is sensitive to what was already resident; **full is the bar #4 subject and it reproduces.**)

---

## 24. THE COMPLETE SIX-BAR TABLE

**Conditions for every row: PIE on `L_Arena`, 37 units fielded on BOTH sides via the shipping `SummonTestUnit`
path, FPS verified 59.7-59.8 immediately before benching and re-settled after the spawn burst, `gpu=0` pinned
with `desc=` verified as the RTX 5070 on every load, each tier run TWICE and the SECOND reported.**

| # | Bar | Measured | Verdict |
|---|---|---|---|
| 1 | No frame > 33 ms attributable to inference (**partial**) | p99 delta **+0.82 ms** prefill / **+0.43 ms** decode; decode **0** frames >33 ms; prefill **1** of 3,959 at 49.27 ms — but **baseline with model IDLE hit 41.37 ms** | ✅ **PASS on p99 · worst-frame NOT CLEANLY ATTRIBUTABLE** (improved vs part 2) |
| 2 | TTFT + wall for ~60 constrained tokens: <=2 s partial, <=6 s cpu | partial **TTFT 1655 ms / wall 6472 ms**; cpu **TTFT 2716 ms / wall 8730 ms**, **2 of 5 ABORTED with truncated JSON** | ⚠️ **PASS on the TTFT reading · FAIL on the wall-clock reading — BAR WORDING MUST BE RULED.** CPU aborts are a FAIL under any reading |
| 3 | KV-prefix reuse ~70 % drop | **DROP = 77.1 %** on 7/7 bounds, all 3 tiers; reuse lands **9 tokens** into Zone B | ✅ **PASS — reproduced exactly** |
| 4 | Peak VRAM + RSS fits 8 GB with room | full-offload **`vram_delta = 2701.8 MiB`** (2.64 GiB) of **7891 MiB** = 34 % | ✅ **PASS — reproduced to 1 MiB** |
| 5 | **>= 85 % exact match on the HOLDOUT** | **HOLDOUT 10/15 = 66.7 %** (strict identical), **holdout's own floor 60.0 %**, `parse_failures=0` | ⛔ **FAIL — 18.3 pp below the bar, and only 6.7 pp above the degenerate floor** |
| 6 | Does a focused `UEditableTextBox` starve Enhanced Input of WASD? | CONTROL-OK / MODE A PASS / MODE B PASS (part 2; sampler-independent, not re-run) | ✅ **PASS — ships on `FInputModeGameAndUI`** |

---

## 25. THE TWO CONDITIONS QA REQUIRES TRAVELLING BESIDE THE NUMBERS — BOTH NOW SPENT AND BINDING

1. ⚠️ **BAR #5 IS CONDITIONAL ON TASK-416's `MaxRosterKinds` SEAM** (WARN-5). **The harness scored a 13-kind
   roster that the shipped snapshot truncates to 8.** No truncation fired in *this* run (the fixture roster fits),
   **so 66.7 % is the number for a board that fits the cap — shipped accuracy on a >8-kind board will be WORSE
   than measured, not better.** The bar is failed at 66.7 %; the seam can only push it further down.
   Also binding: **read against the HOLDOUT's own floor of 60.0 %, never dev's 48 %**; **5 holdout rows assert
   <=1 field** (HOLD-05/07/11/13/14) so the split tests less than 15 full comparisons implies —
   `assertions_compared=35`, not 60.
2. ✅ **BAR #1's RUN-TWICE CONDITION — HONOURED.** Every tier run twice, second reported, model already resident.
   Superseded in importance by the throttle rule, which was **verified live (59.7 FPS) and not assumed.**

**A third, carried from part 2 and now CONFIRMED AS STANDING:** the gitignored `Engine.ini` remote-exec block
**is dropped on every editor shutdown**. It was gone again this session. **Whoever re-runs must re-append it
after every editor close** — it is not a one-off.

---

## 26. THE VERDICT — WRITTEN FOR JONATHAN

### **GO-WITH-CAVEATS. The engineering works; the accuracy does not — yet.**

**What is now settled beyond argument.** The plumbing is real and it is good. The prompt layout is right
(**bar #3, 77.1 %**, reproduced to the token across six runs and three tiers). The model fits comfortably on a
modal 8 GB card (**bar #4, 2.64 GiB of 7.9 GB**, reproduced to 1 MiB). The background-thread design costs
**under 1 ms at p99** and never moves the median (**bar #1**). The input question went the cheap way
(**bar #6**). And the defect that blocked part 2 — **one character in a rule name** — is fixed, verified by a
real parse, and the `<think>` pathology that made this model risky is **structurally dead** whenever the grammar
is attached.

**What fails, and it is the thing the whole spike existed to find.** **Bar #5: 66.7 % against 85 %.** That is
not a near-miss. And the honest reading is worse than the gap suggests: **the holdout's own degenerate floor is
60 %**, so the model is one row — *one* — above a strategy with no language understanding at all. `STRICT` and
`LENIENT` agree exactly, so there is no scoring generosity to argue about, and `parse_failures=0` means **the
grammar is not the problem. The model's comprehension is.**

**The failures are not random, and that is the good news inside the bad.** Of five misses, **two are refusals
that became live orders** — including *"spend my gold on another ogre"*, which your own ruling (iii) says the
assistant must never act on, and *"send the trebuchets"*, a unit that does not exist. **The grammar cannot catch
these by construction**: it admits every kind and every intent regardless of the board, so an out-of-scope
sentence always has a legal-looking answer. The remaining three are ordinary comprehension gaps — a missing
synonym (*"bowmen"*), a determiner read as a quantifier (*"the wizard"* -> all), and one dropped unit selection.
**Three of the five look like few-shot and vocabulary work — the first two rungs of the plan's own escalation
ladder — and none of them argues for a bigger model yet.**

**Bar #2 is a genuine open question I refuse to close for you.** The board says *"TTFT + wall-clock ... <= 2 s"* —
one threshold, two quantities. **On TTFT both named tiers pass. On wall-clock both fail**, partial by 3.2x.
I have given you both numbers rather than the flattering one. What is *not* ambiguous: **the CPU tier failed to
finish 2 of 5 generations inside 10 seconds and emitted truncated JSON.** Your ruling (i) makes the CPU fallback
mandatory, and **as configured it does not reliably produce a usable order.** Meanwhile **full offload is
comfortable — 417 ms TTFT, 2.1 s wall, zero aborts, zero hitches over 33 ms** — and bar #4 says it fits. **If
this ships, it ships on full offload; the CPU path needs its own decision.**

**Why this is not a NO-GO:** nothing measured says the feature cannot work. Latency on the tier that matters is
fine, the footprint is fine, the frame cost is negligible, and the grammar guarantees structurally valid output
every time. **Why it is not a clean GO:** the assistant currently mistranslates one order in three and will
execute an order it was supposed to refuse. **That is a correctness-and-trust problem, not a performance one.**

**What I would put in front of you as the next decision — offered as measurement input, not a decision:**

1. **Rule bar #2's wording** (TTFT or wall clock). It changes a PASS into a FAIL and it is not build-master's call.
2. **Decide the CPU fallback's fate** — it is the only bar-2 fact that is unambiguous, and it is bad.
3. **Bar #5 is a fail, so the plan's own ladder applies: better few-shots -> tighter grammar -> bigger model ->
   only then fine-tune.** The refusal failures probably need a mechanism the grammar does not currently have
   (a policy/refusal route), not a better prompt.
4. ⚠️ **A FRESH HOLDOUT MUST BE AUTHORED BEFORE ANY TUNING IS SCORED.** This one is spent, and §21a burned five
   of its fifteen rows as signal.

⚠️ **I am not converting four passes and one ambiguity into a GO. Bar #5 is measured, it is a FAIL, and the
number is 66.7 %.**

---

## 27. MACHINE STATE — WHAT I CHANGED AND WHAT I LEFT

**Repo: nothing.** No commit, no push, no `reset`, no `clean`. **HEAD unmoved at `ffe3fdd`.** `git status` is as
I found it (17 modified, 7 untracked — all pre-existing; `qa/TASK-416.md` appeared from QA's parallel run, not
mine). **`L_Arena.umap` untouched — mtime still Jul 29 03:53**; `DIRTY_COUNT: 0` verified before the editor
close and again after PIE ended; every close was in-engine `quit_editor()`, **never a force-kill.**
**Board: `#### TASK-413` only.**

**Machine residue — unchanged from part 2, both outside the repo, both backed up:**
1. **`%LOCALAPPDATA%/UnrealEngine/5.8/Saved/Config/WindowsEditor/EditorSettings.ini`** —
   `bThrottleCPUWhenNotForeground=False`. **USER-GLOBAL: affects every project Jonathan opens.** Survived this
   session's shutdown. **Left in place.** Restore: `scratchpad/EditorSettings.TASK413.bak`.
2. **`Saved/Config/WindowsEditor/Engine.ini`** — python remote-exec block, loopback only (bind 127.0.0.1,
   multicast TTL 0). **GITIGNORED** (`.gitignore:91`, re-verified with `git check-ignore -v`).
   ⚠️ **Dropped again by the editor's shutdown rewrite and re-appended — this is now a CONFIRMED standing
   behaviour, not a one-off.**

**The editor is running, on `L_Arena`, PIE ended, exactly as I found it.**

**Evidence (all in `scratchpad/`):** `build_task413_p3.log` (compile) · `t413_p3_partial_r{1,2}.log`,
`t413_p3_cpu_r{1,2}.log`, `t413_p3_full_r{1,2}.log` (six bench runs) · `t413_p3_ctrl_gram0.log` (the negative
control) · `t413_p3_eval_dev.log`, `t413_p3_eval_holdout.log` (bar #5) · `t413_p3_gbnf.log`, `p3_runtime.gbnf`
(the runtime bytes), `p3_offline_parse.txt` (the offline parse) · `t413_p3_prompt_t0.log` (token counts) ·
`t413_p3_load_partial.log`.
