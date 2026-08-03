# TASK-410 — [LLM-1a] THE GO / NO-GO SPIKE HARNESS

**Agent:** gameplay-programmer · **Date:** 2026-08-02 · **Status:** `ready-for-qa` (gate = TASK-412)
**Lane posture:** developed on `main`. **NOT COMMITTED, NOT PUSHED** — TASK-414 owns the commit.
**No compile run. No Git. `L_Arena` never opened. No `Content/` change. No editor touched.**

**M8 DECLARATION DUTY:** adds no replicated property, no new replicated class, no new relevancy tier.

---

## 0. ⛔ READ THIS FIRST — TASK-413 CANNOT COMPILE UNTIL SOMEONE ELSE FIXES ONE LINE

### BLOCKER-1 — `Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaModule.h:38` does not compile

```cpp
/**
 * True only when every vendored llama.cpp DLL resolved.
 *
 * !! GATE EVERY llama_*/ggml_* CALL ON THIS !!        <-- the */ ENDS THE COMMENT HERE
 * The DLLs are delay-loaded, so calling into them while this is false
 * raises a structured exception rather than returning an error.
 */                                                     <-- now an unmatched */
bool IsLlamaAvailable() const { return bLlamaAvailable; }
```

The `*/` inside `llama_*/ggml_*` terminates the block comment. `ggml_* CALL ON THIS !!` then parses
as a class-scope declaration and the trailing `*/` is unmatched. **This is the CONVENTIONS §10
compile trap ("no literal `*/` inside doc comments") occurring verbatim**, and it fails **every**
translation unit that includes the header — all four plugin `.cpp` files, including this one.

It survived because TASK-409's build was **deliberately deferred** under the quiet-module law, so the
plugin has never been through UBT. **`SiegeLlamaModule.h` is TASK-409's file under ruling 6, so I
flagged it instead of editing it.** One-line fix: `GATE EVERY llama_ AND ggml_ CALL ON THIS`.

**How it was found (not by reading — by sweeping):** a scanner over every plugin and assistant source
for a multi-line block comment whose `*/` is followed by code on the same line. Two hits, both real:
this one and one I had just written myself in the same phrasing (fixed). The `/*ParamName*/ false`
inline comments in `SiegeAssistantCommand.cpp` and `SiegeAssistantInputProbe.cpp` are false positives.
**Re-run the sweep at TASK-412 — it is three lines of Python and it catches a whole error class.**

---

## 1. What was built

**ONE new file, and it is the only file this task touched:**
`Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` (3,260 lines, heavily commented).

Six `FAutoConsoleCommandWithWorldAndArgs` in the `Siege.Llama.*` namespace. **Never a
`UFUNCTION(exec)`** — `USiegeCheatManager` and `ASiegePlayerController` are untouched, which is what
keeps the lane new-files-only (CONVENTIONS §5).

| Command | Bars | What it does |
|---|---|---|
| `Siege.Llama.SpikeLoad` | — | Loads the GGUF for a tier; prints `n_layer`, load ms, model size, VRAM/RSS deltas |
| `Siege.Llama.SpikeUnload` | — | Frees model + context (refuses while a job is in flight) |
| `Siege.Llama.SpikeBench` | **#1 #2 #3 #4** | Hitch histogram, TTFT + wall clock, turn-1 vs turn-2 prefill, peak VRAM/RSS |
| `Siege.Llama.SpikeEval` | **#5** | Scores the sealed corpus per split, reported separately |
| `Siege.Llama.SpikePrompt` | — | Dumps exact Zone A/B/C bytes + char and token counts (the `MaxSnapshotChars` correction) |
| `Siege.Llama.SpikeGrammar` | — | Dumps the GBNF so QA **diffs** it instead of proof-reading it |

Bar **#6** is TASK-411's `Siege.Assistant.InputProbe` and is deliberately absent here.

Every measurement line carries a grep tag: `SPIKE_RUN` `SPIKE_LOAD` `SPIKE_HITCH` `SPIKE_LATENCY`
`SPIKE_PREFILL` `SPIKE_MEM` `SPIKE_TOKENS` `SPIKE_EVAL_ROW` `SPIKE_EVAL_SCORE` `SPIKE_WARN`.

---

## 2. ⚠️ WHAT EACH MEASUREMENT ACTUALLY MEASURES — read before quoting any number

### Bar #1 — frame-time hitch histogram

- **Sampled on the game thread** by an `FTSTicker` delegate recording the true tick-to-tick period.
  During the measured window it does **one `FPlatformTime::Seconds()` and one array append** —
  process RSS is read every 30th frame and **device VRAM is never read on the game thread at all**,
  because a per-frame driver query would pollute the very histogram being measured.
- **Reported as WORST and p99. The mean is printed last and explicitly labelled "NOT the bar."**
- **A baseline control window is captured first** (default 180 frames) on the same map, in the same
  session, with the model already resident and nothing running. Every inference-phase line is
  followed by an `ATTRIBUTABLE worst_delta / p99_delta` line against it. **`L_Arena` with a fielded
  army hitches on its own; an absolute worst-frame number with no control is not attributable to
  inference.** If the baseline window came up short, the runner says so and tells you to distrust
  the deltas.
- Phases are separated: `baseline` / `load` / `prefill` / `decode`.

> ⚠️ **DEVIATION, DECLARED: inference runs on a background thread at `TPri_BelowNormal`, via
> `AsyncThread` — not an `FRunnable` class, and not on the game thread.** The spec says "no
> `FRunnable`", which I read as "do not build TASK-423's production worker", and no worker class
> exists here. But a **game-thread-synchronous** spike cannot measure bar #1 *at all*: it would
> report one multi-second frame and manufacture a NO-GO the shipped design (a below-normal worker,
> per the plan's own mitigation list) would never have hit. **If QA rules that literal, the harness
> cannot answer bar #1 and the bar has to be rewritten.** Flagging rather than hiding.

### Bar #2 — TTFT and wall clock

- **TTFT is measured from the start of the request** (tokenize included), not from the start of
  prefill. That is the number a player feels.
- ⚠️ **"60-token equivalent" is EXTRAPOLATED, and it says so on the line.** A grammar-constrained
  JSON command terminates when the object closes — it physically cannot be stretched to 60 tokens.
  The line prints measured `ttft_ms`, `decode_ms`, `out_tokens` and `ms_per_token` **beside** the
  extrapolation so the arithmetic is auditable. `grammar=0 tokens=60` gives a genuine fixed-length
  unconstrained decode as a control.
- **Greedy sampling, deliberately.** A go/no-go number that moves between runs is not a number.

### Bar #3 — KV-prefix reuse

- Turn 1 is forced **cold** (`llama_memory_clear`) so the ~70 % claim is measured against a genuine
  full prefill, not against whatever happened to be resident.
- Reuse = the token-level common prefix with the previous turn's prompt; the divergent tail is
  dropped with `llama_memory_seq_rm` and only that tail is re-prefilled.
- ⚠️ **`llama_memory_seq_rm`'s return value is checked.** It is documented to return false when a
  partial removal is impossible. Ignoring it would leave stale KV under this turn's positions — which
  shows up as a bad *accuracy* number with no log line naming the cause, while bar #3 cheerfully
  reports a large and entirely fictional reuse. On false the runner clears the whole cache and says
  the turn's reuse figure is 0 *by construction, not by measurement*.
- Below 60 % the runner prints the plan's own instruction: **fix the prompt layout before reading
  anything else.**

### Bar #4 — peak VRAM and RSS

- ⚠️ **`ggml_backend_dev_memory` reports DEVICE-WIDE free bytes, not this process's usage.** So the
  headline is a **free-VRAM low-water mark for the whole machine**, which is the right shape for the
  "fits 8 GB with room" question but is **not** a process attribution. **The model's own footprint is
  the before/after-load delta**, logged separately on the `SPIKE_MEM stage=model_load` line.
- Sampled **only from the worker**, at phase boundaries and every 8 decoded tokens.
- RSS: `FPlatformMemory::GetStats().UsedPhysical`, sampled every 30 frames plus at load boundaries.
- "With the game at *its* peak" is **TASK-413's** job — field the army first. The harness records the
  map name and net mode on the `SPIKE_RUN START` line and **warns if the world is not a game world**,
  so an empty-map run is self-evident in the log instead of a claim to be trusted.

### Bar #5 — accuracy (see §4 — this one has a judgement call in it)

---

## 3. The exact prompt bytes and the exact GBNF

### Zone A — 4,314 chars, 85 lines

Transcribed line-for-line from the landed `USiegeAssistantSnapshot::BuildZoneA`, plus the synonym
block, which is `USiegeAssistantVocabulary`'s **C++ constructor defaults** run through
`BuildSynonymTable()`'s normalisation (per row lower-cased, de-duped, canonical removed, sorted by
string; rows sorted by canonical; empty rows dropped).

