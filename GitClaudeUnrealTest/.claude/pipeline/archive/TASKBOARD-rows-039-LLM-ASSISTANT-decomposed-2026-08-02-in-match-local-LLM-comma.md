<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-413 — [LLM-1c] 🎯 RUN THE SPIKE — the six measurements on `L_Arena` with units on the field, and the three-way VERDICT (build-master)
- assignee: build-master
- status: ✅ **DONE — ALL SIX BARS MEASURED. VERDICT: GO-WITH-CAVEATS** (2026-08-03, `handoffs/TASK-413-buildmaster.md` **PART 3**). ✅ **STEP 0 COMPILE GREEN** — `Result: Succeeded`, judged on the log line never the exit code, **zero diagnostics** under warnings-as-errors, plugin **and** game module both relinked. ✅✅ **THE GBNF BLOCKER IS CLOSED — THE GRAMMAR PARSES, PROVEN THREE WAYS AND NOT INFERRED FROM THE ABSENCE OF AN ERROR** (the spike logs ONLY on failure, so silence is not proof): **(a) in-engine A/B — `grammar=0` ⇒ `output=<think>` on 2/2 iterations; `grammar=1` ⇒ schema-perfect JSON on 10/10** across two runs, e.g. `{"intent":"send","who":[{"kind":"footman","n":12},{"kind":"archer","n":2},{"kind":"sorcerer","n":1}],"where":"ancient_ground_far","when":{"kind":"knight","at_least":4}}` — **the flag demonstrably changes the output distribution, which a NULL sampler cannot do**; **(b) aggregate sweep: 0 `init_grammar returned NULL` in ALL 12 part-3 logs, and `<think>` appears in EXACTLY ONE PLACE — the negative control**, across 30 constrained bench generations + 40 constrained eval rows; **(c) offline parse of today's LIVE 1418-char runtime dump through the SAME b10235 binary that produced part 2's `expecting ::= at _least` error ⇒ zero diagnostics, constrained output `{"ask":"how_many"}`.** **13 rules, all legal identifiers, `at-least` the only hyphenated one, JSON key `"at_least"` intact.** ✅ **HYBRID-REASONING RISK CLOSED AND NOW FALSIFIABLE:** Qwen3-4B emits `<think>` **100 % unconstrained / 0 % constrained**; `parse_failures=0` on all 40 eval rows. **No thinking-token pathology in bar #2 or #5.** 📊 **THE SIX BARS** (PIE on `L_Arena`, **37 units BOTH sides** via the shipping `SummonTestUnit` path, **FPS verified 59.7-59.8 live before each bench**, `gpu=0` pinned with `desc=` verified as the RTX 5070 on every load, **each tier run TWICE and the SECOND reported**): **#1 hitch — ✅ PASS on p99, IMPROVED vs part 2** now that decode runs at its true short length: partial ATTRIBUTABLE **p99 +0.82 ms prefill / +0.43 ms decode** (was +2.02/+2.32), **decode 0 frames >33 ms** (was 1), median never moves; **the FULL tier has ZERO frames >33 ms in EVERY phase**. ⚠️ **Worst-frame still NOT cleanly attributable — full run 1's BASELINE with the model IDLE hit `WORST=41.37 ms`**, so the single 49.27 ms prefill frame in 3,959 samples is within the environment's own noise. **#2 latency — ⚠️ MEASURED, AND THE BAR'S OWN WORDING HAS TWO READINGS THAT DISAGREE. NOT RESOLVED BY BUILD-MASTER.** The board says “TTFT **+** wall-clock … ≤ 2 s partial · ≤ 6 s cpu” — one threshold, two quantities. **partial: TTFT 1655.4 ms / wall 6472.0 ms** · **cpu: TTFT 2716.1 ms / wall 8729.6 ms** · (full, context only: **417.3 ms / 2106.1 ms, 0 aborts**). ⇒ **TTFT reading = PASS both tiers; WALL-CLOCK reading = FAIL both (partial by 3.2×).** ⛔ **UNAMBIGUOUS AND BAD: the CPU tier ABORTED 2 of 5 generations at the 10 s ceiling with TRUNCATED JSON** (`…"where":"ancient_ground_far`) — **a truncated command is unusable, not merely slow**, and ruling (i) makes the CPU fallback mandatory. ⚠️ **“~60 constrained tokens” is a hypothetical the feature never reaches — real output is 19-57 tokens (mean ~39), so the wall figures are already generous; `wall60_equiv_ms` is an EXTRAPOLATION and MUST NOT be quoted as a measurement.** Decode is the whole cost (ms/token: full ~36-49 · partial ~117-131 · cpu ~156-182). **#3 KV reuse — ✅ PASS, REPRODUCED EXACTLY: DROP = 77.1 % on 7/7 SHIPPED_WORST_CASE bounds across all 3 tiers and 6 runs**, `reused=1156`, `landed_in=ZONE_B`, **`reused − zoneB_start~ = 9 tokens`, identical to part 2 — no fixture and no zone boundary moved.** ⚠️ **WARN-R2 SPRANG AGAIN on the cpu tier and I CHECKED BEFORE BELIEVING IT AGAIN**: `DROP=0.9%` + “THE PROMPT LAYOUT IS WRONG” was immediately preceded by `prefill llama_decode returned 2 at offset 1024` — **a decode abort, NOT a layout defect**; the same run's SHIPPED bound was healthy at 77.1 %. **Do not let that line be quoted.** **#4 VRAM — ✅ PASS, REPRODUCED TO 1 MiB: full-offload `vram_delta = 2701.8 MiB`** (2.64 GiB) of a **7891 MiB** card = 34 %. **#5 accuracy — ⛔ FAIL. THE ONE-SHOT IS SPENT. `SPIKE_EVAL_SCORE split=HOLDOUT rows=15 PRIMARY(lenient)=10/15 = 66.7% | STRICT=10/15 = 66.7% | parse_failures=0 | assertions_compared=35` — 18.3 pp BELOW the 85 % bar.** Run ONCE on `tier=full gpu=0` (the only tier with zero aborts), reported exactly as printed, **no second run and no cherry-pick.** ⚠️ **THE HOLDOUT'S OWN PRINTED FLOOR IS 9/15 = 60.0 %, NOT DEV'S 48 %** (WARN-9 honoured) ⇒ **the result is only 6.7 pp — ONE ROW — above a degenerate model with zero translation ability; on this split the headline carries very little discriminating power, and that cuts AGAINST the result.** **STRICT and LENIENT are IDENTICAL, so no part of the score is a leniency artefact**, and **`parse_failures=0` ⇒ the grammar is NOT the problem; the model's comprehension is.** **5 holdout rows assert ≤1 field** (HOLD-05/07/11/13/14). Dev, context only and NOT the bar: **18/25 = 72.0 %**, floor 48.0 %, 6 thin rows, DEV-08 asserts nothing. 🔎 **THE 5 FAILURES ARE A COHERENT STORY, NOT NOISE — 4 mechanisms:** ⛔ **(1) REFUSAL IS SAFETY-SHAPED AND 2 OF 3 REFUSE ROWS BECAME EXECUTABLE ORDERS**: HOLD-07 “send the **trebuchets**” ⇒ invented `send sapper×1 → enemy_castle` **for a unit type that does not exist**; HOLD-13 “**spend my gold** on another ogre” ⇒ `send miner×1 → nearest_mine`, **violating Jonathan's DECIDED ruling (iii) that the AI never spends gold.** **The grammar CANNOT catch either by construction — it admits all 13 kinds and all 7 intents regardless of board state or policy, so an out-of-scope sentence always has a legal-looking production. `{"ask":"unsupported"}` EXISTS and the model simply does not route to it** (HOLD-14 proves it can). **A silently-executed refused order is worse than no assistant.** **(2) unit selection silently dropped** (HOLD-10 “attack with the archers” ⇒ `"who":"none"`) — ⚠️ **this is EXACTLY the pathology part 2 §13d flagged as a single anecdote and refused to extrapolate; it has now REPRODUCED on the holdout.** **(3) synonym gap** (HOLD-01 “**bowmen**” ⇒ `longbowman`, expected `archer` — a `DA_AssistantVocabulary` data fix, not code). **(4) determiner→quantity** (HOLD-08 “**the** wizard” ⇒ `n:"all"`). **3 of 5 are few-shot/vocabulary work — the first two rungs of the plan's own ladder — and NONE argues for a bigger model yet.** ⚠️ **THE SEAL IS SPENT AND THESE 5 ROWS ARE BURNED AS TUNING SIGNAL — the runner itself says “DO NOT TUNE ANYTHING AFTER THIS POINT.” A FRESH HOLDOUT MUST BE AUTHORED BEFORE ANY TUNING IS SCORED; re-scoring this file after tuning would be measuring the training set.** **#6 input — ✅ PASS** (part 2, sampler-independent, not re-run): ships on `FInputModeGameAndUI`. ✅ **TRUNCATION CHECKED ON EVERY EVAL ROW, NOT JUST THE FIRST** (the 6-char margin): **ZERO** `Snapshot roster degraded`/`TRUNCATED` lines across all 40 eval rows and all 12 part-3 logs. `SPIKE_TOKENS` confirms **`MaxSnapshotChars=1085 headroom=130`** (the corrected value, **not** a regression), `zoneC_chars=887` against an 893 budget, `zoneBC_tok~=352` inside the 400 cap. **The prompt was NOT truncated at any point, so bar #5's number stands as a measurement.** ⚠️ **BAR #5 IS CONDITIONAL ON TASK-416's `MaxRosterKinds` SEAM** (WARN-5): the harness scored a 13-kind roster the shipped snapshot truncates to 8 — no truncation fired here because the fixture fits, **so 66.7 % is the number for a board that FITS the cap; shipped accuracy on a >8-kind board will be WORSE, never better.** ➡️ **WHAT JONATHAN MUST DECIDE: (1) RULE BAR #2's WORDING** — TTFT or wall-clock; it turns a PASS into a FAIL and is not build-master's call. **(2) DECIDE THE CPU FALLBACK'S FATE** — the only unambiguous bar-2 fact, and it is bad. **(3) BAR #5 IS A FAIL ⇒ the plan's own ladder applies (better few-shots → tighter grammar → bigger model → only then fine-tune); the refusal failures likely need a POLICY/REFUSAL ROUTE the grammar does not currently have.** **(4) AUTHOR A FRESH HOLDOUT before any tuning is scored.** ⚠️ **I did NOT convert four passes and one ambiguity into a GO. Bar #5 is measured, it is a FAIL, and the number is 66.7 %.** **NO COMMIT, NO PUSH, `L_Arena` NEVER SAVED** (`DIRTY_COUNT: 0` verified before the editor close and again after PIE ended; every close in-engine `quit_editor()`, never a force-kill; `L_Arena.umap` mtime still Jul 29), **HEAD unmoved at `ffe3fdd`**, tree as found. Editor left running on `L_Arena`, PIE ended. ⚠️ **MACHINE RESIDUE UNCHANGED (both outside the repo, both backed up): the USER-GLOBAL `EditorSettings.ini` throttle line (SURVIVED this shutdown; left in place on purpose) and the gitignored `Engine.ini` python-remote-exec block — which was DROPPED AGAIN by the editor's shutdown rewrite and re-appended. That drop is now a CONFIRMED STANDING BEHAVIOUR, not a one-off: re-append it after every editor close.** ⬇️ **PART 2 RECORD RETAINED BELOW —** ✅ **STEP 0 COMPILE GREEN** — `Result: Succeeded`, judged on the log line never the exit code, **zero diagnostics** under warnings-as-errors, plugin **and** game module rebuilt. ⛔ **THE ONE BLOCKER: THE GBNF HAS NEVER PARSED — IN THE SPIKE *OR* IN THE LANDED SHIPPED GENERATOR.** `llama_sampler_init_grammar returned NULL` on **every generation of all six bench runs**. Root cause obtained from llama.cpp's OWN parser (`llama-completion.exe`, same b10235 build), not by proof-reading: `parse: error parsing grammar: expecting ::= at _least ::=` — **`at_least` is not a legal GBNF rule name** (llama.cpp rule names are letters/digits/HYPHENS; its own grammars are kebab-case). **`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp:312` (rule) + `:392` (reference) have the SAME defect — so CONVENTIONS §9c's named AUTHORITY emits a grammar llama.cpp cannot load.** QA verified the two generators mirror each other rule-for-rule and they do — **what no by-derivation review could catch is that they are identically UNPARSEABLE; a dump is not a parse.** ✅ **FIX VERIFIED, NOT PROPOSED: renaming ONLY the rule to `at-least` (JSON key literal `"at_least"` UNTOUCHED — it is the wire format the corpus asserts) parses clean.** ⚠️ **HYBRID-REASONING RISK: REAL, AND FULLY NEUTRALISED BY A WORKING GRAMMAR — measured A/B, same model/prompt/temp0:** unconstrained ⇒ `<think>` + reasoning prose **every time** (all 5 warm iters on cpu printed `output=<think>` and burned the whole budget to the 10 s abort, zero JSON, every tier); constrained ⇒ `{"intent":"fallback","who":"none","where":"mid","when":"now"}`. **The GBNF DOES structurally forbid `<think>` from token 0 (`root` → both branches open with a `{` literal) — verified, not assumed.** ⇒ **bars #2 and #5 measure a model free-associating and are NOT reported against their bars.** 📊 **THE SIX BARS** (PIE on `L_Arena`, ~40 units BOTH teams via the shipping `SpawnUnitSwarm` path, `gpu=0` pinned to the RTX 5070, FPS verified ≥58 before each run, each tier run TWICE and the SECOND reported): **#1 hitch — PASS on p99 / NOT CLEANLY SCOREABLE on worst-frame**: partial-tier ATTRIBUTABLE p99 **+2.02 ms** prefill / **+2.32 ms** decode, median never moves (16.6-16.8 ms); worst-delta +24.68/+33.04 ms and **2 frames >33 ms of 5,297** — but the **full tier's own BASELINE hit `WORST=42.98ms` with the model IDLE**, so 2 outliers cannot be attributed to inference; **the measurement is CONSERVATIVE** (broken grammar ⇒ ~93 out-tokens/iter vs the short JSON that ships). **#2 latency — ⛔ NOT MEASURED** (no run was constrained); the TTFT half, which is grammar-independent, would pass: partial **1744 ms** (≤2000), cpu **2636 ms** (≤6000), full **417 ms**. **#3 KV reuse — ✅ PASS: DROP = 77.1 %** identical on all 3 tiers and all 6 runs vs a ~70 % bar; **`reused − zoneB_start~ = 1156 − 1147 = 9 tokens`, EXACTLY inside QA's predicted 8-10 band — the fixtures have NOT drifted**; `landed_in=ZONE_B` every time, agreeing with the independently-printed token boundaries. **§8's `165/765 = 78 %` reference is now CORROBORATED BY MEASUREMENT (within 0.9 pp) and is no longer UNVERIFIED.** `BEST_CASE` not quoted. **#4 VRAM — ✅ PASS: model-attributable `vram_delta = 2702.8 MiB` (2.64 GiB) at FULL offload on a 7891 MiB card = 34 %, ~5.1 GB left for the game** (cpu 366.7 / partial-18 793.5 MiB). ⚠️ **`vram_free_lowwater=607.0 MiB` MUST NOT be quoted as bar #4 — it includes the EDITOR rendering PIE on the same card; a standalone `-game` run is the one thing PIE cannot answer.** RSS peak 7818 MiB (irrelevant at 64 GB). **#5 accuracy — ⛔ NOT MEASURED. 🔒 THE HOLDOUT WAS NOT OPENED AND THE ONE-SHOT IS STILL UNSPENT** — I deliberately declined to run it: **spending a one-shot sealed holdout to score a broken sampler would destroy the only clean shot at bar #5 for a number that means nothing.** **#6 input — ✅ PASS, de-provisionalised** (`qa/TASK-411.md` qa-passed): **`BANKED row 1/3 CONTROL-OK path=523.25cm wasd=YES`** (control passed attempt 1 of 4 — the thing part 1 could never get), **row 2/3 MODE A PASS path=0.00cm focus=212/212**, **row 3/3 MODE B PASS focus=208/208**. ⇒ **a focused `UEditableTextBox` DOES starve Enhanced Input of WASD, and MODE A PASS ⇒ THE CONSOLE SHIPS ON `FInputModeGameAndUI`; the UIOnly fallback is NOT needed.** RMB (cursor parked over the viewport DPI-aware, quoted per QA): **MODE A YES @ (1198,1043) vs MODE B NO @ (1154,1105) — RMB is the real cost of UIOnly and it is not incurred**; a first run whose cursor landed at (2739,1097), OUTSIDE the window, had its RMB rows DISCARDED. **Keyboard untouched all run** (all keystrokes synthetic, headless). **Enter NOT reported as an input-mode cost** per QA's engine-source ruling. ⚠️ Escape NOT INJECTED in PIE ⇒ the Escape matrix still needs standalone `-game`. 🔢 **§8 TOKEN TRIGGER HAS FIRED — THE GUESS IS OVER.** `SpikePrompt` with the model resident: **zoneA_tok~=1139 zoneB_tok~=32 zoneC_tok~=320 zoneBC_tok~=352 assembled_total_tok=1504 chars_per_token=3.56**; boundaries `zoneB_start_tok~=1147 zoneC_start_tok~=1179` (t1 identical). **WORST (smallest) chars/token = Zone B at 2.13; the region the cap actually governs (B+C) = 2.71; even the whole-prompt average is 3.56 — §8's 3.6 is OPTIMISTIC ON EVERY READING, not conservative.** ⇒ **`MaxSnapshotChars = 1440` OVER-ADMITS: at 2.71 it admits 531 tokens against a 400-token budget, 33 % over. To bind at 400 it must drop to ~1085 chars** (~850 on the strict worst-zone reading). Today's board does NOT breach it (955 chars = 352 tok) — **the breach is LATENT and appears as soon as the roster or order line grows.** Also settled: **Zone A is 1139 tok, not §8's “~600 as-built”**, and **the assembled prompt is 1504 of 2048 ctx (1600 with the 96-token output budget = 78 %; ~87 % if the snapshot ever reached the current cap) — the context headroom is thinner than §8 implies and TASK-423's spec should say so.** ⚠️ **I RETRACT MY OWN PART-1 FINDING: the “~3 FPS for the first ~25 s startup window” DOES NOT EXIST.** It is the editor's **“Use Less CPU when in Background”** idle (`EditorEngine.cpp:5327-5375`, `bThrottleCPUWhenNotForeground`, default true): **a headless-driven PIE session is ALWAYS background, so it idles at EXACTLY 3.0 FPS FOREVER** — measured held for 11 straight minutes across two editor sessions; a `SetForegroundWindow` grab was REFUSED and changed nothing. **A bar #1 taken under it is not conservative, it is MEANINGLESS — the ~333 ms baseline floor would have made inference look free.** The setting is `config=EditorSettings` ⇒ the **USER-GLOBAL** `%LOCALAPPDATA%/UnrealEngine/5.8/Saved/Config/WindowsEditor/EditorSettings.ini`, **NOT** the project's `Saved/Config` (my first edit went to the wrong file, had no effect, and was reverted). With it off **PIE runs at 59.9 FPS** and the real startup ramp is **~3 s**, not 25. ⇒ **part 1's third re-run condition is WITHDRAWN and replaced by: disable the throttle AND verify measured FPS ≥58 immediately before each bench.** QA's run-twice rule was HONOURED but is **necessary and never sufficient** — under the throttle both runs are equally worthless. ⚠️ **DEVICE ORDER IS THE OPPOSITE OF TASK-409's LINK PROOF:** live inventory is `dev=0/Vulkan0/GPU = “NVIDIA GeForce RTX 5070 Laptop GPU” (7891 MiB)` and `dev=1/Vulkan1/iGPU = Intel Arc 140T`. QA note 6 predicted the reverse. **Do not hard-code either index — read it off the live `SPIKE_MEM stage=devices_before_load` line and verify the `desc=` string.** `Vulkan1` also reports free (47690 MiB) > total (37020 MiB), incoherent — the iGPU must never be a VRAM subject. **This machine's dGPU IS the modal 8 GB card bar #4 asks about.** ⚠️ **WARN-R2 SPRANG EXACTLY AS QA PREDICTED AND I CHECKED BEFORE BELIEVING IT:** cpu run 1 printed `bound=BEST_CASE reused=0 DROP=0.9%` + **“THE PROMPT LAYOUT IS WRONG AND MUST BE FIXED BEFORE ANYTHING ELSE IS BUILT”** — the line immediately above was `decode llama_decode returned 2 at token 0`. **It is a decode abort, NOT a layout defect** (the same run's SHIPPED bound was healthy at 77.1 %, and partial/full `BEST_CASE` ran normally at 98.7 %). **The prompt layout is fine; do not let that line be quoted.** ✅ **NORTON DID NOT AFFECT THIS RUN** — `scratchpad/vulkan/llama-cli.exe` was quarantined (`IDP.Generic`) but is **not ours and nothing needs it**; `llama-completion.exe` from the same release served identically. Verified after the fact: **all 19 vendored DLLs present, model intact at 2,497,280,256 bytes, zero DLL-load/access-denied errors in any of the six bench logs.** ⚠️ **The GBNF failure is NOT a Norton symptom** — deterministic, 6/6 in-engine + offline on two binaries, names a grammar token not a file, and a one-character rename fixes it. **It must not be laundered into “antivirus interfered”.** ➡️ **NEXT, and it is short: (1) `gameplay-programmer` renames the GBNF rule `at_least` → `at-least` in BOTH `SiegeAssistantGrammar.cpp:312`/`:392` (the SHIPPED authority — the more important) and the mirrored spike sites, leaving the JSON key literal alone, plus a sweep for any other underscored rule name. (2) Re-run bars #2 and #5 ONLY — #1/#3/#4/#6 do not depend on the sampler. (3) CONVENTIONS §8: lower `MaxSnapshotChars` 1440 → ~1085 and replace every `DERIVED — PENDING TASK-413` figure with the measured table.** ⚠️ **MACHINE RESIDUE, BOTH OUTSIDE THE REPO, BOTH BACKED UP: (1) the USER-GLOBAL `EditorSettings.ini` throttle line — LEFT IN PLACE ON PURPOSE because the re-run is worthless without it; it affects EVERY project Jonathan opens (no background idling ⇒ more CPU/battery on a laptop); restore from `scratchpad/EditorSettings.TASK413.bak`. (2) the gitignored `Saved/Config/WindowsEditor/Engine.ini` python-remote-exec block — the editor REWRITES this file on every shutdown and silently drops it, which is why part 1's “left in place” did not survive; it must be re-appended after each close.** **NO COMMIT, NO PUSH, `L_Arena` NEVER SAVED (dirty verified 0 before every close, all closes in-engine `quit_editor()`, never a force-kill), HEAD unmoved at `ffe3fdd`, tree as found.** Editor left running on `L_Arena`, PIE ended.
- blocked-by: **TASK-412 PASS**
- parallel-safe: no (EXCLUSIVE editor + compile)
- spec: >
    **THIS IS THE GO/NO-GO. Nothing in Wave 1 is written until this reports.**
    **(0)** Compile GREEN. Fetch the model with `Tools/fetch_llm_model.py`. **DEFAULT MODEL: `Qwen3-4B-Q4_K_M.gguf` from `Qwen/Qwen3-4B-GGUF` (Apache 2.0, ungated, sha256 verified, ON DISK) — CORRECTED per the ruling above; the former `Qwen3.5-4B-Instruct` NAMES A FILE THAT CANNOT EXIST** from the cleared
    permissive list. ⚠️ **VERIFY THE EXACT QUANT'S MODEL CARD, NOT THE FAMILY** — a quantizer's re-upload is a *different* licence card.
    Record **repo id + exact filename + quant + the licence line quoted verbatim**, and fill the model section of `Docs/ThirdPartyNotices.md`.
    ⚠️ **xLAM-2 / Hammer 2.1 / Arch-Function are BANNED (non-commercial) and must never be selected** — they are exactly what a search suggests.
    **(1) ⚠️ RUN IN PIE ON `L_Arena` WITH UNITS ON THE FIELD — NEVER AN EMPTY MAP.** The entire risk is *contention*; an empty-map number is
    worse than no number because it looks like evidence. Use `SummonTestUnit` to field a realistic fleet on both sides first.
    **(2) MEASURE ALL SIX BARS** (table at the top of this batch section), each across **full-offload / partial / CPU-only** where applicable:
    frame-time hitch histogram (**worst + p99, never mean**) · TTFT + wall-clock for ~60 constrained tokens on a warm prefix · turn-1 vs
    turn-2 prefill tokens (KV reuse, target **~70 % drop**) · peak VRAM + RSS **with the game at its own peak** · the 40-sentence exact-match
    accuracy · and measurement **#6 from TASK-411's probe** (`Siege.Assistant.InputProbe`, both modes) run in the same session.
    **Also record the REAL token count of the spike's snapshot text** so `MaxSnapshotChars` can be corrected from measurement instead of guessed
    a second time.
    **(2b) ⚠️ BAR #5 IS SCORED ON THE HOLDOUT, AND YOU OPEN IT EXACTLY ONCE (ruling 14, CONVENTIONS §11).** `assistant_eval_holdout.csv` has
    been sealed since TASK-426 and TASK-410 was forbidden to read it. **Run dev and holdout, report BOTH, and state plainly that the number
    measured against the ≥ 85 % bar is the HOLDOUT number.** ⚠️ **Quoting the dev score as the bar is a QA FAIL** — the plan's own remediation
    ladder begins *"better few-shots"*, i.e. tuning against dev, so a dev score is a fitted number by construction.
    ⚠️ **DO NOT tune anything after opening the holdout.** If the holdout misses and dev passes, that gap **is the finding** — report it as
    GO-WITH-RESCOPE with the gap named, and any re-tune requires a **fresh** holdout, which is a new task, not an edit to this one.
    **(3) THE VERDICT IS THREE-WAY and you REPORT it, you do NOT act on it:** **GO** (all bars met) · **GO-WITH-RESCOPE** (name the missed bar
    and apply the plan's ladder — for accuracy: better few-shots → tighter grammar → bigger model → *only then* fine-tune) · **NO-GO**.
    ⚠️ **DO NOT ROLL INTO TASK-423 ON YOUR OWN VERDICT.** The measurement is yours, the reading is the manager's, **the decision is Jonathan's
    (TASK-415).**
    **(4) NO Git** (TASK-414 owns the commit) · **`L_Arena` NEVER saved** · `reset --hard` / `clean -fd` BANNED.
    Handoff `handoffs/TASK-413-buildmaster.md` — a numbers table with one row per bar per tier, the raw histograms, the per-row accuracy diff,
    the model card evidence, and the verdict with its reasoning. Post the headline numbers + verdict in 🔧 Build & Git **and** a one-liner in
    🚨 Blockers if the verdict is anything other than GO.
- names: >
    Model default `Qwen3-4B-Q4_K_M.gguf` → `<ProjectRoot>/Models/` · commands `Siege.Llama.SpikeBench` / `Siege.Llama.SpikeEval` /
    `Siege.Assistant.InputProbe` · `Docs/ThirdPartyNotices.md`. Law: CONVENTIONS §7 (licensing + the banned list), §8 (prompt zones, the
    ≤400-token cap), §10 (tunables). Plan: §"Do the spike".

#### TASK-414 — [LLM-1d] Integration commit — the spike lane (build-master)
- assignee: build-master
- status: ✅ **done — COMMITTED 2026-08-03 as `66c6854`** (`main`, **NOT PUSHED**, ahead **7**). Handoff `handoffs/TASK-414-buildmaster.md`. **ONE commit, not the planned A/B/C** — ⚠️ **the scope was RE-DERIVED from `git status`, not taken from this spec, because Jonathan's `e70f5ba` (2026-08-02 17:22) had already swept in commit A's plugin + vendored `LlamaCpp` (22 `.dll`/`.lib`, LFS pointers intact) AND commit B's two eval CSVs.** Remaining set = **24 paths**, all verified in-lane by inverse filter. ✅ **COMPILE `Result: Succeeded`, ZERO diagnostics** (the comment-only fix + the WARN-3 test predicate had never been compiled; UBT rebuilt exactly `SiegeAssistantGrammar.cpp` + `SiegeAssistantGrammarTest.cpp`, link reached). ✅ **AUTOMATION 12/12 PASS — `Siegebound.Assistant.Grammar.RuleNameCharset` RUNS AND PASSES**, which converts the new comment's "the test is what protects a Shipping build" from a documented intention into evidence (closes NIT-R1). ⛔ **`Models/Qwen3-4B-Q4_K_M.gguf` NOT committed** — `git check-ignore -v` ⇒ `.gitignore:132:/Models/`; zero `.gguf` and zero `Models/` paths in the commit. ⚠️ **`.uproject` REPAIRED: `e70f5ba` captured the `SiegeLlama` entry as `"Enabled": false` (a hold-out snapshot) — restored to `true`** (disk 859 B / sha256 `63058f3c…`, matching TASK-420's recorded good restore; the 805 B blob is just git's LF normalisation, HEAD stores 806 B the same way — **NOT the TASK-420 CRLF corruption**). **`L_Arena` never opened, never saved; no `Content/` path in the commit.** ⚠️ **Board + `qa/TASK-433.md` were written by the TASK-433 reviewer DURING this commit window and are deliberately LEFT UNCOMMITTED for TASK-434, which owns them.**
- blocked-by: **TASK-412 PASS** + **TASK-413** (commits the evidence **whatever the verdict** — a NO-GO is still a result worth keeping)
- parallel-safe: no (EXCLUSIVE Git)
- spec: >
    **⛔ (0) BLOCKING PRE-FLIGHT — THE `.gitattributes` LFS RULE LANDS BEFORE OR WITH THE FIRST `Plugins/SiegeLlama/` COMMIT. THIS IS NOT A
    FOLLOW-UP NOTE (CONVENTIONS "GIT HAZARD LAWS" (b); Jonathan ruled LFS).** `Plugins/` already holds **72 MB across 20 binaries with 0
    tracked files**, headlined by **`ggml-vulkan.dll` at 49.9 MB**. ⚠️ **Git history is append-only — blobs committed raw stay raw FOREVER**,
    and undoing it means a history rewrite on a repo Jonathan pushes. Add the `*.dll` / `*.lib` LFS rules, then **PROVE they bind BEFORE
    committing**: paste `git check-attr filter -- <one vendored dll>` and `git lfs status`. **A `.gitattributes` that exists but does not match
    the path is the same defect as no rule at all.** If LFS is unavailable on this machine, **STOP and escalate — do not commit the binaries raw.**
    ⚠️ **(0a) VERIFY ANY HOLD-OUT RESTORE BY CHECKSUM, NEVER BY `git diff`** (same law, (a)). A `.uproject` toggle round-tripped through Python
    came back **805 B instead of 859 B** — CRLF→LF on all 54 line endings — and **`git diff` showed it perfectly clean, because `autocrlf`
    normalises exactly that away.** Record sha/size before, re-compare after.
    Commit **on `main`, NO PUSH** (Jonathan's push, standing law). **PER-DELIVERABLE COMMITS** — the shipped house pattern; it is what lets
    Jonathan revert one lane without losing the others:
    **commit A** = the plugin + vendored `LlamaCpp` + `.uplugin` + both `Build.cs` + `.uproject` entry + `.gitignore` negations +
    `Docs/ThirdPartyNotices.md` + `Tools/fetch_llm_model.py`;
    **commit B** = the spike harness + `SiegeAssistantInputProbe.{h,cpp}` + **both corpus CSVs** (`Docs/Data/assistant_eval_dev.csv` +
    `Docs/Data/assistant_eval_holdout.csv` — ⚠️ **the holdout is committed as authored and must never be edited afterwards**; a re-tune needs
    a FRESH holdout in a new task, per ruling 14);
    **commit C** = pipeline docs — `qa/TASK-412.md` + `handoffs/` + board + CONVENTIONS (docs-only commits are always permitted).
    ⚠️ **VERIFY THE VENDORED BINARIES ACTUALLY LANDED**: after staging, `git ls-files Plugins/SiegeLlama/Source/ThirdParty` must list the
    `.dll`/`.lib` files. **A silent `.gitignore` swallow is the specific failure this step exists to catch** — an empty result is a STOP.
    ⚠️ `git ls-files` must show **no `.gguf` anywhere** and nothing under `/Models/`.
    ⚠️ **The editor's Git provider auto-stages saved assets — stage by EXPLICIT PATHSPEC and re-check `git status --porcelain` after staging**
    (the TASK-378 hazard). If anything foreign is dirty or staged, **STOP and report rather than committing it.**
    `git diff --stat` clean on each · **`L_Arena` NEVER saved** · `reset --hard` / `clean -fd` BANNED · **real hashes on the board and in the
    handoff, not placeholders** (the TASK-355/357 lesson) · **verify the ahead-count before asserting one** — do not trust a memory note.
    Report `handoffs/TASK-414-buildmaster.md`. Post the hashes in 🔧 Build & Git.
- names: >
    Commits on `main`. Law: the hard gate (no code commit without a PASS QA report) + the per-deliverable commit pattern + Standing lesson 3
    (commit board + CONVENTIONS at every batch boundary).

#### TASK-420 — [LLM-INT1] Compile GREEN + run the grammar automation tests + machine PIE sanity of the game-lane Wave 0 (build-master)
- assignee: build-master
- status: ✅ **done — COMPILE GREEN + 11/11 TESTS PASS** (2026-08-02 15:37, `Result: Succeeded`, zero diagnostics of any kind). **BLOCKER-1 CONFIRMED CLOSED BY MACHINE** (headless CDO+`DT_Cards` probe, **NOT** PIE — desktop LOCKED). ⛔ **WARN-3 SETTLED AND IT IS LIVE — IT GATES TASK-422** (see below). Handoff: `handoffs/TASK-420-buildmaster.md`
- blocked-by: **TASK-419 PASS** — ✅ cleared
- parallel-safe: no (EXCLUSIVE compile + editor)
- result: >
    ## ✅ COMPILE GREEN (2026-08-02 15:37)
    ```
    Result: Succeeded
    Total execution time: 10.96 seconds
    ```
    Judged **on log text, not exit code**: `grep -inE "error|warning|unresolved|LNK|fatal|failed"` over the whole log returns
    **ZERO rows**. Link reached and succeeded (`.lib` + `.dll`). Binary **3,283,456 B @ 15:37:01**, postdates every lane source.
    **Nothing to attribute — zero diagnostics from any file, this lane's or any other's.**
    ✅ **NOT A NO-OP RE-RUN OF TASK-401's GREEN.** TASK-401 RUN #3 (15:02) **predates four lane edits** (`Vocabulary.cpp` 15:20:48,
    `GrammarTest.cpp` 15:21:16, `Snapshot.h` 15:21:51, `Vocabulary.h` 15:21:59) ⇒ **TASK-417's loop-2 fix had never seen a compiler
    until this gate.** UBT recompiled exactly those TUs.
    ✅ **MODULE VERIFIABLY QUIET:** 14 m 51 s since the last source write; no build/editor process running; 416/417/418 all
    `qa-passed` (finished, handoffs written); TASK-401 `done`; 403/422/425 `backlog`. **Source snapshot before vs after the gate:
    186 files, ZERO differences** ⇒ no foreign write, and I wrote no source file.
    ✅ **PLUGIN HELD OUT AND RESTORED.** `SiegeLlama` set **explicitly `"Enabled": false`** (NOT deleted — the `.uplugin`'s
    `"EnabledByDefault": true` at `:17` means deleting the entry would leave it ENABLED). **Zero `siegellama|llama` occurrences in
    the build log** ⇒ fully excluded. Restored byte-identically: sha `63058F3C…`, **859 B**, `"Enabled": true`.
    ⚠️ **NEW HAZARD CAUGHT — VERIFY A HOLD-OUT RESTORE BY CHECKSUM, NEVER BY `git diff`.** One toggle done via a Python
    round-trip returned **805 B / sha `ca5c2cf6…`** — universal-newline read silently rewrote all 54 line endings CRLF→LF.
    **`git diff` showed it as perfectly clean** (autocrlf normalises it away); only the checksum caught it. Restored from backup.
    ⚠️ **`Plugins/SiegeLlama/` has NO `Binaries/` — it has never been built**, so leaving it enabled fails the *editor launch*, not
    just the build. It stays **UNPROVEN through UBT/UHT** and keeps its **TASK-412** gate.
    ## ✅ AUTOMATION TESTS — 11 FOUND, 11 RUN, **11 PASS**, 0 FAIL, 0 SKIPPED
    `Found 11 automation tests based on 'Siegebound.Assistant'` → `...Automation Test Queue Empty 11 tests performed.`
    All Success: `Command.NeverPartiallyFills` · `Command.Rejection` · `Command.RoundTrip` · `Command.SelectionInvariants` ·
    `Grammar.CountRange` · `Grammar.DegenerateInputs` · `Grammar.Determinism` · `Grammar.Grounding` · `Grammar.Intents` ·
    `Grammar.SelectionCap` · `Vocabulary.SynonymTable`. Exactly QA's 11 names — none added, removed, renamed or weakened.
    **QA's four load-bearing tests all pass**, including **both new guards inside `Vocabulary.SynonymTable`** (guard A `:955-956`
    no `_` in a unit canonical; guard B `:968-969` substring `mage`/`caster`/`spellcaster`, old whole-string check still alongside at `:975-976`).
    ⚠️ **A first test attempt crashed and it was MY HARNESS FLAG, not the lane** — a non-standard `-noshadercompile` produced
    `Fatal: Null assigned to TNotNull` **after** `FEngineLoop::Init()`, with a callstack of **100% engine DLLs and ZERO frames in
    `UnrealEditor-GitClaudeUnrealTest.dll`**. Dropping the flag ran clean first time. **Not a finding against anyone.**
    ## ⚠️ BLOCKER-1 (`militiamob`) — CONFIRMED CLOSED BY MACHINE, BUT **HEADLESS, NOT LIVE**
    ❌ **NO PIE. I did NOT field a MilitiaMob and I claim no §(3) item.** Desktop **LOCKED** (`LogonUI` PID 30612).
    ⚠️ **THE SPEC'S SUBSTITUTE IS NARROWER THAN ASSUMED — RECORD THIS:** `CanonicalKind` is a **private static in the `.cpp`** and
    `GetUnitKinds()` a **plain inline getter**; `USiegeAssistantSnapshot` has **NO reflected functions at all**, so a commandlet
    probe **cannot call either one.** What IS reflected is `USiegeAssistantVocabulary`'s three `UPROPERTY` tables.
    ✅ **What I ran:** headless `-run=pythonscript` reading the **shipped CDO** (the object test 11 reads) against **`DT_Cards`**:
    all **12 `CardType==Unit` CardIDs round-trip** (`MilitiaMob -> militiamob` **YES**); `MISSING: NONE`; the only extra canonical is
    `miner` ← DT_Cards row `Miner`/`Economy` (QA-verified correct); **`militia_mob` absent**; **zero `_` in any unit canonical**;
    **zero aliases embedding `mage`/`caster`/`spellcaster`**. This is the **`DT_Cards` set-membership check QA said "would close the
    class outright"** but rightly kept out of the asset-free suite — **now run, but EXTERNAL to the suite, so it is not a regression guard.**
    ❌ **What it does NOT prove:** it **derives** the symbol by applying `CanonicalKind`'s documented rule **in Python — it does not
    execute the shipped function**, and **no roster line was printed by the real serializer.** The live "field a MilitiaMob, watch
    Zone C print `militiamob`" confirmation is **STILL OWED** to the first task that reaches PIE.
    ❌ **SPEC §(3)/§(4) NOT PERFORMED, ZERO ITEMS CLAIMED:** no snapshot capture/paste/char-count, no roster-aggregation check, no
    `ancient_ground_near ≠ ancient_ground_far` 180° twin check, no coordinate/timestamp/prose sweep, no `BuildZoneA`-twice byte-diff,
    no regression floor. All need a live world **and** a reflected entry point; both absent. Carry to the first live session.
    ## ⛔ WARN-3 — SETTLED, AND IT IS **LIVE**. IT GATES TASK-422.
    `git show HEAD:GitClaudeUnrealTest/Source/.../Castle.h | grep FindNearestCastleForTeam` → **no output**; `Castle.cpp` → **0**.
    ⇒ **`ACastle::FindNearestCastleForTeam` is ABSENT FROM `HEAD`.** It exists only in the **uncommitted working tree**
    (declared `Castle.h:190`, defined `Castle.cpp:655`), and `SiegeAssistantSnapshot.cpp:234-235` calls it **twice**.
    > ⛔ **TASK-422 commit A MUST NOT land before — or without — the FOLLOW lane's `Castle.{h,cpp}` commit (TASK-403), or `main`
    > is left NON-COMPILING** even though every working tree is green. **Invisible from either board in isolation.**
    Orchestrator's options: **(a) run TASK-403 first** (clean), or **(b)** TASK-422 commit A additionally includes `Castle.{h,cpp}`
    — which crosses lane ownership and **needs a ruling**. **I did not choose. Nothing is staged.**
    ⚠️ **PATH TRAP FOR ANYONE RE-RUNNING IT:** repo root is **one level above** the project (`--show-prefix` = `GitClaudeUnrealTest/`).
    QA's literal `git show HEAD:Source/...` **fails with `fatal:`, exits non-zero and prints nothing — which reads exactly like
    "absent".** I nearly filed the right answer for the wrong reason. Correct form: `HEAD:GitClaudeUnrealTest/Source/...`.
    ## ✅ OTHER
    **QA note 4 / criterion (9) — `AncientGround` additive-only PASSES mechanically:** `git diff --numstat` = **44/0** (`.cpp`) and
    **28/0** (`.h`) — **zero deletions ⇒ zero modified lines.**
    **`L_Arena` NEVER opened, NEVER saved — 535,522 B / 7/29 03:53:38**, verified at gate start, post-build, and after both editor runs.
    **NO Git, no push, nothing staged**; final `git status --porcelain` **identical to the session-start snapshot**, no `Content/`
    asset dirtied by either headless run. `reset --hard` / `clean -fd` never invoked.
    ⚠️ **CARRY-FORWARD — `.gitattributes` LFS TRAP IS LIVE, WITH NUMBERS.** Repo-root `.gitattributes` (305 B) covers
    `uasset/umap/fbx/png/jpg/wav/mp4` and **has NO `*.dll` / `*.lib` rule**. `Plugins/` = **72 MB, `git ls-files` = 0 tracked** ⇒
    **the trap has NOT sprung yet.** What it must catch: **20 `.dll`/`.lib`**, headlined by **`ggml-vulkan.dll` 49.9 MB** + `llama.dll`
    2.7 MB + 15 `ggml-cpu-*.dll`. ⛔ **The pattern must be committed BEFORE the first commit that adds `Plugins/SiegeLlama/`** or those
    blobs stay **raw in history permanently** — a later edit is not retroactive. **TASK-414 springs it; TASK-422 must not add `Plugins/` either.**
    ➡️ **TASK-421 UNBLOCKED** (`USiegeAssistantVocabulary` compiled and in-editor — the probe instantiated its CDO by name).
    ⚠️ Dispatch it with **WARN-6 + WARN-7 as explicit acceptance criteria**: my guard replication covers the **C++ CDO defaults ONLY**;
    `DA_AssistantVocabulary` does not exist yet and **no test in the suite will ever see it.**
