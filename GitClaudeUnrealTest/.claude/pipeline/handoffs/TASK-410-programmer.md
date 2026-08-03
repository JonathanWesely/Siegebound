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