Dump it verbatim at runtime with `Siege.Llama.SpikePrompt`. Structure:

```
[RULES] / schema (a command) / schema (a question) / intents / places /
rules / synonyms(SYNONYMS [units] [places] [intents] [notes]) / examples
```

**The three few-shots are TASK-416's, unchanged. This task introduced ZERO new few-shots** — they
were authored before it ran, so **the coverage is not reactive** (CONVENTIONS §11 declaration duty),
and none of them appears in either corpus file.

> ⚠️ **THE SYNONYM TABLE IS TRANSCRIBED VERBATIM AND UNIMPROVED, INCLUDING THE ENTRY QA HAS ALREADY
> RULED AGAINST.** `ancient_ground_near <- ancient ground, ...` is present because it is on disk
> today. `qa/TASK-419.md` **WARN-6** orders TASK-421 to remove it, precisely because the bare alias
> quietly resolves the ambiguity `DEV-06` was built on. **So this fixture is PESSIMISTIC on those
> rows relative to what will ship, and it was left wrong on purpose** — silently "fixing" the prompt
> in the direction that raises my own score is exactly what the sealed corpus exists to prevent.
> Once `DA_AssistantVocabulary` lands, re-run with `prompt=<path>` holding the real `BuildZoneA`
> output; no rebuild needed.

### Zone B — 68 chars · Zone C — 887 chars · **B + C = 955 of 1,440 (485 headroom)**

```
[MATCH]
own_castle_hp: 80%
enemy_castle_hp: 60%
mid: ours
gold: 120
[FORCES]
places: own_castle, enemy_castle, mid, ancient_ground_near, ancient_ground_far, nearest_mine, hero
roster:
- footman: 8 total, 8 orderable, 8 followable
- archer: 6 total, 6 orderable, 6 followable
- knight: 3 total, 3 orderable, 3 followable
- miner: 3 total, 3 orderable, 0 followable
- militiamob: 4 total, 4 orderable, 4 followable
- pikeman: 5 total, 5 orderable, 5 followable
- sapper: 2 total, 0 orderable, 2 followable
- cavalry: 4 total, 4 orderable, 4 followable
- longbowman: 3 total, 3 orderable, 3 followable
- cleric: 2 total, 0 orderable, 2 followable
- ogre: 1 total, 0 orderable, 1 followable
- wizard: 2 total, 2 orderable, 2 followable
- sorcerer: 1 total, 1 orderable, 1 followable
other_kinds: none
stances: free 20, following 12, holding 8, ambushing 4
hero: alive
pending: none
[ORDER]
order: <the sentence>
```

Roster counts are chosen so the corpus's *stated* situations are true — footman is **8** so DEV-03's
"you asked for 10 and 8 exist" is a real shortfall; knight is **3** so DEV-11's deferred trigger is
not already satisfied. Orderable/followable follow the **shipped** eligibility split (Cleric
follow-only, Ogre/Sapper no zone orders, Miner zone-orders but no auto-follow).