- spec: >
    **(0) ⛔ QUIESCE THE MODULE FIRST (CONVENTIONS "THE QUIET-MODULE LAW", 2026-08-02).** **Confirm no programmer task in
    `Source/GitClaudeUnrealTest/` is in flight and no other compile gate is running** — TASK-401 / TASK-403 (FOLLOW) and TASK-422 / TASK-425
    are **mutually exclusive** with this one. **A compile gate needs a QUIET MODULE, not merely non-overlapping files:** this exact contamination
    already failed TASK-401 on three foreign diagnostics. If the module is not quiet, **STOP and report — do not build.**
    **(1) COMPILE GREEN** (game module + the `SiegeLlama` plugin together). ⚠️ If it fails for a **Smart App Control** reason
    (`0x800711C7`, ~2 s) that is **NOT a code error — do not loop QA**; escalate to Jonathan in 🚨 Blockers.
    ⚠️ **ATTRIBUTE EVERY DIAGNOSTIC TO THE FILE THAT NAMES IT.** Anything from outside this batch's owned files is routed to its owning batch,
    never recorded as this lane's finding, and the gate is **re-run once quiet, not re-litigated.**
    **(2) RUN THE AUTOMATION TESTS** (`Automation RunTests` / the session frontend, headless is fine) and paste the pass/fail list.
    **(3) MACHINE PIE SANITY on `L_Arena`** — no gameplay change is expected and none may appear:
    · field a fleet with `SummonTestUnit`, capture a snapshot, and paste it **in full** with its character count (must be ≤ `MaxSnapshotChars`);
    · confirm the roster is **aggregated by CardID and group, never per-unit**, and that eligibility matches the shipped predicates;
    · confirm `ancient_ground_near` and `ancient_ground_far` resolve to **two DIFFERENT actors** and that each is the nearer one to its own
      castle (the 180° twin check — this is a free assertion, take it);
    · confirm **no coordinate, no timestamp and no prose** appears anywhere in the captured text;
    · call `BuildZoneA` twice and diff the bytes — **must be identical.**
    **(4) REGRESSION FLOOR:** the match runs exactly as before — every keyboard command works, nothing new spawns, nothing auto-enables.
    **NO Git** (TASK-422 owns the commit) · **`L_Arena` NEVER saved** · `reset --hard` / `clean -fd` BANNED.
    Handoff `handoffs/TASK-420-buildmaster.md`. Post in 🔧 Build & Git.