### The GBNF (1,431 bytes) — generated, not pasted

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
kind ::= "\"footman\"" | "\"archer\"" | "\"knight\"" | "\"miner\"" | "\"militiamob\"" | "\"pikeman\"" | "\"sapper\"" | "\"cavalry\"" | "\"longbowman\"" | "\"cleric\"" | "\"ogre\"" | "\"wizard\"" | "\"sorcerer\""
where ::= "\"own_castle\"" | "\"enemy_castle\"" | "\"mid\"" | "\"ancient_ground_near\"" | "\"ancient_ground_far\"" | "\"nearest_mine\"" | "\"hero\"" | "\"none\""
count ::= "1" | "2" | ... | "30" | "\"all\""
at_least ::= "1" | "2" | ... | "30"
item ::= "{\"kind\":" kind ",\"n\":" count "}"
selection ::= "[" item "]" | "[" item "," item "]" | "[" item "," item "," item "]"
who ::= selection | "\"all\"" | "\"none\""
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at_least "}"
```

**Written as a generator mirroring `USiegeAssistantGrammar::Build`, not as a pasted string** — 30
`count` alternatives are exactly the sort of thing that gets mistyped, and a generator means QA
**diffs behaviour** rather than proof-reading quotes. `Siege.Llama.SpikeGrammar` prints it.

**§9c conformance, checked by eye against the landed file:** key order `intent, who, where, when` ·
pair keys **`kind`/`n`** (not `count` — that is the *rule* name) · army-wide verbs emit
**`"who":"none"`** · the **`{"ask":ASK}`** branch exists and Zone A teaches it. `count` is
**1..30, not 1..live-max** (CONVENTIONS §1). The 3-kind cap is a **bounded alternation in the
grammar**, so a 4-kind selection is unreachable by the sampler, not merely rejected after.

> **The landed grammar is the authority. If this file and `SiegeAssistantGrammar.cpp` disagree,
> THIS FILE IS WRONG.**

---

## 4. ⚠️⚠️ BAR #5's SCORING — the one judgement call, stated in full because it moves the number

The sealed corpus is **internally inconsistent about what a `Clarify` row requires of a single-turn
model, and it is inconsistent for a good reason**: `Clarify` is an outcome of the *product*, reached
by two different routes.

| route | example | correct model output |
|---|---|---|
| the **model** declines | `DEV-06` "send the mage to the ancient ground" | `{"ask":"which_unit"}` |
| the **executor** objects | `DEV-03` "send 10 footmen" with 8 alive | a **full, correct command** — the row's own note says *"the parse is correct and the EXECUTOR is what must object"* |

So a `Clarify` row cannot demand one output form. **The rule I implemented:**

- **`Execute`** → must be a **command**; every asserted field must match.
- **`Refuse`** → must be a **question** (any ask code — the corpus does not assert the taxonomy, so
  neither does the scorer).
- **`Clarify`** → **either** form. If a command was emitted, every asserted field must still match.
  If a question was emitted, the asserted fields are **unobservable** and are **skipped**.
- **An empty cell is "not asserted", never "asserted empty"** — scored as **skipped**, never matched
  (CONVENTIONS §11). Kinds compare **order-insensitively over (kind, count) pairs**; the model's
  ordering is not a semantic difference.
- Deferred-trigger fields are compared and reported **separately** — they are not among bar #5's
  four fields and must not quietly widen the bar.

### The leniency is real, so the runner BOUNDS IT WITH A NUMBER instead of promising it is small

That last clause can be gamed, so **three honesty lines print beside every score, for both splits**:

1. **`STRICT=`** — a second score in which a question passes a `Clarify` row only if that row asserts
   **nothing**. Printed on the same line as the primary.
2. **`LENIENCY FLOOR=`** — computed, not claimed: what a degenerate model answering `{"ask":...}` to
   *every* sentence would score. It passes exactly the non-`Execute` rows. **On dev that floor is
   12/25 = 48 %** (13 Execute, 9 Clarify, 3 Refuse). The 85 % bar is not reachable by refusing —
   but "bounded" is not "absent", and now nobody has to take my word for the bound.
3. **thin-row and zero-row lines** — `%d row(s) assert at most ONE field` and `%d row(s) assert
   NOTHING AT ALL and pass on any parseable output`, each listing the IDs.

### What that third line exposes on dev — QA and TASK-413 should see this before the verdict

Verified with a Python mirror of the runner's own parser, run over **dev only**:

- **6 of 25 dev rows assert ≤ 1 field**: `DEV-04 DEV-06 DEV-08 DEV-09 DEV-10 DEV-25`.
- **`DEV-08` asserts NOTHING** (`intent=unasserted`, no kinds/counts/where, `Clarify`) — **it passes
  on any parseable output.**
- **`DEV-25` ("rally on me") asserts only `intent=rally`.** Its note says *"a model that invents a
  where fails this row"* — **but the scorer cannot enforce that**, because `ExpectWhere` is empty and
  empty means *not asserted*. This is exactly CONVENTIONS §11's recorded overloaded-empty-cell
  limitation; fixing it needs a `<none>` sentinel in a **new** corpus, never an edit to the sealed one.

**None of this is a corpus defect** — §11 predicted it and accepted it for v1. It is stated here so
the go/no-go reader knows how much of the headline percentage is evidence of translation.

### Corpus contract verified (dev only — the holdout was never opened)

Header matches character-for-character · **25/25 rows parse to exactly 9 fields** · **25/25 carry the
pinned `^intent=([a-z]+); ` prefix**, all values inside {7 intents, `none`, `unasserted`} · every
count numeric · every `ExpectOutcome` inside the pinned three · kinds/counts index-aligned wherever
both are asserted. **0 problems.**

**The CSV splitter is full RFC 4180** (quoted fields, doubled quotes, embedded commas) —
**deliberately, because the holdout could not be inspected.** A naive split on `,` would silently
shift every column of any quoted row, and that failure would look like a model error at the one
moment nobody is allowed to re-open the file. A structurally bad *row* is reported and skipped, not
fatal: a run that dies on row 7 of a file that may not be re-opened is far worse than one that scores
14 of 15 and says so.

### ⚠️ FEW-SHOT DISJOINTNESS: the literal test PASSES, but one shot is a near-paraphrase of DEV-01

Checked literally against dev: **no few-shot appears as a corpus sentence.** §11's criterion is met.
But surface overlap is worth naming rather than leaving for someone to notice in the diff table:

| few-shot | closest dev row | Jaccard |
|---|---|---|
| `send ten footmen with a sorcerer to the ancient ground on our side` | **DEV-01** `send 10 footmen with a sorcerer to the nearest ancient ground` | **0.60** |
| `all archers guard the middle` | DEV-12 | 0.33 |
| `everyone attack` | DEV-13 | 0.17 |

**Neither was fitted to the other — both descend independently from the plan's own flagship sentence**
("send 10 footmen with a sorcerer to the nearest ancient ground"), which predates both the corpus and
Zone A. **But DEV-01 is therefore not an independent test of the multi-kind pattern**, and it should
not be quoted as evidence that multi-kind generalises. **The holdout is where multi-kind is actually
tested** — which is the split's whole point. (These are TASK-416's few-shots; I changed none of them.)

### 🔒 THE HOLDOUT WAS NOT OPENED, READ, PRINTED, LOADED OR INSPECTED BY THIS TASK

`Docs/Data/assistant_eval_holdout.csv` was never touched — not by a command, not by a script, not
structurally. `handoffs/TASK-426-artist.md` was also **not** read, since it may quote holdout rows.
`Siege.Llama.SpikeEval` therefore gives `dev=` a default and **deliberately gives `holdout=` none**,
so the sealed file can only ever be opened by an explicit act. When it is, the runner prints the
ruling-14 reminder first: **report the holdout number against the bar, and do not tune anything
afterwards.**

---

## 5. 🚩 FINDINGS AGAINST OTHER TASKS — flagged, not fixed (single-owner law)

**(a) BLOCKER-1 — `SiegeLlamaModule.h:38` does not compile.** See §0. TASK-409's file.

**(b) `use_mmap` NO LONGER EXISTS in the vendored build, and TASK-423 will go looking for it.**
CONVENTIONS §7 and the plan both mandate `use_mmap = true` (load-bearing for the 8 GB claim). In
llama.cpp **b10235** that bool is **gone from `llama_model_params`**; it was replaced by
`enum llama_load_mode load_mode`, and `LLAMA_LOAD_MODE_MMAP` is its exact successor. The spike sets
it. **The requirement survives; only the field name changed.**

**(c) `abort_callback` — the documented limit, recorded now rather than discovered later.**
`llama.h` states plainly that it *"currently works only with CPU execution."* So on the full/partial
GPU tiers the 10 s hard deadline can only fire **between graph submissions**, not inside one, and the
**token budget is what actually bounds a GPU run.** CONVENTIONS §8 calls `abort_callback` "the only
correct mechanism" — on GPU it is necessary but **not sufficient**.

**(d) FINDING for TASK-416 — §8's "Zone A ~600 tok as-built" was measured with NO vocabulary.**
Reproduced exactly: Zone A **minus** the synonym block is 2,142 chars ≈ **595 tok** at 3.6 c/t, which
is §8's number to within rounding. **With** `DA_AssistantVocabulary`'s table (2,172 chars) Zone A is
**4,314 chars ≈ 1,100–1,200 tok** — roughly double. Bar #3's *ratio* gets **better** (a bigger static
prefix reuses more), so this is not a threat to that bar; but **turn-1 cold prefill — the first
sentence of every match, i.e. bar #2's worst TTFT — roughly doubles.** Not fixed here; §8's figure
wants a footnote once `DA_AssistantVocabulary` lands.

**(e) FINDING for TASK-416 — `MaxRosterKinds = 8` hides kinds the grammar still admits.**
`GetUnitKinds()` feeds the grammar and is **uncapped**, while Zone C's printed roster is capped at 8
and the tail collapses into `other_kinds:`. On a board with >8 kinds alive the sampler can emit a
symbol the prompt never showed the model — the §9c Zone-A/grammar seam, one level down.

> **DECLARED DEVIATION: the spike prints all 13 kinds and does not honour `MaxRosterKinds`.** In
> DT_Cards row order the first eight are Footman, Archer, Knight, Miner, MilitiaMob, Pikeman, Sapper,
> Cavalry — which collapses **Cleric, Ogre, Wizard and Sorcerer**, the four kinds six sealed dev rows
> are built on. Honouring the cap would have converted six *model* results into six *harness
> artefacts* and read as "the model is bad" when the model was never asked. **Zone B+C measures 955
> of 1,440 chars, so the cap that actually governs the ≤ 400-token budget was never the constraint.**

**(f) A real bug caught in my own code before it shipped, named because it is the class of thing a
compile would not have caught cleanly.** The first draft submitted the whole prefill as **one**
`llama_batch`. The prompt is ~1,000+ tokens and `n_batch` is 512; `llama_decode` **rejects a batch
larger than `n_batch` outright**, so the very first call of the very first tier would have returned
`-1` and every bar would have read "inference failed" rather than "prefill is too big". The prefill
is now **submitted in `n_batch` slices**, and there is a pre-check that reports a prompt exceeding
`ctx` as a **prompt-budget finding** rather than an opaque return code.

---

## 6. How to reproduce every number

```
0.  Fix BLOCKER-1 (SiegeLlamaModule.h:38).                       <-- TASK-409's file
1.  Set "SiegeLlama" -> "Enabled": true in GitClaudeUnrealTest.uproject.  <-- TASK-409's file
2.  Build.bat GitClaudeUnrealTestEditor Win64 Development -project=... -waitmutex
    (judge on log text, NOT exit code -- Build.bat returns 0 on failure)
3.  python Tools/fetch_llm_model.py ...       -> <ProjectRoot>/Models/<file>.gguf
4.  Open the editor, PIE on L_Arena, FIELD AN ARMY ON BOTH SIDES (SummonTestUnit).
5.  Siege.Llama.Info                          -> DLLs + backends + model present
6.  Siege.Llama.SpikePrompt                   -> exact zone bytes, chars, TOKEN COUNTS
7.  Siege.Llama.SpikeGrammar                  -> diff vs USiegeAssistantGrammar::Build
8.  Siege.Llama.SpikeLoad tier=cpu            -> prints the model's REAL n_layer
9.  Siege.Llama.SpikeBench tier=full     iters=5
    Siege.Llama.SpikeBench gpulayers=<n_layer/2> iters=5     <-- THE PARTIAL TIER, the one bar #1 is judged on
    Siege.Llama.SpikeBench tier=cpu      iters=5
10. Siege.Llama.SpikeEval                                     <-- dev only, tune here if you must
11. Siege.Llama.SpikeEval holdout=Docs/Data/assistant_eval_holdout.csv   <-- ONCE. Then stop tuning.
12. Siege.Assistant.InputProbe (TASK-411) in the SAME session -> bar #6
13. Restore the .uproject if you toggled it, and VERIFY BY CHECKSUM, NOT git diff.
```

### ⚠️ THE `.uproject` — NOT TOUCHED BY THIS TASK, AND HERE IS THE VERIFIED BASELINE

The plugin is currently `"Enabled": false`, which is also what `HEAD` contains — the file is **clean**
against `HEAD` right now. **I did not enable it**, for three reasons, the last of which is decisive:

1. Enabling it while unbuilt makes the **editor refuse to launch**, and Jonathan may open the editor.
   Leaving a landmine is worse than leaving a documented step.
2. The build is build-master's, and **TASK-413 step (0) already owns "compile GREEN".**
3. **`GitClaudeUnrealTest.uproject` is TASK-409's exclusively-owned file under ruling 6.** Editing it
   from TASK-410 is a single-owner violation regardless of how small the edit is.

**Baseline for the checksum restore (recorded before anything, re-verified after):**

```
sha256  2147635ca97e6f4224727594093415271830246ea475370c84ebc9f99574182a
bytes   860
git     clean vs HEAD
```

⚠️ **Verify the restore by `sha256sum` + byte count, NEVER by `git diff`.** A `.uproject` round-tripped
through Python came back 805 B instead of 859 B — CRLF→LF on every line ending — and **`git diff`
showed it perfectly clean, because `autocrlf` normalises exactly that away** (GIT HAZARD LAWS (a)).

---

## 7. What QA should scrutinise hardest

1. **BLOCKER-1**, and **re-run the stray-`*/` sweep** across the whole batch — it is an error class,
   not one typo, and both hits were the same phrasing.
2. **The bar-#5 Clarify rule (§4).** It is the one place I made a call the corpus does not make for
   me. If QA prefers the strict reading, **the number is already printed** — no code change needed,
   just say which one TASK-413 reports.
3. **The few-shot / GBNF seam (§9c), by execution not by eye.** All three Zone-A few-shots must parse
   under the landed grammar. `Siege.Llama.SpikeGrammar` prints the GBNF; the few-shots are in §3.
   **"It looks like the schema" is not evidence** — a few-shot the grammar would reject is a BLOCKER.
4. **Few-shot disjointness from both corpus files, by literal string comparison** (CONVENTIONS §11).
   The three few-shots came from TASK-416 unchanged; I added none. Please still check it literally.
5. **The 13-kind roster deviation (§5e)** — declared, quantified, and reversible in one array.
6. **The background-thread deviation (§2)** — declared. If it is ruled out, bar #1 has no answer.
7. **Threading**: all llama/ggml state is touched only from a spike worker; the only cross-thread
   state is two `FThreadSafeCounter`s; `SpikePrompt`/`SpikeUnload` refuse while a job is in flight;
   VRAM is never queried from the game thread. `FCoreDelegates::OnEnginePreExit` frees the model
   before the module frees the delay-loaded DLLs.
8. **Not verified by me, and I could not verify it:** *nothing here has been compiled.* No game-module
   build was run (the quiet-module law, and it is not my job); the plugin cannot build until
   BLOCKER-1 lands. What **was** verified: every `llama_*`/`ggml_*` signature used here was read out
   of the **vendored** headers rather than remembered (`load_mode`, `llama_memory_seq_rm`,
   `llama_sampler_init_grammar`, `llama_chat_apply_template`, `llama_batch_get_one`,
   `ggml_backend_dev_memory`), every UE API against the 5.8 engine headers on disk
   (`AsyncThread`, `FTSTicker::AddTicker`, `FPlatformMemoryStats::UsedPhysical`,
   `PLATFORM_SEH_EXCEPTIONS_DISABLED`, `FConsoleCommandWithWorldAndArgsDelegate`'s
   `(Args, World)` order), and the corpus contract by running a mirror of the parser over dev.

---

## 8. Law compliance

| Clause | How |
|---|---|
| §1 central law | One utterance + one snapshot → one constrained JSON. **No model output is ever fed back.** What is reused across turns is the **KV cache of a byte-identical static prefix** — a cache, not a conversation. |
| §1 count 1..30 | Grammar emits 1..30 + `"all"`, **never the live max**. `at_least` is the same range **without** `"all"`. |
| §3 symbols only | No coordinate, no prose, no player-facing text anywhere in the prompt or the output surface. |
| §5 console commands | Six `FAutoConsoleCommandWithWorldAndArgs` in `Siege.Llama.*`, in ONE new plugin-private file. **No `UFUNCTION(exec)`.** |
| §7 repo hygiene | No `.gguf` touched, `.gitignore` untouched, no `Content/` change, no Git. |
| §8 zone layout | Byte layout mirrors the landed builders; fixed key order; every key always emitted; bands only; no timestamps, no coordinates. |
| §8 GGML crash | SEH around the whole job via a thunk with no destructible locals; a fault logs and voids the run instead of taking the editor down. |
| §9 pinned registry | `SiegeAssistantMaxSelectionKinds` 3, ask codes, intent symbols, JSON keys `intent/who/where/when/kind/n/at_least/ask` — all transcribed, none re-invented. |
| §9a place vocabulary | The seven pinned spellings, in the pinned order. **`own_castle`, not `my_castle`** — verified against the file that asserts it, not the message that quotes it. |
| §9b `mage` prohibition | `mage` / `caster` / `spellcaster` appear under **neither** wizard nor sorcerer. The `[notes]` block routes them to `which_unit`. **No alias added.** |
| §10 tunables | ctx 2048 · MaxOutputTokens 96 · HardTimeout 10 s · GrammarCountMax 30 · MaxSelectionKinds 3, all named constants. |
| §11 sealed corpus | Loaded **by path at runtime**, never `#include`d — the plugin gains no Siegebound dependency. Holdout never opened. Intent parsed from the pinned `Notes` prefix; the corpus was **not edited, narrowed or reformatted**. |
| Quiet-module law | **No build run.** No game-module file touched. |

---

## Fix loop 1 (2026-08-02) — answering `qa/TASK-412.md`

**One file touched: `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp`.** Nothing else. The corpus,
Zone A's bytes, the GBNF, the few-shots and the scoring rule are byte-unchanged. **NOT COMPILED** — the editor is
open and build-master owns the gate.

Per blocker: what changed, and **which number it moves**.

### BLOCKER-1 — bar #3 measured a KV reuse the shipped path cannot reach ✅

**What changed.** There is now a **second world fixture**. `FSpikeWorldFixture` holds one complete Zone B + Zone C
state (HP bands, mid owner, gold, roster, stances, hero), and there are two instances:

- **`t0`** — the corpus's board. **Its emitted bytes are unchanged**: the refactor turned literals into `Appendf`
  calls that produce the identical characters, so Zone B is still **68** chars and Zone C still **887** for the
  default order line (I re-derived both by hand; `Siege.Llama.SpikePrompt` now prints a WARNING if they ever drift).
- **`t1`** — the same board a few seconds later. Every Zone B key moved (`80%` → `70%`, `60%` → `45%`, `ours` →
  `neutral`, `120` → `165`), the roster shrank, stances moved, hero went `alive` → `down`. **Same 13 kinds in the
  same order** — the GBNF's `kind` alternatives are generated from `SpikeRoster`, so a fixture with different kinds
  would open the §9c seam. That invariant is a `static_assert` on the count **plus** a run-time
  `VerifyFixtureKindParity()` that the bench and `SpikePrompt` both call.

The bench now runs **two bounds** instead of one number (`RunPrefillBound`):

| bound | fixtures | where it diverges | how to read it |
|---|---|---|---|
| `BEST_CASE` | t0 → t0 | the `order:` line, end of Zone C | a **paused** board. This is the ~97-98 % the old code reported. **Do not quote it.** |
| `SHIPPED_WORST_CASE` | t0 → **t1** | the **start of Zone B** | a **live** board — the shipped worst case. **This is bar #3.** |