- names: >
    Build command per CLAUDE.md. Tests `SiegeAssistantGrammarTest.cpp`. Law: CONVENTIONS "In-match LLM command assistant" §3, §4, §8.

#### TASK-423 — [LLM-A2] `USiegeLlamaSubsystem` — worker thread, queue depth 1, abort, timeouts, offload tiering, KV reuse (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: **qa-passed** (2026-08-03 — `qa/TASK-424.md`: **PASS, 0 blockers**. Flip applied by the ORCHESTRATOR; qa-reviewer has no partial-edit tool. ⚠️ **STILL THE ZONE-A TWO-LANE GATE ONLY** — the runnable `USiegeLlamaSubsystem` was carved out to **TASK-450** (also `qa-passed`) after an ORCHESTRATION scoping error, not a programmer shortfall; the declared shortfall was recorded as a credit. ✅ QA diffed the 98-line frozen spike copy line-by-line against `SiegeLlamaSpike.cpp:220-354` — **genuinely verbatim, in order**. ✅ §13 clean: 5 `TestEqualSensitive` + 1 `TestNotEqualSensitive`, and all 7 `TestEqual` sites are `int32`. ⛔ **WARN-5 REMAINS INSTRUMENTED, NOT DISCHARGED** — no test in this repo has ever executed; discharge is TASK-447's compile **plus a green run, claimed against that run.**)
- ⛔ **DO NOT READ THIS `ready-for-qa` AS "TASK-423 IS DONE" (gameplay-programmer, 2026-08-03).** Delivered: ONE new file,
    `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantZoneATest.cpp` — five `Siegebound.Assistant.ZoneA.*` automation tests
    asserting the shipped `BuildZoneA(default vocab)` byte-identical to a frozen verbatim copy of the spike's `AppendZoneA`, both at
    exactly **5116 chars AND 5116 UTF-8 bytes**, both ASCII-clean, plus the static-prefix contract. **⛔ NO TOKEN CONSTANT ANYWHERE
    (§12g).** NOT delivered: the subsystem, the runnable, queue depth 1, abort, the three timeouts, tiering/KV reuse, and spec item (9)'s
    budget work — **these need their own dispatch.**