Every `SPIKE_PREFILL`, `SPIKE_LATENCY` and `SPIKE_EVAL_ROW` line now carries
`reused=N landed_in=ZONE_A|ZONE_B|ZONE_C zoneB_start~=N zoneC_start~=N` — the reuse point **against the zone
boundaries in token space**. The boundaries come from `BuildPrompt` reporting the zones' **character** offsets
structurally (never by searching for a marker like `[FORCES]`, which also appears inside Zone A's schema block) and
tokenizing the prefix. They are marked `~` because tokenization is not compositional; ±1 does not affect the only
question being asked, which is *which zone*.

**New guard, and it is the one that was missing:** when a bound claims to be shipped-shaped and the reuse lands at
or past the start of Zone C, it warns loudly. The old `DropPercent < 60.0` guard fired only on too **little** reuse
and was silent on implausibly too **much** — the direction that flatters.

**Numbers this moves.**

- **Bar #3** — the headline drop moves from a reported ~97-98 % to whatever the SHIPPED bound measures (expect the
  ~78-82 % family, near §8's 165/765 = 78 %). **Both bounds are printed, so the gap between them is itself evidence.**
- **Bar #2** — the warm iterations now **alternate t0/t1**, so every one pays the full Zone B + Zone C re-prefill
  (~265 tok, not ~20). `mean_ttft_ms` / `WORST_wall_ms` will get **worse** and are now the shipped shape;
  `mean_prefill_tok` is printed beside them so the basis is visible. The best-case warm TTFT is still on the record
  on the `BAR#3 BOUNDS` line, so both bounds exist for latency too.
- **Bar #5 — unaffected.** The eval is pinned to **t0**, with a comment saying why: DEV-03's shortfall is only real
  while footman is 8 and DEV-11's trigger only unsatisfied while knight is 3. Running a split against t1 would
  rewrite the corpus's premises without touching the corpus. Each split also prints one line saying its per-row
  `prefill=` / `reused=` columns are a **constant-fixture (best case)** measurement and are **not** bar #3.

⚠️ **Cost:** the prefill phase now runs **4 generations instead of 2** (two cold turn-1s, two turn-2s). On the CPU
tier that is a few extra seconds before the iteration loop.

### BLOCKER-2 — the offload device is now pinned and named ✅

**What changed.**

- New **`gpu=<index|name>`** argument on every command. The index is the one printed on the device-inventory lines,
  so what the log prints can be pasted straight back in; a name is matched against `ggml_backend_dev_name`.
- `EnsureModelLoaded` builds an explicit **one-entry `devices` array** and sets **`split_mode =
  LLAMA_SPLIT_MODE_NONE`** and **`main_gpu = 0`**. `main_gpu` indexes `params.devices`, so with a one-entry list it
  is unambiguous — no assumption about where ggml's own filtered ordering would have put the card.
- **Every** device is logged **before and after** the load — `dev=<index>/<name>/<type> free= total= desc=` — with
  the pinned one marked `<== PINNED, AND THE SUBJECT OF EVERY vram_ FIGURE`.
- `SampleMemory()` now reports on **`GRunner.OffloadDevice`** (the pinned one) instead of "the first discrete GPU it
  finds". Every VRAM line carries **`vram_dev=`**. With no device it prints **`n/a`**, not `0` (that was NIT-3, and
  it was the same lines).
- If nothing was pinned, or the device was only assumed because no `gpu=` was given, the bench says so at the end
  and tells the reader to re-run with an explicit index.
- Selecting a CPU/ACCEL device by name is refused with a reason rather than silently pinned; falling back to an
  iGPU says out loud that bar #4's 8 GB question is about a discrete card.

**Numbers this moves.** **Bar #4.** Previously the weights could layer-split across the iGPU **and** the dGPU while
the free-VRAM low-water mark came from one of them — understating the footprint, i.e. **manufacturing a PASS** on
"does it fit 8 GB". Now the tier means one named card, and the before/after delta is two samples of the **same**
device. Bars #1/#2 also stop being measured on a split configuration no player has.

⚠️ **`enum ggml_backend_dev_type` is spelled with the elaborated `enum` keyword** in two places, and it must be:
ggml declares a **function** of exactly that name (`ggml-backend.h:182`), which hides the enum type in C++. The
vendored header does the same thing for the same reason. This is the likeliest thing to look like a typo in review.

### BLOCKER-3 — a model swap can no longer be silently ignored ✅

**What changed.** The reload key gained the model path **and** the device spec — `GRunner.LoadedModelPath ==
Options.ModelPathOverride` and `GRunner.LoadedOptions.GpuDeviceSpec == Options.GpuDeviceSpec`, on top of the four
fields already compared. `UnloadModel()` now also clears `LoadedModelPath`. I did **not** take QA's stated
alternative (a procedural "run `SpikeUnload` between swaps") — QA did not recommend it either, and a procedural
mitigation for a silent wrong measurement is the shape that has burned this project.

**Numbers this moves.** `SpikeBench model=<B>` after `model=<A>` at the same tier now **reloads and prints a
`SPIKE_LOAD` line**. Previously it printed **A's numbers under B's command line with no load line at all** —
directly on TASK-413's model ladder. `FString::operator==` is case-insensitive, which is right for Windows paths; a
slash-style difference costs one honest reload, which is the safe direction.

### The four warns, same pass

- **WARN-1 ✅** `SpikeBenchUtterances[4]` was `"everyone pull back to our castle"`, byte-identical to a sealed
  holdout sentence. It is now `"whole army disengage from the middle and regroup at the home keep"` — same job (the
  only army-wide `who:"none"` shape in the list), deliberately long and compound so it cannot plausibly collide with
  a short authored corpus row. **I checked it against the dev split (no match). I did not and cannot check it
  against the holdout** — that file is not mine to open. The bench is unscored, so if it does collide no number is
  invalidated; change the string and re-run.
- **WARN-2 ✅** `llama_n_batch()`, `llama_n_ctx()` and `llama_n_ubatch()` are queried after context creation.
  `GRunner.ContextSize` is new and is what the prompt-budget pre-check and the context-full check use;
  `GRunner.BatchSize` now comes from the actual value, so a prefill slice can no longer exceed the effective
  `n_batch`. When any of the three is clamped, a loud `SPIKE_WARN` prints requested vs actual, and the `SPIKE_LOAD`
  line prints both as `ctx=N(req M)` / `ubatch=N(req M)`. **Moves:** prevents the failure mode where every bar reads
  "inference failed" for a reason no line names.
- **WARN-4 ✅** `bKindsOk` and `bCountsOk` are now two independent `KindSetMatches` calls. **Pass/fail is provably
  unchanged**: `bFieldsOk` is their conjunction, and `KindSetMatches(counts=true)` implies
  `KindSetMatches(counts=false)`, so the conjunction equals what the single call returned. Only the attribution
  moves — a right-unit/wrong-quantity row now prints `kinds=ok counts=BAD`, so TASK-413's per-row diff points the
  remediation ladder at counting instead of naming. (A wrong kind still shows `counts=BAD`, which is true: there is
  no quantity to compare.)
- **WARN-10 ✅** New derived option `bIgnoreEogForFixedLength`, set by the **bench only** when `grammar=0`. The
  decode loop then keeps going to the token budget, records `EogTokenIndex`, drops the pieces after EOG from the
  text (timing filler, not an answer), and prints
  `FIXED_LENGTH_CONTROL(grammar=0: EOG IGNORED at out token N ...)` on the line. **The eval job never sets it** — an
  accuracy run must stop where the model stopped, so no bar #5 number moves.

### What I did not do, and why

- **No compile, no build, no Git.** The editor is open.
- **WARN-3** (baseline captured before the first load of a tier) is a **procedure** item — QA's own remedy is "run
  each tier's bench twice and report the second". Restructuring the phase order would change what the control window
  means; I left it, and it stays on TASK-413's list.
- **WARN-5 / WARN-9 / NIT-4** are rulings about how to *read* bar #5 and are TASK-413's to state beside the number.
- **WARN-6** is `Tools/fetch_llm_model.py` — TASK-409's file, not mine.
- **WARN-7 / WARN-8** are `.gitignore` and `SiegeLlamaModule.cpp` — not my file.
- **NIT-1 / NIT-2** left as recorded for TASK-423; **NIT-3** is fixed as a side effect of BLOCKER-2 (`n/a` instead of
  a `0` that reads like a measurement).

### What QA should scrutinise hardest

1. **The t0 bytes.** The refactor must be byte-identical for t0 or the sealed corpus's premises shift under it. I
   re-derived Zone B = **68** and Zone C = **887** by hand against the new `Appendf` forms and they match; there is
   also a run-time tripwire in `SpikePrompt`. Please re-derive rather than take it.
2. **The `%` handling.** `AppendZoneB` uses `%%` in a **format** string (correct). The two bound-`Expectation`
   strings contain a bare `%` and are passed as **`%s` arguments**, never as format strings — doubling them would
   print `%%`. Both are commented at the site.
3. **`enum ggml_backend_dev_type`** (see BLOCKER-2 above) — reads like a typo, is required.
4. **Format-specifier / argument counts** on the rewritten `UE_LOG` lines. I audited every one of them by hand;
   every format string is still a literal.
5. **The comment trap.** I ran the terminator audit on **raw reads, not Grep** (per your tooling note): 78 block
   openers, 78 terminators, paired sequentially with no overlap and no premature close. The `/*ParamName*/` idiom is
   used in four new places (`bCompareCounts` twice, `bExpectZoneBDivergence` twice) and is the benign single-line
   form.
6. **The one-entry `devices` array's lifetime** — declared in `EnsureModelLoaded`'s scope, so it outlives
   `llama_model_load_from_file`, which copies it during the load.

---
---

# Post-TASK-413 fix pass — THE GRAMMAR NEVER PARSED, AND THE CAP RULING

**Date:** 2026-08-03 · **Author:** gameplay-programmer · **Status:** `ready-for-qa`
**Source:** `handoffs/TASK-413-buildmaster.md` PART 2 §13 (the blocker) and §12 (the cap trigger).

> **This one note covers three lanes' files**, because the defect and the ruling cut across them and splitting the
> record would hide the coupling. Board entries **TASK-410**, **TASK-416** and **TASK-417** each carry a
> `POST-TASK-413 FIX PASS` bullet pointing here.
>
> ⚠️ **NOT COMPILED. NO GIT.** Build-master owns the gate and re-runs bars #2 and #5.

---

## 1. What changed

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp` | RULE `at_least` → `at-least` (definition + reference); `IsLegalGbnfRuleName` + `CollectRuleReferences` added; `AppendRule` now validates |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.h` | Doc: the degenerate-input rule list renamed; a new kebab-case law paragraph |
| `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` | The same rename + the same validator (mirrored, not shared — the plugin may not include the game header); `SpikeMaxSnapshotChars = 1085` replaces two bare `1440` literals in the `SPIKE_TOKENS` line |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h` | `MaxSnapshotChars` 1440 → **1085** with the derivation; truncation latch replaced; zone-table figures replaced with TASK-413's measurements |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp` | `BuildZoneC`'s truncation now logs on the case that was silent; one Zone-A comment (not bytes) records the key/rule split |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` | Rename applied; `IsGrammarWellFormed`'s scanner corrected; **new test `Siegebound.Assistant.Grammar.RuleNameCharset`** |

**Not touched, deliberately:** the corpus, Zone A's bytes, the few-shots, the JSON schema, the scoring rule,
`ParseSiegeAssistantCommand`, `GrammarCountMax`, `SiegeAssistantMaxSelectionKinds`, `MaxRosterKinds`,
`ZoneBCharReserve`, `CONVENTIONS.md` (another agent holds it).

---

## 2. ⛔ THE RENAME — and the one thing QA must take from it

`at_least` → `at-least` **as a GBNF RULE NAME ONLY**, in both generators:

- `SiegeAssistantGrammar.cpp` — the definition (was `:312`) and the reference in `when` (was `:392`)
- `SiegeLlamaSpike.cpp` — the definition and the reference in `when`

**The JSON key `"at_least"` is byte-for-byte untouched everywhere**: `SiegeAssistantJsonKeys::AtLeast`, Zone A's
`WHEN = "now", or {"kind":KIND,"at_least":1 to 30}` line, the spike's copy of that line, the parser's key
comparison, and every corpus row. In the emitted `when` rule the two now sit one token apart, which is exactly the
shape of the `n` / `count` split this file already had:

```gbnf
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
                                        ^ JSON key      ^ rule name
```

### 2a. The sentence QA asked for, stated plainly

> **STRING-COMPARING TWO GENERATORS CAN NEVER PROVE EITHER ONE IS VALID. ONLY THE TARGET PARSER CAN. A DUMP IS
> NOT A PARSE.**

TASK-419 diffed `USiegeAssistantGrammar::Build` against the spike mirror rule-for-rule and reported that they
matched. **That report was correct.** TASK-413 part 1 diffed the runtime dump against the generator and blessed it.
**That was correct too.** Both were true of a string llama.cpp refuses to load. The two artifacts agreed with each
other and neither was ever checked against the thing that consumes them — and every other assertion in the test
suite (determinism, grounding, the count range, the selection cap, well-formedness) stayed green throughout,
because each is a property of the *string*, not of the *parse*.

**I am not treating this as a review failure.** It is a failure of a method that cannot see this defect class, which
is why the fix below is a mechanical guard rather than a note asking people to look harder.

---

## 3. THE SWEEP — full result

**Method:** enumerated every `AppendRule` call site and every bare (unquoted) identifier passed into a rule body in
both generators, then read both files raw to confirm no rule name is constructed at runtime.

**Both files emit exactly 13 rules, in the same order, with the same reference set** — they are still true mirrors:

`root · command · question · ask · intent · kind · where · count · at-least · item · selection · who · when`

References: `command`, `question` (from `root`) · `intent`, `who`, `where`, `when` (from `command`) · `ask` ·
`kind`, `count` (from `item`) · `item` ×6 (from `selection`) · `selection` (from `who`) · `kind`, `at-least`
(from `when`).

**Findings:**

1. **`at_least` was the ONLY identifier outside `[a-zA-Z0-9-]` in either file** — build-master's expectation
   confirmed by enumeration rather than assumed. It occurred **twice per file** (definition + reference), i.e. four
   sites total, all four fixed.
2. **⚠️ THERE ARE NO GENERATED OR DERIVED RULE NAMES ANYWHERE.** Every rule name is a compile-time `TEXT("...")`
   literal. **This is the load-bearing half of the sweep**, and I checked it rather than assuming it: all
   data-derived text — unit kinds, place symbols, intents, ask codes, counts — reaches the grammar through
   `GbnfJsonString` / `GbnfTerminal`, i.e. **always inside a quoted terminal, never in identifier position**. So a
   live unit called `militia_mob` could never have produced an illegal rule name, and the failure could only ever
   have come from source. That is why the validator can be a hard check with no risk of firing on live data.
3. **⚠️ THE SAME UNDERSCORE IS CORRECT IN THREE OTHER PLACES AND I LEFT IT ALONE** — this asymmetry is what made the
   defect look reasonable: canonical place symbols (`ancient_ground_near`, `own_castle`), JSON keys (`at_least`),
   and `CanonicalizeSymbols`' `[a-z0-9_]` symbol charset. **`CanonicalizeSymbols` is unchanged.** A sweep that
   "made these consistent" would produce a grammar that parses perfectly and emits commands nothing downstream can
   read; the new test asserts these underscores **survive**, in both directions.

---

## 4. THE VALIDATOR — what it now guarantees, and what it does not

Added to `AppendRule` in **both** generators (duplicated, not shared, for the same reason the generator is: the
plugin is architecturally forbidden to include the game header).

**Guarantees:**

- **Every rule NAME** is `[a-zA-Z0-9-]` and non-empty, checked at construction.
- **Every rule REFERENCE** in every right-hand side is too. This half matters: the defect had two halves, and
  llama.cpp rejects an illegal reference for the same reason it rejects an illegal definition.
- **The offender is NAMED** — the log line prints the exact identifier and the rule it appeared in, which is the
  one thing six bench runs of `SPIKE_WARN: ... returned NULL` could not tell anyone.