- ⛔ **WARN-5 IS *NOT* DISCHARGED — IT IS INSTRUMENTED.** The test **has never run** (no compile this task; TASK-447 owns the one gate).
    **Discharge happens when TASK-447's compile lands AND `Siegebound.Assistant.ZoneA.*` runs GREEN**, and must be claimed against that
    run, never against the handoff. Still out of reach even when green: the **`DA_AssistantVocabulary` ASSET lane is untested** (the asset
    overrides the C++ defaults wholesale), and tokens remain a runtime reading.
- ⚠️ **AMENDMENT A — THE SPIKE DELETION IS DEFERRED, DECLARED HERE AS THE AMENDMENT REQUIRES.** `SiegeLlamaSpike.cpp` + `SpikeCorpus.inl`
    are **NOT deleted**: it is the only command printing `zoneA_tok`, it is what makes the new fixture auditable by text-diff, and — decisive —
    **its replacement does not exist yet, so deleting it now leaves ZERO model-load paths, not one.** The deletion belongs in the same commit
    as the subsystem.
- handoff: `handoffs/TASK-423-programmer.md`
- blocked-by: ✅ **NONE — UN-GATED 2026-08-03 AND DISPATCHABLE NOW (SETTINGS+CONFIRM ruling 1).** Jonathan was asked explicitly and chose *"un-gate Wave 1, build it all"* — **a DELIBERATE OVERRIDE of his own bar-#5 gate, not a bypass.** ⛔ **Bar #5 itself never cleared (20/25 vs 22 + zero refuse-class failures) and that stays on the record.** The original gate is preserved for history: ~~**TASK-415 = GO or GO-WITH-RESCOPE** (⛔ manager ruling 3: this task's entire shape is determined by the spike's numbers — writing it before the verdict is writing it twice)~~ — ✅ **the numbers now EXIST (TASK-413 measured all six bars), so ruling 3's condition is satisfied on its own terms: you are writing this once, from measurements, exactly as intended.**
- ⚠️ **AMENDMENTS FOLDED IN BY THE MANAGER 2026-08-03 — READ BEFORE TOUCHING THE SPEC BELOW, WHICH PREDATES THE LADDER WAVE:**
    **(A) ⛔ DO NOT DELETE THE MEASURING INSTRUMENT AND THEN QUOTE A NUMBER FROM MEMORY.** Spec step "FIRST ACT: DELETE `SiegeLlamaSpike.cpp`" is
    **still correct in intent** (two model-load paths must never coexist) but it now also deletes **the ONLY command that prints `zoneA_tok`**
    (`Siege.Llama.SpikePrompt` — §12c's hard gate reads it) **and one of the two lanes of the owed `BuildZoneA` equality test** (§12g,
    `handoffs/TASK-434-buildmaster.md` §6 item 4). ⇒ **Take the two-lane equality assertion and any final token reading FIRST, in this task,
    BEFORE the deletion — or defer the deletion and SAY SO in the handoff.** ⛔ **A silent deletion that strands both is a QA FAIL.**
    **(B) TWO SYMBOLS IN THE SPEC BELOW ARE STALE AND THE VENDORED HEADER WINS:** it is **`llama_memory_seq_rm`** (`llama.h:735`), **not**
    `llama_kv_cache_seq_rm` — that symbol **does not exist anywhere in the vendored header**; and it is **`load_mode = LLAMA_LOAD_MODE_MMAP`**,
    **not** `use_mmap = true` — that member no longer exists. **Rename only; both requirements are unchanged.**
    **(C) THE BUDGET ASSERTION LANDS ON CHARACTERS, THE CEILING STAYS A RUNTIME READING (§12g).** The startup assertion is on **CHARS/BYTES**
    (exact, countable offline, and this repo's prompt literals are verified ASCII-clean so char == byte); ⛔ **a DERIVED token constant must
    NEVER be baked into a test** — its error sign is unknown, and an automated guardrail that reports safe is worse than none.
    **`MaxSnapshotChars` is RETIRED at your commit, not re-pointed** (1085 is an *authority* value assuming the SMALLEST plausible ratio; a
    *pre-filter* must assume the LARGEST — the safe direction inverts with the role). Introduce **`MaxSnapshotTokens` = 400** and
    **`SnapshotPreFilterMaxChars` = 3000**. ⚠️ **And the observable-truncation condition is re-anchored onto THE ACT: any mechanism that truncates
    the snapshot, under any name, must LOG observably** — your new cut sites are not covered by the snapshot builder's existing log.
    **(D) §12h — THE REPEAT LAW:** any figure you quote from a single run carries its spread, and ⛔ **no "+1 row" claim may rest on one run.**
- parallel-safe: yes vs the game lane; **EXCLUSIVE owner of `Plugins/SiegeLlama/Source/SiegeLlama/{Public,Private}/SiegeLlamaSubsystem.{h,cpp}`**
- spec: >
    **Read CONVENTIONS §8 and the §9 pinned registry first — `USiegeLlamaSubsystem`'s four methods and the delegate are binding
    character-for-character**, because Wave 1's B2 compiles against them.
    **⚠️ FIRST ACT: DELETE `SiegeLlamaSpike.cpp` AND ITS CORPUS FILE.** Two model-load paths must never coexist (manager ruling 4). Carry the
    spike's *measured* settings forward into this file; carry none of its structure.
    **(1) `USiegeLlamaSubsystem : UGameInstanceSubsystem`** — it **survives level travel, so Play Again never reloads 2.5 GB** (the
    `USiegeSessionSubsystem` precedent). Model load is **async and never blocks match start**; a failure logs on `LogSiegeLlama`, latches
    faulted, and leaves `IsReady()` false forever after.
    **(2) ONE DEDICATED `FRunnable` AT `TPri_BelowNormal`.** `llama_context` is **not thread-safe**, and a long-lived runnable makes
    single-thread ownership **structural rather than a comment**. Do not use the task graph, do not use `Async()`, do not create a context per
    request.
    **(3) QUEUE DEPTH 1 — and it is enforced by the FSM, not by a queue.** `RequestCompletion` returns **false immediately** when
    `!IsReady()` or `IsBusy()`. **This deletes the out-of-order / stale-context bug class entirely** — do not add a request queue "for
    robustness"; that reintroduces exactly the class this design removes.
    **(4) CANCELLATION VIA `llama_context_params::abort_callback` — the ONLY correct mechanism.** Polling a flag after generation is far too
    late during a long prefill. `CancelActiveRequest()` routes through it.
    **(5) THREE TIMEOUTS:** soft **4 s** (UX only — keeps waiting, just tells the FSM to say something), hard **10 s** (abort), token budget
    **96**. Values are `EditDefaultsOnly`/named constants, **corrected from TASK-413's measurements**, all flagged for Jonathan's feel pass.
    **(6) OFFLOAD TIERING + KV REUSE, BOTH DRIVEN BY THE SPIKE'S NUMBERS:** pick full-offload / partial / CPU-only from measured VRAM
    headroom, defaulting to **partial** (the tier the frame-time bar is written against); **`load_mode = LLAMA_LOAD_MODE_MMAP`** (⚠️ **CORRECTED
    2026-08-03 — this clause read `use_mmap = true` and THAT MEMBER NO LONGER EXISTS; rename only, requirement unchanged**); small `n_ubatch`;
    **`llama_memory_seq_rm`** (⚠️ **CORRECTED 2026-08-03 — this clause read `llama_kv_cache_seq_rm`, which appears NOWHERE in the vendored
    header, `llama.h:735` is the real symbol**) prefix retention keyed on the **Zone A + Zone B** prefix so turn two reuses it
    (**~70 % prefill drop** is the bar).
    **(7) SAFETY:** SEH around the eval · **`bAssistantFaulted` session latch** · **the completion delegate ALWAYS fires on the GAME THREAD** ·
    a GGML crash must not kill the game · shutdown joins the worker cleanly and frees the model.
    **(8) THE PLUGIN STILL KNOWS NOTHING ABOUT SIEGEBOUND.** No `#include` of any game-lane header, no Siegebound type in any signature.
    The entire API surface between the two lanes is `(prompt, gbnf) → string`. **A Siegebound include in this file is a QA FAIL.**
    ⚠️ **AND NO MULTI-TURN LOOP** — this class exposes exactly one single-shot call and holds **no conversation state whatsoever** (§1).
    **(9) 📥 FOLDED IN BY THE MANAGER 2026-08-03 (from `qa/TASK-416.md` — both items were CORRECTLY deferred to you, not neglected):**
    ⚠️ **(9a) WARN-1 — THE DIMENSION THAT BITES FIRST, AND THE ROSTER-WIDTH ANALYSIS MISSED IT.** `MaxUtteranceChars = 240` applies to **BOTH**
    the `order:` line **AND** the `pending:` line, and **neither is ever truncated by the budget — only the roster is.** QA's bound: Zone C head
    ≈ 116 + tail ≈ 577 leaves a **~200-char roster budget ⇒ ~4 of 8 kinds print**, where the old 1440 cap did not truncate at all. It takes
    **both** a long typed sentence **and** a long pending line to bite, which is why it is a WARN — **and it is logged loudly with both causes
    named, which is the §8 ruling's condition doing its job.** Your token-based enforcement must account for **both** unbounded lines, not just
    roster width.
    ⚖️ **(9b) `ZoneBCharReserve` IS THE HONEST LEVER AND IT WAS CORRECTLY LEFT ALONE — IT IS YOURS.** It charges **192** for a Zone B that
    measures **68 (t0) / 71 (t1)**, worst case ~85. QA agrees it is the **only** adjustment that buys Zone C room **without raising the token
    admission ceiling** (`MaxSnapshotChars` caps B+C; the reserve merely partitions it). ⛔ **Raising `MaxRosterKinds` to buy that room is
    CONFIRMED CLOSED OFF** — at 13 kinds Zone C is 887 against an 893-char budget (six characters), so it converts a *deterministic policy*
    collapse into a *budget-driven* one firing on almost every real sentence. ⚠️ **Set the reserve FROM Zone B's printed worst case
    (`Siege.Llama.SpikePrompt` measures it) — do NOT eyeball 96 or 128.** The header says so at `:222-223`: *"re-measure it, do not eyeball it."*
    **NO game-lane edits, NO `Content/`, NO Git, NO compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-423-programmer.md`: the tiering policy with the numbers it came from, the abort path traced end to end, and the
    thread-ownership argument.
- names: >
    `Plugins/SiegeLlama/Source/SiegeLlama/{Public,Private}/SiegeLlamaSubsystem.{h,cpp}` · `USiegeLlamaSubsystem` ·
    `FSiegeLlamaCompletionSignature` · `IsReady` / `IsBusy` / `RequestCompletion` / `CancelActiveRequest` (**§9 registry,
    character-for-character**) · `LogSiegeLlama`. **DELETES** `SiegeLlamaSpike.cpp` + `SpikeCorpus.inl`.
    Law: CONVENTIONS "In-match LLM command assistant" §1, §5, §8, §9, §10.

#### TASK-424 — [LLM-QA3] QA gate on TASK-423 **+ TASK-450** — the subsystem gets its OWN gate (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-03 — report `qa/TASK-424.md`. **PASS / PASS, 0 blockers**, 8 warns, 7 nits. Slack posted in ⚙️ Dev & QA. Board flips applied by the ORCHESTRATOR, not by QA. 🔒 **The spike deferral was verified at FOUR artifacts and ruled CORRECT:** `assistant_eval_holdout2.csv` exists · its rows assert *"at t0"* by name · `t0` lives at `SiegeLlamaSpike.cpp:509-519` · `Siege.Llama.SpikeEval` is registered at `:4744-4751` **and exists nowhere else** ⇒ deleting that file would make the sealed corpus **unmeasurable**. CONVENTIONS **§16** already ruled the deletion **UNSCHEDULED**. ⚠️ **THIS GATE'S OWN ITEM (10) IS SUPERSEDED** by §16, as is TASK-447's inherited *"deletion in the same commit"* clause — **both owed a strike by manager.** ⚠️ `SpikeCorpus.inl` **confirmed never to have existed** — owed a strike from both `names:` lines. ⚠️ The game-lane `MaxSnapshotChars` work is **15 matching lines, not 10** — the gap is prose-vs-call-site, **§14 instance 2**.)
- blocked-by: TASK-423 **+ TASK-450** — ⚠️ **SCOPE WIDENED 2026-08-03**
- parallel-safe: no (single QA report)
- ⚠️ **WHY THE SCOPE MOVED, STATED PLAINLY: TASK-423 WAS DISPATCHED AGAINST ITS HEADLINE AND DELIVERED ONLY THE ZONE-A EQUALITY TEST.** The
  subsystem itself is **TASK-450**. ✅ **The programmer DECLARED the shortfall rather than letting a partial pass as `done` — that is the
  behaviour this pipeline wants, it is the only reason this was caught before the compile gate, and it is recorded as a credit, not a miss.**
  ⇒ **ONE report covers both:** the **landed test** (TASK-423) and the **subsystem** (TASK-450). The spec's items (1)–(11) below are the
  subsystem's criteria and apply to TASK-450. **Two additions:**
  **(12) ⚠️ THE ZONE-A EQUALITY TEST ASSERTS ON CHARACTERS, NEVER ON A DERIVED TOKEN CONSTANT** (§12g — a baked derived constant is an
  assertion whose error **sign is unknown**, and it looks authoritative, which is worse). **And check its comparator: a byte-identity claim made
  with `TestEqual` is VACUOUS** (CONVENTIONS "Settings screen…" §13 — this is what TASK-451 exists to sweep, and this file is in its scope).
  **(13) ⛔ THE SPIKE DELETION IS DEFERRED-AND-DECLARED, AND THAT WAS THE RIGHT CALL — verify it happened at TASK-450 and not before.** With no
  subsystem, deleting the spike leaves **ZERO** model-load paths rather than one. ⚠️ **Also confirm any final `zoneA_tok` reading was taken
  BEFORE the deletion — it is the only command that prints one.**
- spec: >
    **This is a separate gate on purpose: a threading + native-interop subsystem must never share a QA report with pure gameplay code.**
    **Report `qa/TASK-424.md`.** Beyond the standing laws:
    (1) **THREAD OWNERSHIP.** Exactly one `llama_context`, touched by exactly one thread. No task-graph work, no `Async()`, no per-request
    context. Trace every path that could touch the context off the worker — **any second toucher is a BLOCKER.**
    (2) **The completion delegate ALWAYS fires on the GAME THREAD**, on every path including failure, timeout and abort.
    (3) **Queue depth 1 is enforced by early-return, and NO request queue was added** (adding one reintroduces the stale-context class).
    (4) **Cancellation goes through `abort_callback`** — a post-generation poll is a BLOCKER, not a warning.
    (5) **Shutdown**: the worker is joined, the model freed, no dangling delegate into a destroyed UObject, and level travel does **not** unload
    the model (it is a `UGameInstanceSubsystem` for exactly that reason).
    (6) **SEH around the eval** + the `bAssistantFaulted` latch + **model-load failure never blocks match start.**
    (7) **The plugin includes NO game-lane header and names no Siegebound type** (§8's lane split) — a violation is a BLOCKER.
    (8) **NO multi-turn loop and NO conversation state held anywhere** (§1) — automatic FAIL.
    (9) **§9 signatures character-for-character**, access levels included.
    (10) **The spike harness is DELETED** (`SiegeLlamaSpike.cpp` + corpus) — two load paths coexisting is a BLOCKER (manager ruling 4).
    (11) **Standing coding laws** + **M8 DECLARATION DUTY stated verbatim.**
    Post the verdict in ⚙️ Dev & QA.
- names: >
    Report `qa/TASK-424.md`. Law: CONVENTIONS "In-match LLM command assistant" §1, §8, §9.

#### TASK-425 — [LLM-INT3] Compile GREEN + integration commit — the inference subsystem (build-master)
- assignee: build-master
- status: ⛔ **SUPERSEDED 2026-08-03 by TASK-447 — DO NOT DISPATCH.** ⚖️ **The reason is the QUIET-MODULE LAW, not convenience: two compile gates are MUTUALLY EXCLUSIVE**, and TASK-423's subsystem now lands in the same wave as seven game-module tasks that all need one. **Running both would mean quiescing the module twice to prove the same thing once.** ⇒ **TASK-447 absorbs this task's ENTIRE spec** — the compile, the machine smoke test (`Siege.Llama.Info`; async load never blocks match start; one parseable JSON from a hard-coded prompt+GBNF; **a second concurrent call returns false — queue depth 1**; `CancelActiveRequest()` aborts mid-prefill within the hard timeout; **a deliberately-absent model file leaves the match fully playable with one log line**; and the **partial-tier frame-time hitch re-measured against TASK-413's number — a regression there is a finding, not a footnote**) and the commit, including **the spike-harness deletion in the same commit as the subsystem**. ⚠️ **TASK-424 (QA on the subsystem) is NOT superseded — it is read-only, owes its own report, and gates TASK-447.**
- blocked-by: ~~TASK-424 PASS~~ — **n/a, superseded**
- parallel-safe: no (EXCLUSIVE compile + Git — serialize with TASK-414 / TASK-422)
- spec: >
    Compile GREEN (game module + plugin). Then a **machine smoke test**: `Siege.Llama.Info` still reports the backend; the subsystem loads the
    model asynchronously **without blocking match start**; `RequestCompletion` returns a parseable JSON for one hard-coded prompt+GBNF;
    a second concurrent call **returns false** (queue depth 1); `CancelActiveRequest()` mid-prefill aborts within the hard timeout;
    and a run with the model file **deliberately absent** leaves the match fully playable with one log line. **Re-measure the frame-time hitch
    on the partial tier and compare against TASK-413's number** — a regression here is a finding, not a footnote.
    Then commit **on `main`, NO PUSH**, per-deliverable: **commit A** = the subsystem (+ the spike-harness deletion in the same commit, since
    the deletion is part of that change) + `qa/TASK-424.md` + `handoffs/` + board.
    ⚠️ Stage by **EXPLICIT PATHSPEC**, re-check `git status --porcelain` after staging (the TASK-378 hazard); **STOP** on anything foreign.
    `git diff --stat` clean · **`L_Arena` NEVER saved** · `reset --hard` / `clean -fd` BANNED · **real hashes, not placeholders** ·
    **verify the ahead-count before asserting one.**
    Report `handoffs/TASK-425-buildmaster.md`. Post the hashes + the smoke-test results in 🔧 Build & Git.
- names: >
    Commits on `main`. Law: the hard gate + the per-deliverable commit pattern.