- **⚠️ CORRECTED AFTER QA — TASK-416 BLOCKER-1. This bullet previously read "Not compiled out of Shipping."
  THAT WAS FALSE**, and it was false in the worst possible way for this particular pass: an assertion about a
  consuming system (UBT/Core) written **from memory instead of read out of it** — the exact failure class the
  pass exists to kill, one system over from where it was killed. What UE 5.8 actually does, read from disk:

  | fact | source |
  |---|---|
  | `USE_LOGGING_IN_SHIPPING` defaults to **0** | `Misc/Build.h:200-203` |
  | Shipping **and Test** ⇒ `NO_LOGGING = !USE_LOGGING_IN_SHIPPING` ⇒ **1** | `Build.h:350-352`, `:322-324` |
  | `NO_LOGGING` ⇒ `UE_LOG` *"will only log Fatal errors"* — an `Error` call becomes an empty `if constexpr(false)` | `Logging/LogMacros.h:184-194` |
  | `USE_CHECKS_IN_SHIPPING` **0**, `USE_ENSURES_IN_SHIPPING` follows it ⇒ **0** | `Build.h:205-212` |
  | Shipping **and Test** ⇒ `DO_ENSURE = USE_ENSURES_IN_SHIPPING` ⇒ **0** | `Build.h:332-334`, `:304-306` |
  | `DO_ENSURE 0` ⇒ `ensureAlwaysMsgf` degrades to `(LIKELY(!!(expr)))` — condition still evaluated, nothing reported | `Misc/AssertionMacros.h:467-472` |
  | `GitClaudeUnrealTest.Target.cs` (15 lines, read in full) sets **NO** `bUseLoggingInShipping` / `bUseChecksInShipping` override | `Source/GitClaudeUnrealTest.Target.cs` |

  ⇒ **BOTH HALVES OF THE GUARD COMPILE OUT IN SHIPPING AND IN TEST.** It is live in Debug, DebugGame and
  Development, editor variants included (`Build.h:241-296` gives both `UE_BUILD_DEBUG` and
  `UE_BUILD_DEVELOPMENT` `NO_LOGGING 0` / `DO_ENSURE 1`).

- **✅ AND THAT IS THE CORRECT SCOPE — a development-time and CI gate, by nature rather than by accident.**
  Stating it as a scope rather than as a confession, because the property being checked is settled before the
  program runs:
  - **No rule name is ever built from data** (finding 2 above, re-derived by QA §3b). Every name is a
    compile-time `TEXT("…")` literal; all data-derived text reaches the grammar inside a quoted terminal via
    `GbnfJsonString` / `GbnfTerminal`. **The guard cannot fire on live data — only on source.**
  - ⇒ **A Shipping build contains exactly the identifiers a Development build contained.** There is no input
    that breaks it in a player's hands and not on a developer's machine — which is the *only* thing that would
    have made a dev-only guard "the same silent failure wearing a different hat".
  - ⇒ **The mechanism that protects a shipped build is the automation test, not this log:**
    **`Siegebound.Assistant.Grammar.RuleNameCharset`** in `Siegebound/Tests/SiegeAssistantGrammarTest.cpp`. It
    runs `Build` over all four builder shapes and asserts this same charset **in CI, before anything is
    packaged.** That is the gate. The runtime guard is the fast local echo that *names the offending
    identifier* in a log line while you are still at the keyboard.
  - **The target flags were NOT flipped.** QA ruled that out, I agree, and it is not this file's decision:
    Shipping logging project-wide has performance and log-volume consequences far outside this batch. Recorded
    as an option for TASK-423.
- **Both channels, where they are live.** The `Error` log names the offender in the run log; `ensureAlwaysMsgf`
  adds a callstack and an automation-visible error so an editor or CI run **fails** on it rather than merely
  mentioning it.
- **Cost:** a linear scan of ~13 short identifiers per `Build` call, once per typed sentence, on a pure function
  with no engine state — unmeasurable beside the inference call it precedes. This is exactly why the plan pinned
  `Build` as pure and designated it the one place automation tests are worth writing.

**Deliberate design choices QA should push on if it disagrees:**

- **The offending rule is still EMITTED, not dropped.** Dropping it would leave a dangling reference and a grammar
  that fails to parse for a *second*, less obvious reason, and `Siege.Llama.SpikeGrammar`'s dump would stop showing
  what the code intended. llama.cpp refuses it either way; the job is to make the refusal loud and named.
- **`ensureAlwaysMsgf`, not `checkf`.** A grammar failure degrades the assistant; it does not corrupt the game. A
  fatal check would trade a broken feature for a crashed match. ⚠️ **This bullet used to end "the Error log is
  the always-on channel" — the same false claim as the one above, in weaker clothing, and QA did not catch this
  one. It is not always-on; it is on in Debug/DebugGame/Development.** The ensure is the *louder* of the two
  where both are live (it fails a run instead of appending to it), and the log is the one that survives a
  packaged Development build. Neither survives Shipping or Test — see the corrected bullet above.
- **The terminal scanner honours the backslash escape and skips terminals as whole units** rather than splitting
  the RHS on whitespace. A generated symbol containing a space is legal-but-odd (`CanonicalizeSymbols` warns and
  emits it escaped), and a whitespace split would tear `"foo bar"` in half and report the fragment as an illegal
  rule name. **A validator that fires on legal input is worse than none**, because it teaches people to ignore it.

**What it does NOT guarantee, stated so nobody over-reads it:** it does not parse the grammar. It checks the one
property that this failure turned on. **The only proof remains a live `llama_sampler_init_grammar` returning
non-NULL**, which is build-master's re-run, and I have not claimed it.

### 4a. The test suite was part of the blind spot — and that is now fixed too

`SiegeAssistantGrammarTest.cpp`'s `IsGrammarWellFormed` collected identifiers with
`FChar::IsAlnum(Char) || Char == TEXT('_')`. **That one character is why the suite blessed the broken grammar**: it
read `at_least` as a single, well-formed, fully-resolved reference. **The scanner now breaks identifiers exactly
where llama.cpp breaks them** (`-`, not `_`), so an illegal reference arrives as unresolvable fragments and fails —
and the error string explains that "half an identifier" means a charset violation, because `undefined rule
referenced: at` is genuinely baffling on its own.

**⚠️ UPDATED FOR QA TASK-416 WARN-3 — the two scanners in this file no longer disagree about the charset.** The
first fix left the file with *two* differently-implemented scanners: `IsLegalGbnfRuleName` used an explicit ASCII
range, while `IsGrammarWellFormed`'s inner loop used `FChar::IsAlnum`. QA was right that the divergence was in the
safe direction and unreachable **and** that two scanners disagreeing about a charset inside one file is how this
exact defect class starts — it is what the `_` was.

Both now call a single `IsGbnfNameChar(TCHAR)`. **It converged on the ASCII range, not on `FChar::IsAlnum`**, and
the direction is the whole point: `IsAlnum` is Unicode/locale-aware, so it would bless an accented letter or a
full-width digit that llama.cpp's plain-ASCII rule-name scan breaks an identifier on. **Converging on the
friendlier-looking predicate would have widened the test's notion of legality past the parser's — the direction
that hides a defect rather than catching one.** Where an identifier *stops* and whether an identifier is *legal*
are one question, and the file now answers it once.

**The remaining three-file duplication is stated in the code rather than left silent** (`SiegeAssistantGrammar.cpp`
· `SiegeLlamaSpike.cpp` · this file). It is not fixable here and the comment says why: the production copy has
internal linkage inside an anonymous namespace and is not declared in the header, so the test cannot reach it;
asserting the charset with the very function under test would also be circular; and the spike's copy is across a
module boundary the plugin may not cross. **Within a file there is now exactly one definition — the part that was
actually fixable.** (Carry-forward on TASK-423, which deletes the spike lane anyway.)

**New test — `Siegebound.Assistant.Grammar.RuleNameCharset`** (the 12th; none removed or weakened). It runs over
**all four builder shapes** (populated · empty roster · empty places · both empty), because a rule emitted on only
one branch is exactly the rule that escapes review, and asserts:

- every rule name is legal GBNF;
- every reference resolves under llama.cpp's identifier rules;
- **the guard itself is exercised** — `at-least`/`root`/`item2` accepted, `at_least`/`at least`/empty rejected. A
  test that always passes is not a gate.
- **the wire format keeps its underscores** — `at_least`, `ancient_ground_near`, `own_castle` must all still be
  present, so the rename cannot be "made consistent" in the wrong direction.

Two existing tests were updated for the rename (`GetRuleRhs(..., "at-least")` in the count-range test;
`HasRule(EmptyRoster, "at-least")` in the degenerate test) and the count-range test gained two rows asserting the
key/rule split explicitly. **⚠️ DECLARED: the test file is TASK-417's single-owner file.** The rename breaks it, so
leaving it would have shipped a knowingly-failing suite; the edits are the rename's necessary consequence plus the
guard the dispatch asked for.

---

## 5. THE CAP RULING — `MaxSnapshotChars` 1440 → 1085

Applied exactly as ruled. The derivation, the date and TASK-413 are recorded beside the constant.

```
1085 = 400 tokens × 2.71 chars/token   (Zone B+C: 955 chars / 352 tokens, measured)
```

Recorded in the comment, per the ruling's conditions:

- **3.6 was optimistic on every reading**, not conservative — Zone B 2.13, Zone C 2.77, B+C 2.71, whole prompt
  3.56. At 2.71, the old 1440 admitted **~531 tokens against a 400-token budget, 33 % over**. The live board
  (955 chars / 352 tok) did not breach it, so **the breach was latent**, not active.
- **2.13 is the stricter bound (⇒ ~850 chars)** and is named as the number to move to **if Zone C ever becomes as
  symbol-dense as Zone B** — with the reason it is not used today (Zone B is 68 chars of dense key/value symbols
  and tokenizes far worse per character than Zone C's roster lines; applying it to the whole region over-tightens
  by ~22 %).
- **TASK-423 supersedes the question entirely** by enforcing the token budget with the tokenizer directly, demoting
  this cap to a pre-filter. **Not implemented here**, and the comment says so.

### 5a. ⚠️ A COUPLING THIS CREATES THAT WAS NOT IN THE RULING — flagging, not fixing

Zone C's budget is `MaxSnapshotChars − ZoneBCharReserve = 1085 − 192 =` **893 chars**.

- At the shipped `MaxRosterKinds = 8`, Zone C runs ~220 chars shorter than the spike's 13-kind fixture (887 chars),
  so a realistic board clears 893 with **~220 to spare**. **No truncation today.**
- At **13** printed kinds Zone C is **887 against 893 — six characters.** So **`MaxSnapshotChars` and
  `MaxRosterKinds` are now coupled**, where at 1440 they were not: TASK-416's WARN-5 seam can no longer be closed
  by simply raising `MaxRosterKinds`.
- ⚠️ **The arithmetic in the two bullets above is DERIVED, not measured** — 46 chars for a `- footman: 8 total, 8
  orderable, 8 followable` line, the five collapsed kinds' name lengths, minus the widened `other_kinds:` line.
  **QA should re-derive it rather than take it from me**, and build-master's re-run prints the real Zone C.
- ⛔ **Do not raise the cap to buy that room** — it restores the 33 % over-admission. The honest lever is
  `ZoneBCharReserve`, which charges 192 for a Zone B that measures **68**. **I did not touch it**: it is a separate
  §10 tunable, changing it alters the budget derivation, and it was not in the ruling.

### 5b. The spike's mirror of the constant

`SiegeLlamaSpike.cpp` printed `MaxSnapshotChars=1440` and `headroom = 1440 - zoneBC` as **bare literals** in the
`SPIKE_TOKENS` line. Left alone, the re-run would have reported a headroom computed against a cap the game no
longer uses — a stale premise inside the very tool used to re-measure. Replaced with `SpikeMaxSnapshotChars = 1085`
and a mirror-duty comment. **⚠️ BUILD-MASTER: the `SPIKE_TOKENS` line will now read `MaxSnapshotChars=1085
headroom=130` (t0), not `1440 / 485`.** If any parsing script pins the old numbers, that is why.

---

## 6. ⚠️ WARN-5 CLOSED — the truncation is no longer silent, and this is not optional beside §5

**The old condition could not fire on the common case.** It tested
`KindsToPrint < FMath::Min(UnitKinds.Num(), MaxRosterKinds)`, which is **false at exactly the cap** — so a 13-kind
board collapsing five kinds into `other_kinds:` logged **nothing, on every sentence, forever**. That is the case
that actually costs accuracy: `GetUnitKinds()` is **never** truncated, so the grammar still admits every collapsed
kind and the sampler can emit a symbol the prompt never showed the model.

**A tighter cap that silently degrades the prompt is worse than the loose one that did not**, so the two changes
ship together and neither is complete alone.

**Now:**

- The test is **"did any kind fail to print in full"** — `UnitKinds.Num() - KindsToPrint > 0`.
- **Both causes are named** in the message: `the MaxRosterKinds cap` vs `the CHARACTER BUDGET, below the
  MaxRosterKinds cap`. They call for different fixes, so conflating them would waste the warning.
- The message carries the numbers needed to act: kinds printed / total / collapsed, roster chars, the budget,
  `MaxSnapshotChars`, `ZoneBCharReserve` — and says **"do NOT raise it to hide this"**.
- **Escalating, not one-shot.** `bWarnedSnapshotTruncated` is replaced by `WarnedRosterKindsPrinted` /
  `WarnedRosterKindsCollapsed`. A steady state still logs **once** (the per-sentence spam the original latch
  existed to prevent), but a **new, deeper** collapse can never hide behind an earlier, milder one — which a plain
  bool made impossible.
- A per-turn `Verbose` line records **every** degraded turn, so a specific scored answer can be reconstructed
  afterwards; the `Warning` fires per escalation and therefore cannot say *which sentence* was answered against a
  trimmed roster.

Both latches are `mutable` (the builders are const by the §9 pin) and process-lifetime, exactly as the bool was —
`ResetSnapshot()` did not touch the old one and does not touch these, which is correct: they are log-spam latches,
not snapshot state.

---

## 7. What QA should scrutinise

1. **⚠️ THE THING I CANNOT PROVE AND HAVE NOT CLAIMED: I did not parse the fixed grammar with llama.cpp.**
   Build-master verified `at-least` parses clean against the same b10235 parser and I am relying on that. **Do not
   let my validator, my test, or a rule-for-rule diff against the spike be read as proof the grammar loads** —
   that is precisely the error that produced this task. The only proof is a non-NULL
   `llama_sampler_init_grammar` on the re-run.
2. **The reference scanner in `CollectRuleReferences`** — the terminal-skipping state machine (quote toggling +
   backslash escape). If it mis-tracks quote state it could either miss an illegal reference or, worse, fire on a
   legal one. Worth a hand-trace against `when`'s RHS and against a symbol containing a space.
3. **§5a's derived arithmetic** (the ~220-char / 6-char headroom figures). Derived, not measured. Re-derive it.
4. **Whether `ensureAlwaysMsgf` is the right loudness**, vs `checkf`. My reasoning is in §4; overrule it if the
   project wants a hard stop.
5. **That the JSON key really is untouched everywhere** — I claim four categories (`SiegeAssistantJsonKeys::AtLeast`,
   Zone A in both lanes, the parsers, the corpus). A repo-wide search for `at_least` now returns only JSON-key
   uses, parser assertions, corpus rows and comments; **no rule name and no rule reference**. Please re-run it
   independently.
6. **The cross-owner edits, declared:** `SiegeAssistantGrammar.{h,cpp}` and the test file are TASK-417's;
   `SiegeAssistantSnapshot.{h,cpp}` is TASK-416's; `SiegeLlamaSpike.cpp` is TASK-410's. One dispatch covered all
   three because the defect and the ruling cut across them.
7. **Comment-trap audit: PASS.** Re-run on **raw reads, never Grep**, over every block comment I touched or that
   sat adjacent to an edit. No `*/` inside a glob pair was introduced. Comments corrected because my edits
   falsified them: the grammar header's degenerate-rule list · the snapshot header's zone table and Zone A figures
   (now TASK-413's measurements) and its `BuildZoneC` "logs ONCE" clause · the snapshot `.cpp`'s Zone-A to grammar
   mirror note · the spike's "well inside MaxSnapshotChars (1440)" claim about its 13-kind deviation (**re-checked,
   still true at 1085 — 130 chars of headroom, not 485, and I said so rather than leaving the adjective**).
8. **Nothing sealed was opened.** The corpus, the holdout, Zone A's bytes, the few-shots, the schema and the
   scoring rule are untouched. **Bar #3's 77.1 % result is undisturbed** — no fixture, no zone boundary and no
   prompt byte moved, so `reused − zoneB_start = 9` should reproduce exactly.
